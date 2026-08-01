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
#include "NiagaraNodeWithDynamicPins.h"
#include "NiagaraDataInterface.h"

#include "NiagaraSpriteRendererProperties.h"
#include "NiagaraMeshRendererProperties.h"

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

        // ---- Burst emitter stack (built from an empty emitter so no continuous SpawnRate is left behind) -------
        static auto Add_BurstEmitterStack(
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

        // A Module-usage script whose graph is Input -> Map Get (reads DI + params) -> ExecuteStage (DI member fn)
        // -> Map Set (Particles.*) -> Output. The Particles.Position (LWC) vs DI Vec3 difference is auto-bridged
        // by TryCreateConnection.
        static auto Build_BehaviorModuleScript(
            UObject* InOuter) -> UNiagaraScript*
        {
            auto* Script = NewObject<UNiagaraScript>(InOuter, TEXT("CkParticles_ApplyBehavior_Module"), RF_Transactional);
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
            Wire(GetSeed,     Find_PinByName(FuncNode, TEXT("Seed"),           EGPD_Input));

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

            Graph->NotifyGraphChanged();
            Script->SetLatestSource(Source);
            return Script;
        }

        static auto Try_AddCodeBuiltBehaviorModule(
            UNiagaraSystem* InSystem) -> bool
        {
            if (InSystem->GetEmitterHandles().Num() == 0)
            { return false; }

            const auto& Handle = InSystem->GetEmitterHandle(0);
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

            auto* ModuleScript = Build_BehaviorModuleScript(InSystem);
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

        // ---- One template system (continuous or burst) ---------------------------------------------------------
        static auto DoBuild_OneTemplateSystem(
            const ck::particles::FCk_ParticlesTemplateSpec& InSpec,
            UMaterialInterface* InSpriteMaterial) -> UNiagaraSystem*
        {
            const auto* AssetName = InSpec.AssetName;
            const auto  PkgPathStr    = FString::Printf(TEXT("/CkFoundation/CkParticles/Templates/%s"), AssetName);
            const auto* PkgPath   = *PkgPathStr;
            const auto  UsesBurstSpawn = InSpec.BurstCount > 0;

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
            // Continuous: factory defaults (spawn-rate 10 + init modules + a sprite renderer we then retag).
            // Burst: an EMPTY stack (output nodes only) so no continuous SpawnRate survives.
            constexpr auto CreateDefaultNodes = true;
            auto* System = NewObject<UNiagaraSystem>(Package, AssetName, RF_Public | RF_Standalone);
            UNiagaraSystemFactoryNew::InitializeSystem(System, CreateDefaultNodes);

            const auto AddDefaultModulesAndRenderers = NOT UsesBurstSpawn;
            auto* Emitter = NewObject<UNiagaraEmitter>(GetTransientPackage(), TEXT("CkParticles"), RF_Transactional);
            UNiagaraEmitterFactoryNew::InitializeEmitter(Emitter, AddDefaultModulesAndRenderers);

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

            // The editor utility, not the raw runtime UNiagaraSystem::AddEmitterHandle: it rebuilds the
            // system-script emitter nodes AND creates the System Overview node, so the emitter is wired and visible.
            constexpr auto CreateCopy = true;
            FNiagaraEditorUtilities::AddEmitterToSystem(*System, *Emitter, FGuid(), CreateCopy);

            if (System->GetEmitterHandles().Num() == 0)
            {
                ck::particles_editor::Log(TEXT("Template builder [{}]: emitter failed to attach"), FString(AssetName));
                return nullptr;
            }

            // Everything below configures the SYSTEM's copy of the emitter (renderers, burst modules, RIP values),
            // so the unique-emitter-name-scoped rapid-iteration constants land on the scripts that actually compile.
            const auto& Handle = System->GetEmitterHandle(0);
            auto* SystemEmitter = Handle.GetInstance().Emitter.Get();
            auto* SystemEmitterData = Handle.GetEmitterData();
            if (SystemEmitter == nullptr || SystemEmitterData == nullptr)
            { return nullptr; }

            Configure_Renderers(SystemEmitter, Handle.GetInstance().Version, SystemEmitterData, InSpriteMaterial);

            if (UsesBurstSpawn)
            {
                if (NOT Add_BurstEmitterStack(SystemEmitter, SystemEmitterData, InSpec))
                {
                    ck::particles_editor::Log(TEXT("Template builder [{}]: burst emitter stack failed"), FString(AssetName));
                    return nullptr;
                }
            }

            // ---- User parameters: BehaviorId int + the DI as ParticleScript + the swappable sprite material ----
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
            const auto bModuleAdded = Try_AddCodeBuiltBehaviorModule(System);
            ck::particles_editor::Log(TEXT("[{}] Code-built behavior module added to Particle Update: {}"),
                FString(AssetName), bModuleAdded ? FString(TEXT("YES")) : FString(TEXT("NO")));
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

            ck::particles_editor::Log(TEXT("Built template [{}] ({})"),
                FString(AssetName), UsesBurstSpawn ? FString(TEXT("burst-spawn")) : FString(TEXT("continuous-rate")));
            return System;
        }
    }

    auto Build_AllTemplateSystems() -> bool
    {
        using namespace TemplateBuilderLocal;

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
    }

    auto Build_TemplateSystem() -> UNiagaraSystem*
    {
        Build_AllTemplateSystems();
        return LoadObject<UNiagaraSystem>(nullptr, *ck::particles::Get_DefaultTemplateSystemObjectPath());
    }
}

// --------------------------------------------------------------------------------------------------------------------
