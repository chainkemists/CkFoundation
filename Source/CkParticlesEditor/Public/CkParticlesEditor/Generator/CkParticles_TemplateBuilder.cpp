#include "CkParticlesEditor/Generator/CkParticles_TemplateBuilder.h"

#include "CkParticlesEditor_Log.h"

#include "CkParticles/DataInterface/CkParticles_DataInterface.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEmitterFactoryNew.h"
#include "NiagaraTypes.h"
#include "NiagaraConstants.h"
#include "NiagaraEditorUtilities.h"

#include "NiagaraScript.h"
#include "NiagaraScriptSource.h"
#include "NiagaraGraph.h"
#include "NiagaraNodeInput.h"
#include "NiagaraNodeOutput.h"
#include "EdGraphSchema_Niagara.h"
#include "EdGraph/EdGraph.h"
#include "ViewModels/Stack/NiagaraStackGraphUtilities.h"
#include "NiagaraNodeFunctionCall.h"
#include "NiagaraNodeOp.h"
#include "NiagaraNodeWithDynamicPins.h"
#include "NiagaraDataInterface.h"

#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"
#include "NiagaraRibbonRendererProperties.h"

#include "CkParticlesEditor/Generator/CkParticles_MaterialGenerator.h"
#include "CkParticlesEditor/Generator/CkParticles_MeshGenerator.h"
#include "CkParticlesEditor/Generator/CkParticles_TextureGenerator.h"
#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"

#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "MaterialEditingLibrary.h"
#include "MaterialDomain.h"
#include "SceneTypes.h"
#include "MaterialTypes.h"
#include "Materials/Material.h"
#include "Materials/MaterialInstanceConstant.h"
#include "Materials/MaterialExpressionParticleColor.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionTextureBase.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

// --------------------------------------------------------------------------------------------------------------------
// Builds the template Niagara Systems from C++: a GPU emitter, the tagged renderer set, a User.BehaviorId int, and
// the DI wired as a User.ParticleScript parameter. The behavior-call module (Particle Update calling
// ParticleScript.ExecuteStage) is code-built too — it needs the forked engine's exported pin-authoring API
// (CK_WITH_PARTICLES; inert on a stock engine).
// --------------------------------------------------------------------------------------------------------------------

namespace ck::particles_editor
{
    namespace TemplateBuilderLocal
    {
        // Template package paths come from the cadence table (ck::particles::Get_TemplateSpecs) — there is no
        // per-template constant here to drift out of sync with it.

        // Samples a baked texture through a "BaseTexture" parameter that per-effect material instances swap later.
        // Returns nullptr on failure — the template then falls back to the default sprite material.
        static auto Build_VfxMasterMaterial(UTexture2D* InBaseTexture) -> UMaterialInterface*
        {
            static const TCHAR* MatPkgPath = TEXT("/CkFoundation/CkParticles/Templates/M_CkParticles_VfxMaster");
            static const TCHAR* MatName    = TEXT("M_CkParticles_VfxMaster");

            UPackage* Package = FPackageName::DoesPackageExist(MatPkgPath)
                ? LoadPackage(nullptr, MatPkgPath, LOAD_None)
                : nullptr;
            if (Package == nullptr) { Package = CreatePackage(MatPkgPath); }
            if (Package == nullptr) { return nullptr; }

            if (auto* Old = StaticFindObject(UMaterial::StaticClass(), Package, MatName))
            {
                Old->ClearFlags(RF_Standalone | RF_Public);
                Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
            }

            auto* Mat = NewObject<UMaterial>(Package, MatName, RF_Public | RF_Standalone);
            if (Mat == nullptr) { return nullptr; }

            Mat->MaterialDomain = MD_Surface;
            Mat->BlendMode      = BLEND_Additive;
            Mat->SetShadingModel(MSM_Unlit);
            Mat->bUsedWithNiagaraSprites = true;

            auto* TexSample = Cast<UMaterialExpressionTextureSampleParameter2D>(
                UMaterialEditingLibrary::CreateMaterialExpression(Mat, UMaterialExpressionTextureSampleParameter2D::StaticClass(), -650, 0));
            auto* PColor = Cast<UMaterialExpressionParticleColor>(
                UMaterialEditingLibrary::CreateMaterialExpression(Mat, UMaterialExpressionParticleColor::StaticClass(), -650, 280));
            auto* MulRgb = Cast<UMaterialExpressionMultiply>(
                UMaterialEditingLibrary::CreateMaterialExpression(Mat, UMaterialExpressionMultiply::StaticClass(), -320, 120));

            if (TexSample == nullptr || PColor == nullptr || MulRgb == nullptr)
            { return nullptr; }

            // Auto-pick the sampler type so it matches our linear (SRGB=false) textures — a mismatch fails compilation.
            TexSample->ParameterName = TEXT("BaseTexture");
            if (InBaseTexture != nullptr)
            {
                TexSample->Texture     = InBaseTexture;
                TexSample->SamplerType = UMaterialExpressionTextureBase::GetSamplerTypeForTexture(InBaseTexture);
            }

            // Default "" outputs avoid pin-name guessing; Opacity only matters for translucent variants.
            UMaterialEditingLibrary::ConnectMaterialExpressions(TexSample, TEXT(""),  MulRgb, TEXT("A"));
            UMaterialEditingLibrary::ConnectMaterialExpressions(PColor,    TEXT(""),  MulRgb, TEXT("B"));
            UMaterialEditingLibrary::ConnectMaterialProperty(MulRgb,    TEXT(""),  MP_EmissiveColor);
            UMaterialEditingLibrary::ConnectMaterialProperty(TexSample, TEXT("A"), MP_Opacity);

            UMaterialEditingLibrary::LayoutMaterialExpressions(Mat);
            UMaterialEditingLibrary::RecompileMaterial(Mat);

            Mat->MarkPackageDirty();
            FAssetRegistryModule::AssetCreated(Mat);

            const auto FileName = FPackageName::LongPackageNameToFilename(MatPkgPath, FPackageName::GetAssetPackageExtension());
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            UPackage::SavePackage(Package, Mat, *FileName, SaveArgs);

            return Mat;
        }

        // A spawn picks one per effect through the sprite renderer's user-param binding (SetVariableMaterial).
        static auto Build_TextureMaterialInstances(UMaterialInterface* InMaster) -> void
        {
            if (InMaster == nullptr) { return; }

            static const TCHAR* TexNames[] = { TEXT("Glow"), TEXT("Flare"), TEXT("Smoke"), TEXT("Electric"), TEXT("Streak"), TEXT("Ring"), TEXT("SweepStreak"), TEXT("TileNoise") };
            for (const TCHAR* Tex : TexNames)
            {
                const FString MicName    = FString::Printf(TEXT("MI_CkParticles_%s"), Tex);
                const FString MicPkgPath = FString::Printf(TEXT("/CkFoundation/CkParticles/Materials/%s"), *MicName);

                UPackage* Package = FPackageName::DoesPackageExist(MicPkgPath) ? LoadPackage(nullptr, *MicPkgPath, LOAD_None) : nullptr;
                if (Package == nullptr) { Package = CreatePackage(*MicPkgPath); }
                if (Package == nullptr) { continue; }

                if (auto* Old = StaticFindObject(UMaterialInstanceConstant::StaticClass(), Package, *MicName))
                {
                    Old->ClearFlags(RF_Standalone | RF_Public);
                    Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
                }

                auto* Mic = NewObject<UMaterialInstanceConstant>(Package, *MicName, RF_Public | RF_Standalone);
                if (Mic == nullptr) { continue; }

                Mic->SetParentEditorOnly(InMaster);
                if (auto* Tx = LoadObject<UTexture>(nullptr, *ck::particles::Get_VfxTextureObjectPath(FName(Tex))))
                { Mic->SetTextureParameterValueEditorOnly(FMaterialParameterInfo(TEXT("BaseTexture")), Tx); }
                Mic->PostEditChange();

                Mic->MarkPackageDirty();
                FAssetRegistryModule::AssetCreated(Mic);

                const auto FileName = FPackageName::LongPackageNameToFilename(MicPkgPath, FPackageName::GetAssetPackageExtension());
                FSavePackageArgs SaveArgs;
                SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
                UPackage::SavePackage(Package, Mic, *FileName, SaveArgs);
            }
        }

        // ---- Module + rapid-iteration plumbing (mirrors NiagaraEmitterFactoryNew's internals, which are
        // file-local there; the underlying APIs are exported) ----------------------------------------------------

        static auto Add_ModuleFromAssetPath(
            const TCHAR*       InAssetPath,
            UNiagaraNodeOutput& InOutputNode) -> UNiagaraNodeFunctionCall*
        {
            auto* ModuleScript = LoadObject<UNiagaraScript>(nullptr, InAssetPath);
            if (ModuleScript == nullptr)
            {
                ck::particles_editor::Log(TEXT("Template builder: engine module script missing [{}]"), FString(InAssetPath));
                return nullptr;
            }
            return FNiagaraStackGraphUtilities::AddScriptModuleToStack(ModuleScript, InOutputNode, INDEX_NONE);
        }

        // Writes a module input as a rapid-iteration constant: Constants.<UniqueEmitterName>.<ModuleCallName>.<Input>
        template <typename T_Value>
        static auto Set_ModuleRapidIterationValue(
            const FString&                InUniqueEmitterName,
            UNiagaraScript*               InTargetScript,
            const UNiagaraNodeFunctionCall* InModuleNode,
            const TCHAR*                  InInputName,
            const FNiagaraTypeDefinition& InType,
            T_Value                       InValue) -> void
        {
            if (InTargetScript == nullptr || InModuleNode == nullptr)
            { return; }

            const auto AliasedName = FString::Printf(TEXT("%s.%s"), *InModuleNode->GetFunctionName(), InInputName);
            auto RipVar = FNiagaraUtilities::ConvertVariableToRapidIterationConstantName(
                FNiagaraVariable(InType, FName(*AliasedName)), *InUniqueEmitterName, InTargetScript->GetUsage());
            RipVar.SetValue(InValue);

            constexpr auto AddIfMissing = true;
            InTargetScript->RapidIterationParameters.SetParameterData(RipVar.GetData(), RipVar, AddIfMissing);
        }

        static auto Find_StageOutputNode(
            const FVersionedNiagaraEmitterData* InEmitterData,
            ENiagaraScriptUsage                 InUsage,
            UNiagaraScript*                     InScript) -> UNiagaraNodeOutput*
        {
            if (InEmitterData == nullptr || InScript == nullptr)
            { return nullptr; }

            const auto* Source = Cast<UNiagaraScriptSource>(InEmitterData->GraphSource);
            if (Source == nullptr || Source->NodeGraph == nullptr)
            { return nullptr; }

            return Source->NodeGraph->FindEquivalentOutputNode(InUsage, InScript->GetUsageId());
        }

        // ---- Renderer set (shared by both templates) -----------------------------------------------------------
        // Every behavior writes Particles.VisibilityTag, so each renderer draws only its tagged particles:
        // 0 camera sprite / 1 velocity-aligned sprite / 2 smoke sprite / 3 carrier meshes (Particles.MeshIndex).
        // Were the tag attribute ever missing, Niagara would render every particle in EVERY renderer.
        // A row-declared renderer binds a generated CkUsf master EXPLICITLY — one User.SpriteMaterial cannot carry
        // several. A miss is an Error, not a shrug: the renderer would silently draw the default material.
        static auto Load_LookMaster(const TCHAR* InLookName) -> UMaterialInterface*
        {
            if (InLookName == nullptr)
            { return nullptr; }

            const auto Path = ck::particles::Get_GeneratedLookMasterObjectPath(FName(InLookName));
            auto* Master = LoadObject<UMaterialInterface>(nullptr, *Path);

            if (Master == nullptr)
            {
                ck::particles_editor::Error(TEXT("Template builder: CkUsf look master [{}] missing — generate the "
                    "looks (Ck_Usf_GenerateLooks) BEFORE rebuilding the templates, or its renderer draws the "
                    "default material"), FString(Path));
            }
            return Master;
        }

        // The three sprite kinds differ ONLY in the alignment/facing pair Niagara resolves the quad with, so one
        // emission path serves all three; an unset result means the kind is not a sprite at all.
        struct FSpriteFacingPair
        {
            ENiagaraSpriteAlignment  Alignment;
            ENiagaraSpriteFacingMode Facing;
        };

        static auto Get_SpriteFacingPair(
            const ck::particles::ECk_ParticlesRenderer_Kind InKind) -> TOptional<FSpriteFacingPair>
        {
            using EKind = ck::particles::ECk_ParticlesRenderer_Kind;

            switch (InKind)
            {
                case EKind::CameraFacingSprite:
                    return FSpriteFacingPair{ENiagaraSpriteAlignment::Unaligned, ENiagaraSpriteFacingMode::FaceCamera};
                case EKind::VelocityAlignedSprite:
                    return FSpriteFacingPair{ENiagaraSpriteAlignment::VelocityAligned, ENiagaraSpriteFacingMode::FaceCamera};
                // The VisTag-4 pair: both Particles.SpriteAlignment and Particles.SpriteFacing must exist, or
                // Niagara silently falls back to Unaligned/FaceCamera. CkParticles_DefaultOutput seeds them.
                case EKind::CustomFacingSprite:
                    return FSpriteFacingPair{ENiagaraSpriteAlignment::CustomAlignment, ENiagaraSpriteFacingMode::CustomFacingVector};
                default:
                    return {};
            }
        }

        // Niagara's own default is (1,1) — one frame, no division. A row that declares no sheet must keep it,
        // because a (0,0) written through would divide the quad's UVs by zero.
        static auto Get_RendererSubImageSize(const FIntPoint& InSubImageSize) -> FVector2D
        {
            return InSubImageSize.X > 0 && InSubImageSize.Y > 0
                ? FVector2D(static_cast<double>(InSubImageSize.X), static_cast<double>(InSubImageSize.Y))
                : FVector2D(1.0, 1.0);
        }

        // Renderers a cadence row declares for itself, on top of the shared set. They exist only on that row's
        // template, so nothing effect-specific reaches any other template.
        static auto Configure_RowRenderers(
            UNiagaraEmitter*                                InEmitter,
            const FGuid&                                    InVersion,
            const ck::particles::FCk_ParticlesTemplateSpec& InSpec) -> void
        {
            auto Index = 0;
            for (const auto& Renderer : InSpec.RendererOverrides)
            {
                // A ribbon renderer belongs to the row's SECOND emitter and has no visibility tag to gate it, so one
                // listed here would link every particle on this emitter into ribbons. Falling through would emit a
                // MESH renderer instead — wrong, and silent.
                if (Renderer.Kind == ck::particles::ECk_ParticlesRenderer_Kind::Ribbon)
                {
                    ck::particles_editor::Error(TEXT("Template builder: row [{}] lists a Ribbon renderer among its "
                        "shared-emitter overrides — ribbon renderers belong to the row's RibbonEmitter spec"),
                        FString(InSpec.AssetName));
                    ++Index;
                    continue;
                }

                auto* LookMaster = Load_LookMaster(Renderer.LookName);

                if (const auto FacingPair = Get_SpriteFacingPair(Renderer.Kind);
                    FacingPair.IsSet())
                {
                    auto* Sprite = NewObject<UNiagaraSpriteRendererProperties>(
                        InEmitter, *FString::Printf(TEXT("SpriteRenderer_Row%d"), Index));
                    Sprite->Alignment          = FacingPair->Alignment;
                    Sprite->FacingMode         = FacingPair->Facing;
                    Sprite->RendererVisibility = Renderer.VisTag;
                    Sprite->Material           = LookMaster;
                    Sprite->SubImageSize       = Get_RendererSubImageSize(Renderer.SubImageSize);
                    InEmitter->AddRenderer(Sprite, InVersion);
                    ++Index;
                    continue;
                }

                auto* Mesh = LoadObject<UStaticMesh>(nullptr, *ck::particles::Get_VfxMeshObjectPath(FName(Renderer.MeshName)));
                if (Mesh == nullptr)
                {
                    ck::particles_editor::Error(TEXT("Template builder: row mesh [{}] missing — run "
                        "Generate_AllVfxMeshes first"), FString(Renderer.MeshName));
                    ++Index;
                    continue;
                }

                auto* MeshRenderer = NewObject<UNiagaraMeshRendererProperties>(
                    InEmitter, *FString::Printf(TEXT("MeshRenderer_Row%d"), Index));
                MeshRenderer->RendererVisibility = Renderer.VisTag;
                MeshRenderer->FacingMode = ENiagaraMeshFacingMode::Default;
                MeshRenderer->SubImageSize = Get_RendererSubImageSize(Renderer.SubImageSize);
                MeshRenderer->Meshes.Empty();

                auto MeshEntry = FNiagaraMeshRendererMeshProperties{};
                MeshEntry.Mesh = Mesh;
                MeshRenderer->Meshes.Add(MeshEntry);

                if (LookMaster != nullptr)
                {
                    MeshRenderer->bOverrideMaterials = true;
                    auto Override = FNiagaraMeshMaterialOverride{};
                    Override.ExplicitMat = LookMaster;
                    MeshRenderer->OverrideMaterials = { Override };
                }

                InEmitter->AddRenderer(MeshRenderer, InVersion);
                ++Index;
            }
        }

        // The ribbon emitter's whole renderer set. It carries NONE of the shared set: this emitter exists so the
        // ribbon has a particle population of its own, and every particle in it is trail geometry.
        //
        // Both bindings read attributes the stage already writes (see the naming header's ribbon-emitter block):
        // width takes one float at Particles.SpriteSize's offset, i.e. Size.x, and the ribbon id rides
        // Particles.MeshIndex, which a ribbon renderer otherwise ignores because ribbons have no carrier mesh.
        static auto Configure_RibbonRenderers(
            UNiagaraEmitter*                                     InEmitter,
            const FGuid&                                         InVersion,
            const ck::particles::FCk_ParticlesRibbonEmitterSpec& InSpec) -> void
        {
            auto Index = 0;
            for (const auto& Renderer : InSpec.Renderers)
            {
                auto* LookMaster = Load_LookMaster(Renderer.LookName);

                // MATUSAGE_NiagaraRibbons is its own usage flag: a master that never opted in draws as the DEFAULT
                // material, which is the same silent miss a missing master would be.
                if (const auto* BaseMaterial = LookMaster != nullptr ? LookMaster->GetMaterial() : nullptr;
                    BaseMaterial != nullptr && NOT BaseMaterial->bUsedWithNiagaraRibbons)
                {
                    ck::particles_editor::Error(TEXT("Template builder: CkUsf look [{}] is drawn by a ribbon renderer "
                        "but its master does not declare _UsedWithNiagaraRibbons — set the flag on the look "
                        "definition and regenerate the looks (Ck_Usf_GenerateLooks), or the ribbon draws the default "
                        "material"), FString(Renderer.LookName));
                }

                auto* Ribbon = NewObject<UNiagaraRibbonRendererProperties>(
                    InEmitter, *FString::Printf(TEXT("RibbonRenderer_Row%d"), Index));
                Ribbon->Material = LookMaster;
                Ribbon->RibbonWidthBinding = FNiagaraConstants::GetAttributeDefaultBinding(SYS_PARAM_PARTICLES_SPRITE_SIZE);
                Ribbon->RibbonIdBinding    = FNiagaraConstants::GetAttributeDefaultBinding(SYS_PARAM_PARTICLES_MESH_INDEX);

                InEmitter->AddRenderer(Ribbon, InVersion);
                ++Index;
            }
        }

        static auto Configure_Renderers(
            UNiagaraEmitter*              InEmitter,
            const FGuid&                  InVersion,
            FVersionedNiagaraEmitterData* InEmitterData,
            UMaterialInterface*           InSpriteMaterial) -> void
        {
            auto* BaseSprite = static_cast<UNiagaraSpriteRendererProperties*>(nullptr);
            for (UNiagaraRendererProperties* Renderer : InEmitterData->GetRenderers())
            {
                if (auto* Sprite = Cast<UNiagaraSpriteRendererProperties>(Renderer))
                { BaseSprite = Sprite; break; }
            }
            if (BaseSprite == nullptr)
            {
                BaseSprite = NewObject<UNiagaraSpriteRendererProperties>(InEmitter, TEXT("SpriteRenderer_Camera"));
                InEmitter->AddRenderer(BaseSprite, InVersion);
            }
            BaseSprite->RendererVisibility = 0;
            if (InSpriteMaterial != nullptr)
            {
                BaseSprite->Material = InSpriteMaterial; // fallback if the user-param binding is unset/unresolved
                BaseSprite->MaterialUserParamBinding.Parameter =
                    FNiagaraVariable(FNiagaraTypeDefinition::GetUMaterialDef(), ck::particles::Get_SpriteMaterialParameterName());
            }

            // Streaks/tracers: the stretch is Particles.SpriteSize.y along motion.
            auto* VelocitySprite = NewObject<UNiagaraSpriteRendererProperties>(InEmitter, TEXT("SpriteRenderer_Velocity"));
            VelocitySprite->Alignment  = ENiagaraSpriteAlignment::VelocityAligned;
            VelocitySprite->FacingMode = ENiagaraSpriteFacingMode::FaceCamera;
            VelocitySprite->RendererVisibility = 1;
            if (auto* StreakMi = LoadObject<UMaterialInterface>(nullptr, *ck::particles::Get_TextureMaterialInstanceObjectPath(TEXT("Streak"))))
            { VelocitySprite->Material = StreakMi; }
            else
            { VelocitySprite->Material = InSpriteMaterial; }
            InEmitter->AddRenderer(VelocitySprite, InVersion);

            // Translucent depth-faded soft material; Particles.SpriteRotation applies.
            auto* SmokeSprite = NewObject<UNiagaraSpriteRendererProperties>(InEmitter, TEXT("SpriteRenderer_Smoke"));
            SmokeSprite->RendererVisibility = 2;
            if (auto* SmokeMat = LoadObject<UMaterialInterface>(nullptr, *ck::particles::Get_VfxMasterMaterialObjectPath(TEXT("SoftSmoke"))))
            { SmokeSprite->Material = SmokeMat; }
            else
            { SmokeSprite->Material = InSpriteMaterial; }
            InEmitter->AddRenderer(SmokeSprite, InVersion);

            // Ground decals / range rings: a quad fixed in SIM space rather than billboarded at the camera,
            // driven by Particles.SpriteAlignment (its up axis) + Particles.SpriteFacing (its plane normal).
            // Shares User.SpriteMaterial with the camera sprite, so a behavior-bound CkUsf look reaches it
            // through the same binding and no caller needs to know which renderer drew the particle.
            auto* CustomFacingSprite = NewObject<UNiagaraSpriteRendererProperties>(InEmitter, TEXT("SpriteRenderer_CustomFacing"));
            CustomFacingSprite->Alignment  = ENiagaraSpriteAlignment::CustomAlignment;
            CustomFacingSprite->FacingMode = ENiagaraSpriteFacingMode::CustomFacingVector;
            CustomFacingSprite->RendererVisibility = 4;
            if (InSpriteMaterial != nullptr)
            {
                CustomFacingSprite->Material = InSpriteMaterial;
                CustomFacingSprite->MaterialUserParamBinding.Parameter =
                    FNiagaraVariable(FNiagaraTypeDefinition::GetUMaterialDef(), ck::particles::Get_SpriteMaterialParameterName());
            }
            InEmitter->AddRenderer(CustomFacingSprite, InVersion);

            // Particles.MeshIndex picks the carrier; Particles.Scale + Particles.MeshOrientation apply.
            auto* MeshRenderer = NewObject<UNiagaraMeshRendererProperties>(InEmitter, TEXT("MeshRenderer_Carriers"));
            MeshRenderer->RendererVisibility = 3;
            MeshRenderer->FacingMode = ENiagaraMeshFacingMode::Default;
            MeshRenderer->Meshes.Empty();

            static const TCHAR* CarrierNames[] = { TEXT("Sweep"), TEXT("Tube"), TEXT("Shell"), TEXT("Disc") };
            for (const TCHAR* Carrier : CarrierNames)
            {
                auto* CarrierMesh = LoadObject<UStaticMesh>(nullptr, *ck::particles::Get_VfxMeshObjectPath(Carrier));
                if (CarrierMesh == nullptr)
                {
                    ck::particles_editor::Log(TEXT("Template builder: carrier mesh [{}] missing — run Generate_AllVfxMeshes first"), FString(Carrier));
                    continue;
                }
                auto MeshEntry = FNiagaraMeshRendererMeshProperties{};
                MeshEntry.Mesh = CarrierMesh;
                MeshRenderer->Meshes.Add(MeshEntry);
            }
            InEmitter->AddRenderer(MeshRenderer, InVersion);
        }

        // ---- Declared spawn stack (built from an EMPTY emitter, so the factory's own SpawnRate 10 never
        // survives underneath a row that states its own cadence) -------------------------------------------------
        // A row may declare a burst, a continuous rate, or both; the two modules compose on one emitter exactly as
        // they do on the source emitters this recreates.
        static auto Add_SpawnEmitterStack(
            UNiagaraEmitter*                                InEmitter,
            FVersionedNiagaraEmitterData*                   InEmitterData,
            const ck::particles::FCk_ParticlesTemplateSpec& InSpec) -> bool
        {
            auto* EmitterUpdateOut  = Find_StageOutputNode(InEmitterData, ENiagaraScriptUsage::EmitterUpdateScript,  InEmitterData->EmitterUpdateScriptProps.Script);
            auto* ParticleSpawnOut  = Find_StageOutputNode(InEmitterData, ENiagaraScriptUsage::ParticleSpawnScript,  InEmitterData->SpawnScriptProps.Script);
            auto* ParticleUpdateOut = Find_StageOutputNode(InEmitterData, ENiagaraScriptUsage::ParticleUpdateScript, InEmitterData->UpdateScriptProps.Script);
            if (EmitterUpdateOut == nullptr || ParticleSpawnOut == nullptr || ParticleUpdateOut == nullptr)
            { return false; }

            const auto UniqueEmitterName = InEmitter->GetUniqueEmitterName();
            auto* EmitterUpdateScript = InEmitterData->EmitterUpdateScriptProps.Script.Get();

            auto* EmitterStateNode = Add_ModuleFromAssetPath(TEXT("/Niagara/Modules/Emitter/EmitterState.EmitterState"), *EmitterUpdateOut);
            Set_ModuleRapidIterationValue(UniqueEmitterName, EmitterUpdateScript, EmitterStateNode,
                TEXT("Loop Duration"), FNiagaraTypeDefinition::GetFloatDef(), InSpec.LoopDuration);

            if (InSpec.BurstCount > 0)
            {
                auto* BurstNode = Add_ModuleFromAssetPath(TEXT("/Niagara/Modules/Emitter/SpawnBurst_Instantaneous.SpawnBurst_Instantaneous"), *EmitterUpdateOut);
                if (BurstNode == nullptr)
                { return false; }
                // The module's input variable name has drifted across engine versions ("Spawn Count" in the assets the
                // corpus captured; "SpawnCount" in newer wizard code) — write both spellings; the compile binds whichever
                // the module graph declares and the other stays an inert orphan constant.
                Set_ModuleRapidIterationValue(UniqueEmitterName, EmitterUpdateScript, BurstNode, TEXT("Spawn Count"), FNiagaraTypeDefinition::GetIntDef(),   InSpec.BurstCount);
                Set_ModuleRapidIterationValue(UniqueEmitterName, EmitterUpdateScript, BurstNode, TEXT("SpawnCount"),  FNiagaraTypeDefinition::GetIntDef(),   InSpec.BurstCount);
                Set_ModuleRapidIterationValue(UniqueEmitterName, EmitterUpdateScript, BurstNode, TEXT("Spawn Time"),  FNiagaraTypeDefinition::GetFloatDef(), 0.0f);
                Set_ModuleRapidIterationValue(UniqueEmitterName, EmitterUpdateScript, BurstNode, TEXT("SpawnTime"),   FNiagaraTypeDefinition::GetFloatDef(), 0.0f);
            }

            if (InSpec.SpawnRate > 0.0f)
            {
                auto* RateNode = Add_ModuleFromAssetPath(TEXT("/Niagara/Modules/Emitter/SpawnRate.SpawnRate"), *EmitterUpdateOut);
                if (RateNode == nullptr)
                { return false; }
                // One spelling only, unlike the burst module: the engine's SpawnRate.uasset declares its input as
                // Module.SpawnRate, verified on disk.
                Set_ModuleRapidIterationValue(UniqueEmitterName, EmitterUpdateScript, RateNode, TEXT("SpawnRate"), FNiagaraTypeDefinition::GetFloatDef(), InSpec.SpawnRate);
            }

            Add_ModuleFromAssetPath(TEXT("/Niagara/Modules/Spawn/Location/SystemLocation.SystemLocation"), *ParticleSpawnOut);

            const auto Vars = TArray<FNiagaraVariable>
            {
                SYS_PARAM_PARTICLES_SPRITE_SIZE,
                SYS_PARAM_PARTICLES_SPRITE_ROTATION,
                SYS_PARAM_PARTICLES_LIFETIME,
            };
            const auto Defaults = TArray<FString>
            {
                FNiagaraConstants::GetAttributeDefaultValue(SYS_PARAM_PARTICLES_SPRITE_SIZE),
                FNiagaraConstants::GetAttributeDefaultValue(SYS_PARAM_PARTICLES_SPRITE_ROTATION),
                // Lifetime comes from the spec, not the loop duration: a source system may outlive its own loop
                // (NS_Lightning_Range is 1.1s lifetime on a 1.0s loop) and rounding that away changes the overlap.
                FString::SanitizeFloat(InSpec.ParticleLifetime),
            };
            FNiagaraStackGraphUtilities::AddParameterModuleToStack(Vars, *ParticleSpawnOut, INDEX_NONE, Defaults);

            Add_ModuleFromAssetPath(TEXT("/Niagara/Modules/Update/Lifetime/UpdateAge.UpdateAge"), *ParticleUpdateOut);
            return true;
        }

#if CK_WITH_PARTICLES
        // The Map Get / Map Set node types are Private (no public header), so resolve the UClass by path, NewObject
        // as the public base, and add it to the graph manually — FGraphNodeCreator<T> needs the compile-time type.
        static auto Create_DynamicPinNode(
            UNiagaraGraph* InGraph,
            const TCHAR*   InClassPath) -> UNiagaraNodeWithDynamicPins*
        {
            UClass* NodeClass = FindObject<UClass>(nullptr, InClassPath);
            if (NodeClass == nullptr)
            { NodeClass = StaticLoadClass(UNiagaraNodeWithDynamicPins::StaticClass(), nullptr, InClassPath); }
            if (NodeClass == nullptr)
            { return nullptr; }

            auto* Node = NewObject<UNiagaraNodeWithDynamicPins>(InGraph, NodeClass, NAME_None, RF_Transactional);
            InGraph->AddNode(Node, false, false);
            Node->CreateNewGuid();
            Node->PostPlacedNewNode();
            Node->AllocateDefaultPins();
            return Node;
        }

        static auto Find_ParameterMapPin(
            UEdGraphNode*                 InNode,
            EEdGraphPinDirection          InDirection,
            const UEdGraphSchema_Niagara* InSchema) -> UEdGraphPin*
        {
            for (UEdGraphPin* Pin : InNode->Pins)
            {
                if (Pin->Direction == InDirection &&
                    InSchema->PinToTypeDefinition(Pin) == FNiagaraTypeDefinition::GetParameterMapDef())
                { return Pin; }
            }
            return nullptr;
        }

        static auto Find_PinByName(
            UEdGraphNode*        InNode,
            FName                InName,
            EEdGraphPinDirection InDirection) -> UEdGraphPin*
        {
            for (UEdGraphPin* Pin : InNode->Pins)
            {
                if (Pin->Direction == InDirection && Pin->PinName == InName)
                { return Pin; }
            }
            return nullptr;
        }

        // Seed bank adder: Particles.UniqueID + ck::particles::RibbonSeedBase, the one graph difference between the
        // ribbon emitter's behavior call and the main emitter's.
        //
        // Every pin is retyped to int. The Add op is authored over GENERIC NUMERIC pins, and a numeric pin carries
        // no literal the translator can read — the resolved type is what a graph stores once the operand types are
        // known, which here they are on both sides.
        static auto Create_SeedBankAddNode(
            UNiagaraGraph*                InGraph,
            const UEdGraphSchema_Niagara* InSchema,
            int32                         InSeedBank) -> UNiagaraNodeOp*
        {
            FGraphNodeCreator<UNiagaraNodeOp> OpCreator(*InGraph);
            auto* OpNode = OpCreator.CreateNode();
            OpNode->OpName = TEXT("Numeric::Add");
            OpCreator.Finalize();

            auto* PinA      = Find_PinByName(OpNode, TEXT("A"),      EGPD_Input);
            auto* PinB      = Find_PinByName(OpNode, TEXT("B"),      EGPD_Input);
            auto* PinResult = Find_PinByName(OpNode, TEXT("Result"), EGPD_Output);
            if (PinA == nullptr || PinB == nullptr || PinResult == nullptr)
            { return nullptr; }

            const auto IntPinType = InSchema->TypeDefinitionToPinType(FNiagaraTypeDefinition::GetIntDef());
            PinA->PinType      = IntPinType;
            PinB->PinType      = IntPinType;
            PinResult->PinType = IntPinType;

            PinB->DefaultValue              = FString::FromInt(InSeedBank);
            PinB->AutogeneratedDefaultValue = PinB->DefaultValue;

            return OpNode;
        }

        // A Module-usage script whose graph is Input -> Map Get (reads DI + params) -> ExecuteStage (DI member fn)
        // -> Map Set (Particles.*) -> Output. The Particles.Position (LWC) vs DI Vec3 difference is auto-bridged
        // by TryCreateConnection.
        //
        // InSeedBank is 0 on the main emitter and ck::particles::RibbonSeedBase on the ribbon emitter, which is the
        // ONLY thing that tells the two populations apart — same DI, same signature, same behavior id.
        static auto Build_BehaviorModuleScript(
            UObject*     InOuter,
            const TCHAR* InScriptName,
            int32        InSeedBank) -> UNiagaraScript*
        {
            auto* Script = NewObject<UNiagaraScript>(InOuter, InScriptName, RF_Transactional);
            Script->SetUsage(ENiagaraScriptUsage::Module);

            auto* Source = NewObject<UNiagaraScriptSource>(Script, NAME_None, RF_Transactional);
            auto* Graph  = NewObject<UNiagaraGraph>(Source, NAME_None, RF_Transactional);
            Source->NodeGraph = Graph;

            const auto* Schema = Cast<UEdGraphSchema_Niagara>(Graph->GetSchema());
            if (Schema == nullptr)
            { return nullptr; }

            // ---- Input / Output parameter-map nodes ----
            FGraphNodeCreator<UNiagaraNodeOutput> OutputCreator(*Graph);
            auto* OutputNode = OutputCreator.CreateNode();
            OutputNode->SetUsage(ENiagaraScriptUsage::Module);
            OutputNode->Outputs.Add(FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("Output")));
            OutputCreator.Finalize();

            FGraphNodeCreator<UNiagaraNodeInput> InputCreator(*Graph);
            auto* InputNode = InputCreator.CreateNode();
            InputNode->Usage = ENiagaraInputNodeUsage::Parameter;
            InputNode->Input = FNiagaraVariable(FNiagaraTypeDefinition::GetParameterMapDef(), TEXT("MapIn"));
            InputCreator.Finalize();

            // ---- Map Get / Map Set (Private node types via reflection) ----
            auto* MapGet = Create_DynamicPinNode(Graph, TEXT("/Script/NiagaraEditor.NiagaraNodeParameterMapGet"));
            auto* MapSet = Create_DynamicPinNode(Graph, TEXT("/Script/NiagaraEditor.NiagaraNodeParameterMapSet"));
            if (MapGet == nullptr || MapSet == nullptr)
            { return nullptr; }

            // ---- ExecuteStage DI function-call node (Signature-based member call, no FunctionScript) ----
            auto* DiCdo = CastChecked<UNiagaraDataInterface>(UCkParticles_DataInterface::StaticClass()->GetDefaultObject());
            TArray<FNiagaraFunctionSignature> DiSignatures;
            DiCdo->GetFunctionSignatures(DiSignatures);
            const auto* ExecSig = DiSignatures.FindByPredicate(
                [](const FNiagaraFunctionSignature& InSig){ return InSig.Name == TEXT("ExecuteStage"); });
            if (ExecSig == nullptr)
            { return nullptr; }

            FGraphNodeCreator<UNiagaraNodeFunctionCall> FuncCreator(*Graph);
            auto* FuncNode = FuncCreator.CreateNode();
            FuncNode->FunctionScript = nullptr;
            FuncNode->Signature = *ExecSig;
            FuncNode->Signature.bMemberFunction = true;
            FuncCreator.Finalize();

            // ---- Read pins on Map Get (outputs = values read FROM the map) ----
            const FNiagaraTypeDefinition DiType(UCkParticles_DataInterface::StaticClass());
            UEdGraphPin* GetScript   = MapGet->RequestNewTypedPin(EGPD_Output, DiType,                                  TEXT("User.ParticleScript"));
            UEdGraphPin* GetBehavior = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetIntDef(),      TEXT("User.BehaviorId"));
            UEdGraphPin* GetDelta    = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetFloatDef(),    TEXT("Engine.DeltaTime"));
            UEdGraphPin* GetAge      = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetFloatDef(),    TEXT("Particles.Age"));
            UEdGraphPin* GetLifetime = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetFloatDef(),    TEXT("Particles.Lifetime"));
            UEdGraphPin* GetPosition = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetPositionDef(), TEXT("Particles.Position"));
            UEdGraphPin* GetVelocity = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetVec3Def(),     TEXT("Particles.Velocity"));
            UEdGraphPin* GetSeed     = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetIntDef(),      TEXT("Particles.UniqueID"));
            // The EMITTER's clock, which keeps running as particles are born and die — a particle's own Age
            // subtracted from it is the moment in the loop it spawned.
            UEdGraphPin* GetEmitterAge = MapGet->RequestNewTypedPin(EGPD_Output, FNiagaraTypeDefinition::GetFloatDef(), TEXT("Emitter.Age"));

            // ---- Write pins on Map Set (inputs = values written TO the map) ----
            UEdGraphPin* SetPosition    = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetPositionDef(), TEXT("Particles.Position"));
            UEdGraphPin* SetVelocity    = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetVec3Def(),     TEXT("Particles.Velocity"));
            UEdGraphPin* SetColor       = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetColorDef(),    TEXT("Particles.Color"));
            UEdGraphPin* SetSize        = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetVec2Def(),     TEXT("Particles.SpriteSize"));
            UEdGraphPin* SetScale       = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetVec3Def(),     TEXT("Particles.Scale"));
            UEdGraphPin* SetOrientation = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetQuatDef(),     TEXT("Particles.MeshOrientation"));
            UEdGraphPin* SetDynamic     = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetVec4Def(),     TEXT("Particles.DynamicMaterialParameter"));
            UEdGraphPin* SetRotation    = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetFloatDef(),    TEXT("Particles.SpriteRotation"));
            UEdGraphPin* SetMeshIndex   = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetIntDef(),      TEXT("Particles.MeshIndex"));
            UEdGraphPin* SetVisTag      = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetIntDef(),      TEXT("Particles.VisibilityTag"));
            // Both attributes must EXIST for the custom-facing renderer: a missing Particles.SpriteAlignment
            // makes CustomAlignment silently fall back to Unaligned (NiagaraSpriteRendererProperties.h).
            UEdGraphPin* SetSpriteAlign = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetVec3Def(),     TEXT("Particles.SpriteAlignment"));
            UEdGraphPin* SetSpriteFacing= MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetVec3Def(),     TEXT("Particles.SpriteFacing"));
            // Niagara's SubImageIndexBinding reads this attribute by default, so writing it is the whole
            // flipbook contract on the emitter side; the renderer's SubImageSize decides how it is divided.
            UEdGraphPin* SetSubImage    = MapSet->RequestNewTypedPin(EGPD_Input, FNiagaraTypeDefinition::GetFloatDef(),    TEXT("Particles.SubImageIndex"));

            const auto Wire = [Schema](UEdGraphPin* InFrom, UEdGraphPin* InTo)
            {
                if (InFrom != nullptr && InTo != nullptr)
                { Schema->TryCreateConnection(InFrom, InTo); }
            };

            // ---- Thread the parameter map (matches Engine's NiagaraScriptFactory module skeleton) ----
            // The Input map FANS OUT to both Map Get's and Map Set's Source pins; it threads to Output *through
            // Map Set*. Map Get has NO map output pin, so wiring MapGet-output -> MapSet-Source connects nothing
            // and severs the chain.
            Wire(InputNode->GetOutputPin(0),                        Find_ParameterMapPin(MapGet, EGPD_Input,  Schema));
            Wire(InputNode->GetOutputPin(0),                        Find_ParameterMapPin(MapSet, EGPD_Input,  Schema));
            Wire(Find_ParameterMapPin(MapSet, EGPD_Output, Schema), OutputNode->GetInputPin(0));

            // ---- Wire Map Get reads -> ExecuteStage inputs (by signature pin name) ----
            Wire(GetScript,   Find_PinByName(FuncNode, TEXT("ParticleScript"), EGPD_Input));
            Wire(GetBehavior, Find_PinByName(FuncNode, TEXT("BehaviorId"),     EGPD_Input));
            Wire(GetDelta,    Find_PinByName(FuncNode, TEXT("DeltaTime"),      EGPD_Input));
            Wire(GetAge,      Find_PinByName(FuncNode, TEXT("Age"),            EGPD_Input));
            Wire(GetLifetime, Find_PinByName(FuncNode, TEXT("Lifetime"),       EGPD_Input));
            Wire(GetPosition, Find_PinByName(FuncNode, TEXT("Position"),       EGPD_Input));
            Wire(GetVelocity, Find_PinByName(FuncNode, TEXT("Velocity"),       EGPD_Input));

            auto* SeedSource = GetSeed;
            if (InSeedBank != 0)
            {
                auto* SeedBankAdd = Create_SeedBankAddNode(Graph, Schema, InSeedBank);
                if (SeedBankAdd == nullptr)
                { return nullptr; }

                Wire(GetSeed, Find_PinByName(SeedBankAdd, TEXT("A"), EGPD_Input));
                SeedSource = Find_PinByName(SeedBankAdd, TEXT("Result"), EGPD_Output);
            }

            Wire(SeedSource,    Find_PinByName(FuncNode, TEXT("Seed"),         EGPD_Input));
            Wire(GetEmitterAge, Find_PinByName(FuncNode, TEXT("EmitterAge"),   EGPD_Input));

            // ---- Wire ExecuteStage outputs -> Map Set writes ----
            Wire(Find_PinByName(FuncNode, TEXT("OutPosition"),    EGPD_Output), SetPosition);
            Wire(Find_PinByName(FuncNode, TEXT("OutVelocity"),    EGPD_Output), SetVelocity);
            Wire(Find_PinByName(FuncNode, TEXT("OutColor"),       EGPD_Output), SetColor);
            Wire(Find_PinByName(FuncNode, TEXT("OutSize"),        EGPD_Output), SetSize);
            Wire(Find_PinByName(FuncNode, TEXT("OutScale"),       EGPD_Output), SetScale);
            Wire(Find_PinByName(FuncNode, TEXT("OutOrientation"), EGPD_Output), SetOrientation);
            Wire(Find_PinByName(FuncNode, TEXT("OutDynamic"),     EGPD_Output), SetDynamic);
            Wire(Find_PinByName(FuncNode, TEXT("OutRotation"),    EGPD_Output), SetRotation);
            Wire(Find_PinByName(FuncNode, TEXT("OutMeshIndex"),   EGPD_Output), SetMeshIndex);
            Wire(Find_PinByName(FuncNode, TEXT("OutVisTag"),      EGPD_Output), SetVisTag);
            Wire(Find_PinByName(FuncNode, TEXT("OutSpriteAlignment"), EGPD_Output), SetSpriteAlign);
            Wire(Find_PinByName(FuncNode, TEXT("OutSpriteFacing"),    EGPD_Output), SetSpriteFacing);
            Wire(Find_PinByName(FuncNode, TEXT("OutSubImageIndex"),   EGPD_Output), SetSubImage);

            Graph->NotifyGraphChanged();
            Script->SetLatestSource(Source);
            return Script;
        }

        static auto Try_AddCodeBuiltBehaviorModule(
            UNiagaraSystem* InSystem,
            int32           InEmitterHandleIndex,
            const TCHAR*    InScriptName,
            int32           InSeedBank) -> bool
        {
            if (InSystem->GetEmitterHandles().Num() <= InEmitterHandleIndex)
            { return false; }

            const auto& Handle = InSystem->GetEmitterHandle(InEmitterHandleIndex);
            auto* EmitterData = Handle.GetEmitterData();
            if (EmitterData == nullptr)
            { return false; }

            UNiagaraScript* UpdateScript = EmitterData->UpdateScriptProps.Script;
            if (UpdateScript == nullptr)
            { return false; }

            auto* UpdateSource = Cast<UNiagaraScriptSource>(UpdateScript->GetLatestSource());
            if (UpdateSource == nullptr || UpdateSource->NodeGraph == nullptr)
            { return false; }

            auto* OutputNode = UpdateSource->NodeGraph->FindEquivalentOutputNode(
                ENiagaraScriptUsage::ParticleUpdateScript, UpdateScript->GetUsageId());
            if (OutputNode == nullptr)
            { return false; }

            auto* ModuleScript = Build_BehaviorModuleScript(InSystem, InScriptName, InSeedBank);
            if (ModuleScript == nullptr)
            { return false; }

            auto* AddedModule = FNiagaraStackGraphUtilities::AddScriptModuleToStack(
                ModuleScript, *OutputNode, INDEX_NONE, TEXT("CkParticles Apply Behavior"));
            return AddedModule != nullptr;
        }
#endif // CK_WITH_PARTICLES

        // ---- Imported source art -------------------------------------------------------------------------------
        //
        // A recreation may need leaf ART (textures/meshes) it cannot generate procedurally. Those are COPIED once
        // into plugin-owned content so the shipped effect never references the source pack; materials are never
        // copied — they are recreated as CkUsf looks.
        //
        // This runs only when the source pack happens to be mounted (a dev host with the pack installed). On any
        // other machine the copies already exist as committed plugin content and this is a no-op, which is exactly
        // why the runtime has no dependency on the source pack.
        struct FImportedTexture { const TCHAR* SourceObjectPath; const TCHAR* DestPackagePath; const TCHAR* DestAssetName; };

        static auto Get_ImportedTextures() -> TArrayView<const FImportedTexture>
        {
            static const FImportedTexture Textures[] =
            {
                // NS_Lightning_Range / M_VFX_DisAdd_Ring04: Main_Tex + Color_Tex (the ring shape).
                { TEXT("/Game/Vefects/Anime_VFX/Shared/Textures/T_VFX_Ring_04.T_VFX_Ring_04"),
                  TEXT("/CkFoundation/CkParticles/Imported/Vefects/NS_Lightning_Range/T_VFX_Ring_04"),
                  TEXT("T_VFX_Ring_04") },
                // ... and Dissolve_Tex (the erosion noise). Distortion_Tex is the same asset but
                // Distortion_Intensity resolves to 0 on this instance, so it is imported once, for the dissolve.
                { TEXT("/Game/Vefects/Anime_VFX/Shared/Textures/T_VFX_Noise_04.T_VFX_Noise_04"),
                  TEXT("/CkFoundation/CkParticles/Imported/Vefects/NS_Lightning_Range/T_VFX_Noise_04"),
                  TEXT("T_VFX_Noise_04") },
            };
            return MakeArrayView(Textures);
        }

        static auto Import_SourceTextures() -> void
        {
            for (const auto& Import : Get_ImportedTextures())
            {
                if (FPackageName::DoesPackageExist(Import.DestPackagePath))
                {
                    ck::particles_editor::Log(TEXT("Imported texture [{}] already present — leaving it alone"),
                        FString(Import.DestAssetName));
                    continue;
                }

                auto* Source = LoadObject<UTexture2D>(nullptr, Import.SourceObjectPath);
                if (Source == nullptr)
                {
                    ck::particles_editor::Log(TEXT("Import source [{}] not mounted — skipping (expected off a dev host)"),
                        FString(Import.SourceObjectPath));
                    continue;
                }

                auto* Package = CreatePackage(Import.DestPackagePath);
                auto* Copy = Cast<UTexture2D>(StaticDuplicateObject(Source, Package, FName(Import.DestAssetName)));
                if (Copy == nullptr)
                {
                    ck::particles_editor::Log(TEXT("Import of [{}] failed to duplicate"), FString(Import.DestAssetName));
                    continue;
                }

                Copy->SetFlags(RF_Public | RF_Standalone);
                Copy->MarkPackageDirty();
                FAssetRegistryModule::AssetCreated(Copy);

                const auto FileName = FPackageName::LongPackageNameToFilename(
                    Import.DestPackagePath, FPackageName::GetAssetPackageExtension());
                FSavePackageArgs SaveArgs;
                SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
                UPackage::SavePackage(Package, Copy, *FileName, SaveArgs);

                ck::particles_editor::Log(TEXT("Imported texture [{}] -> [{}]"),
                    FString(Import.SourceObjectPath), FString(Import.DestPackagePath));
            }
        }

        // Names a declared spawn stack; "factory rate" is the one this builder does not emit itself. Takes the two
        // cadence numbers rather than a row, because a ribbon emitter declares the same pair on its own spec.
        static auto Get_SpawnCadenceLabel(
            int32 InBurstCount,
            float InSpawnRate) -> FString
        {
            const auto HasBurst = InBurstCount > 0;
            const auto HasRate  = InSpawnRate > 0.0f;

            if (HasBurst && HasRate) { return FString::Printf(TEXT("burst %d + rate %g/s"), InBurstCount, InSpawnRate); }
            if (HasBurst)            { return FString::Printf(TEXT("burst %d"), InBurstCount); }
            if (HasRate)             { return FString::Printf(TEXT("rate %g/s"), InSpawnRate); }
            return TEXT("factory rate");
        }

        // The emitter shape every template emitter shares, attached to the system. Returns its handle index, or
        // INDEX_NONE if the attach failed.
        //
        // The editor utility, not the raw runtime UNiagaraSystem::AddEmitterHandle: it rebuilds the system-script
        // emitter nodes AND creates the System Overview node, so the emitter is wired and visible.
        static auto Add_TemplateEmitter(
            UNiagaraSystem* InSystem,
            const TCHAR*    InEmitterName,
            bool            InAddFactoryDefaults) -> int32
        {
            auto* Emitter = NewObject<UNiagaraEmitter>(GetTransientPackage(), InEmitterName, RF_Transactional);
            UNiagaraEmitterFactoryNew::InitializeEmitter(Emitter, InAddFactoryDefaults);

            if (auto* EmitterData = Emitter->GetLatestEmitterData())
            {
                EmitterData->SimTarget = ENiagaraSimTarget::GPUComputeSim;

                // LOCAL space: behaviors write absolute positions, so in world space every spawned system would
                // collapse onto the world origin instead of rendering where it was spawned.
                EmitterData->bLocalSpace = true;

                // GPU emitters don't auto-compute bounds cheaply; without generous fixed bounds the whole system is
                // frustum-culled when its (tiny default) box leaves view.
                EmitterData->CalculateBoundsMode = ENiagaraEmitterCalculateBoundMode::Fixed;
                EmitterData->FixedBounds = FBox(FVector(-3000.0), FVector(3000.0));
            }

            const auto HandleCountBefore = InSystem->GetEmitterHandles().Num();

            constexpr auto CreateCopy = true;
            FNiagaraEditorUtilities::AddEmitterToSystem(*InSystem, *Emitter, FGuid(), CreateCopy);

            return InSystem->GetEmitterHandles().Num() > HandleCountBefore ? HandleCountBefore : INDEX_NONE;
        }

        // The row's SECOND emitter — the ribbon population. It shares the template's clock (loop duration and
        // particle lifetime come from the row) and the system's User parameters with the main emitter; what differs
        // is its spawn stack, its renderer set, and the seed bank its behavior call adds.
        static auto Add_RibbonEmitter(
            UNiagaraSystem*                                 InSystem,
            const ck::particles::FCk_ParticlesTemplateSpec& InSpec) -> int32
        {
            const auto& RibbonSpec = InSpec.RibbonEmitter;

            const auto DeclaresOwnSpawn = RibbonSpec.BurstCount > 0 || RibbonSpec.SpawnRate > 0.0f;
            if (NOT DeclaresOwnSpawn)
            {
                ck::particles_editor::Error(TEXT("Template builder: row [{}] declares a ribbon emitter with neither a "
                    "burst nor a rate — it would spawn nothing and the trail would never appear"), FString(InSpec.AssetName));
                return INDEX_NONE;
            }

            // The ribbon emitter's cadence IS the row's, with the ribbon spec's spawn stack swapped in and the main
            // emitter's renderers dropped — so the one stack builder serves both emitters.
            auto RibbonRowSpec = InSpec;
            RibbonRowSpec.RendererOverrides = {};
            RibbonRowSpec.BurstCount        = RibbonSpec.BurstCount;
            RibbonRowSpec.SpawnRate         = RibbonSpec.SpawnRate;

            constexpr auto AddFactoryDefaults = false;
            const auto EmitterIndex = Add_TemplateEmitter(InSystem, TEXT("CkParticlesRibbon"), AddFactoryDefaults);
            if (EmitterIndex == INDEX_NONE)
            {
                ck::particles_editor::Log(TEXT("Template builder [{}]: ribbon emitter failed to attach"), FString(InSpec.AssetName));
                return INDEX_NONE;
            }

            const auto& Handle = InSystem->GetEmitterHandle(EmitterIndex);
            auto* RibbonEmitter     = Handle.GetInstance().Emitter.Get();
            auto* RibbonEmitterData = Handle.GetEmitterData();
            if (RibbonEmitter == nullptr || RibbonEmitterData == nullptr)
            { return INDEX_NONE; }

            Configure_RibbonRenderers(RibbonEmitter, Handle.GetInstance().Version, RibbonSpec);

            if (NOT Add_SpawnEmitterStack(RibbonEmitter, RibbonEmitterData, RibbonRowSpec))
            {
                ck::particles_editor::Log(TEXT("Template builder [{}]: ribbon spawn emitter stack failed"), FString(InSpec.AssetName));
                return INDEX_NONE;
            }

            return EmitterIndex;
        }

        // ---- One template system (declared burst/rate cadence, or the factory's own) ----------------------------
        static auto DoBuild_OneTemplateSystem(
            const ck::particles::FCk_ParticlesTemplateSpec& InSpec,
            UMaterialInterface* InSpriteMaterial) -> UNiagaraSystem*
        {
            const auto* AssetName = InSpec.AssetName;
            const auto  PkgPathStr    = FString::Printf(TEXT("/CkFoundation/CkParticles/Templates/%s"), AssetName);
            const auto* PkgPath   = *PkgPathStr;
            const auto  DeclaresOwnSpawn = InSpec.BurstCount > 0 || InSpec.SpawnRate > 0.0f;

            // ---- Package (idempotent refresh) ----
            UPackage* Package = FPackageName::DoesPackageExist(PkgPath)
                ? LoadPackage(nullptr, PkgPath, LOAD_None)
                : nullptr;
            if (Package == nullptr) { Package = CreatePackage(PkgPath); }

            if (auto* Old = StaticFindObject(UNiagaraSystem::StaticClass(), Package, AssetName))
            {
                Old->ClearFlags(RF_Standalone | RF_Public);
                Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
            }

            // ---- System + GPU emitter ----
            // No declared cadence: factory defaults (spawn-rate 10 + init modules + a sprite renderer we then retag).
            // Declared cadence: an EMPTY stack (output nodes only) so the factory's own SpawnRate never survives
            // underneath the row's burst and/or rate modules.
            constexpr auto CreateDefaultNodes = true;
            auto* System = NewObject<UNiagaraSystem>(Package, AssetName, RF_Public | RF_Standalone);
            UNiagaraSystemFactoryNew::InitializeSystem(System, CreateDefaultNodes);

            const auto AddDefaultModulesAndRenderers = NOT DeclaresOwnSpawn;
            const auto MainEmitterIndex = Add_TemplateEmitter(System, TEXT("CkParticles"), AddDefaultModulesAndRenderers);

            if (MainEmitterIndex == INDEX_NONE)
            {
                ck::particles_editor::Log(TEXT("Template builder [{}]: emitter failed to attach"), FString(AssetName));
                return nullptr;
            }

            // Everything below configures the SYSTEM's copy of the emitter (renderers, burst modules, RIP values),
            // so the unique-emitter-name-scoped rapid-iteration constants land on the scripts that actually compile.
            const auto& Handle = System->GetEmitterHandle(MainEmitterIndex);
            auto* SystemEmitter = Handle.GetInstance().Emitter.Get();
            auto* SystemEmitterData = Handle.GetEmitterData();
            if (SystemEmitter == nullptr || SystemEmitterData == nullptr)
            { return nullptr; }

            Configure_Renderers(SystemEmitter, Handle.GetInstance().Version, SystemEmitterData, InSpriteMaterial);
            Configure_RowRenderers(SystemEmitter, Handle.GetInstance().Version, InSpec);

            if (DeclaresOwnSpawn)
            {
                if (NOT Add_SpawnEmitterStack(SystemEmitter, SystemEmitterData, InSpec))
                {
                    ck::particles_editor::Log(TEXT("Template builder [{}]: spawn emitter stack failed"), FString(AssetName));
                    return nullptr;
                }
            }

            // ---- The row's ribbon emitter, if it declares one. Added after the main emitter is fully configured:
            // a handle reference is into the system's own array, which attaching another emitter may move. ----
            const auto RibbonEmitterIndex = InSpec.RibbonEmitter.Get_IsDeclared()
                ? Add_RibbonEmitter(System, InSpec)
                : INDEX_NONE;

            if (InSpec.RibbonEmitter.Get_IsDeclared() && RibbonEmitterIndex == INDEX_NONE)
            { return nullptr; }

            // ---- User parameters: BehaviorId int + the DI as ParticleScript + the swappable sprite material ----
            // Both emitters read these: the ribbon population runs the same behavior id and the same DI instance.
            auto& Exposed = System->GetExposedParameters();

            const FNiagaraVariable BehaviorVar(FNiagaraTypeDefinition::GetIntDef(), TEXT("User.BehaviorId"));
            Exposed.AddParameter(BehaviorVar);
            constexpr auto AddIfMissing = true;
            Exposed.SetParameterValue<int32>(0, BehaviorVar, AddIfMissing);

            const FNiagaraVariable DiVar(FNiagaraTypeDefinition(UCkParticles_DataInterface::StaticClass()), TEXT("User.ParticleScript"));
            Exposed.AddParameter(DiVar);
            Exposed.SetDataInterface(NewObject<UCkParticles_DataInterface>(System), DiVar);

            const FNiagaraVariable SpriteMatVar(FNiagaraTypeDefinition::GetUMaterialDef(), ck::particles::Get_SpriteMaterialParameterName());
            Exposed.AddParameter(SpriteMatVar);
            if (InSpriteMaterial != nullptr) { Exposed.SetUObject(InSpriteMaterial, SpriteMatVar); }

            // ---- Code-built behavior module (forked engine only; inert on stock via CK_WITH_PARTICLES) ----
#if CK_WITH_PARTICLES
            constexpr auto MainSeedBank = 0;
            const auto bModuleAdded = Try_AddCodeBuiltBehaviorModule(
                System, MainEmitterIndex, TEXT("CkParticles_ApplyBehavior_Module"), MainSeedBank);
            ck::particles_editor::Log(TEXT("[{}] Code-built behavior module added to Particle Update: {}"),
                FString(AssetName), bModuleAdded ? FString(TEXT("YES")) : FString(TEXT("NO")));

            if (RibbonEmitterIndex != INDEX_NONE)
            {
                const auto bRibbonModuleAdded = Try_AddCodeBuiltBehaviorModule(
                    System, RibbonEmitterIndex, TEXT("CkParticles_ApplyBehavior_Module_Ribbon"),
                    ck::particles::RibbonSeedBase);
                ck::particles_editor::Log(TEXT("[{}] Code-built behavior module added to the ribbon emitter: {}"),
                    FString(AssetName), bRibbonModuleAdded ? FString(TEXT("YES")) : FString(TEXT("NO")));
            }
#endif

            // ---- Compile (incl. GPU) + save ----
            constexpr auto ForceCompile = true;
            System->RequestCompile(ForceCompile);
            constexpr auto IncludingGpuShaders = true;
            constexpr auto ShowProgress = false;
            System->WaitForCompilationComplete(IncludingGpuShaders, ShowProgress);

            System->MarkPackageDirty();
            FAssetRegistryModule::AssetCreated(System);

            const auto FileName = FPackageName::LongPackageNameToFilename(PkgPath, FPackageName::GetAssetPackageExtension());
            FSavePackageArgs SaveArgs;
            SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
            UPackage::SavePackage(Package, System, *FileName, SaveArgs);

            ck::particles_editor::Log(TEXT("Built template [{}] ({}{})"),
                FString(AssetName), Get_SpawnCadenceLabel(InSpec.BurstCount, InSpec.SpawnRate),
                RibbonEmitterIndex == INDEX_NONE
                    ? FString()
                    : FString::Printf(TEXT("; ribbon emitter %s over %d renderer(s)"),
                        *Get_SpawnCadenceLabel(InSpec.RibbonEmitter.BurstCount, InSpec.RibbonEmitter.SpawnRate),
                        InSpec.RibbonEmitter.Renderers.Num()));
            return System;
        }
    }

    auto Build_AllTemplateSystems() -> bool
    {
        using namespace TemplateBuilderLocal;

#if !CK_WITH_PARTICLES
        // Without the fork's NiagaraEditor pin-authoring exports there is no behavior-call module, so every
        // template this would write is INERT: the DI is never invoked and no behavior renders. The assets still
        // save cleanly and still load, so the failure is invisible to any test that only checks existence —
        // which is exactly why this refuses instead of proceeding. Regenerate on a fork-enabled engine.
        ck::particles_editor::Error(TEXT("Refusing to build CkParticles templates: CK_WITH_PARTICLES=0 (the engine "
            "is missing NiagaraEditor/Public/CkNiagaraAuthoring.h). Templates built here would silently render "
            "nothing. Regenerate on an engine with the Chainkemists Niagara authoring exports."));
        return false;
#else

        // Order matters: textures -> materials (sample them) -> meshes (slot them) -> templates (reference all three).
        Generate_AllVfxTextures();
        Import_SourceTextures();

        auto* BaseTex = LoadObject<UTexture2D>(nullptr,
            TEXT("/CkFoundation/CkParticles/Textures/T_CkParticles_Glow.T_CkParticles_Glow"));
        auto* VfxMaterial = Build_VfxMasterMaterial(BaseTex);
        Build_TextureMaterialInstances(VfxMaterial);

        Generate_AllVfxMaterials();
        Generate_AllVfxMeshes();

        // One template per cadence row — adding a recreation with a new cadence is a row in
        // ck::particles::Get_TemplateSpecs(), not another hand-maintained build call here.
        auto AllBuilt = true;
        for (const auto& Spec : ck::particles::Get_TemplateSpecs())
        {
            if (DoBuild_OneTemplateSystem(Spec, VfxMaterial) == nullptr)
            {
                ck::particles_editor::Log(TEXT("Template [{}] failed to build"), FString(Spec.AssetName));
                AllBuilt = false;
            }
        }

        return AllBuilt;
#endif
    }

    auto Build_TemplateSystem() -> UNiagaraSystem*
    {
        Build_AllTemplateSystems();
        return LoadObject<UNiagaraSystem>(nullptr, *ck::particles::Get_DefaultTemplateSystemObjectPath());
    }
}

// --------------------------------------------------------------------------------------------------------------------
