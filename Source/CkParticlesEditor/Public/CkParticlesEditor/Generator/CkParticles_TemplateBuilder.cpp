#include "CkParticlesEditor/Generator/CkParticles_TemplateBuilder.h"

#include "CkParticlesEditor_Log.h"

#include "CkParticles/DataInterface/CkParticles_DataInterface.h"

#include "CkCore/Validation/CkIsValid.h"

#include "NiagaraSystem.h"
#include "NiagaraEmitter.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSystemFactoryNew.h"
#include "NiagaraEmitterFactoryNew.h"
#include "NiagaraTypes.h"
#include "NiagaraEditorUtilities.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

// --------------------------------------------------------------------------------------------------------------------
// Builds the SCAFFOLD of the template Niagara System from C++: a GPU emitter (default spawn/init modules + sprite
// renderer), a User.BehaviorId int, and the DI wired as a User.ParticleScript parameter.
//
// The actual behavior-call MODULE (Particle Update calling ParticleScript.ExecuteStage and writing
// Position/Velocity/Color) is NOT built here: it requires NiagaraEditor internals that Epic does not export
// (FNiagaraStackGraphUtilities::SetCustomExpressionForFunctionInput, UNiagaraGraph::FindOutputNode,
// UNiagaraNodeCustomHlsl::VirtualIncludeFilePaths are all non-public to external modules). That one module is
// added once in-editor (see CkParticles/Claude.md). The DI param is already in place so it's a 1-field hookup.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::particles_editor
{
    namespace TemplateBuilderLocal
    {
        static const TCHAR* PkgPath   = TEXT("/CkFoundation/CkParticles/Templates/PS_CkParticles_Template");
        static const TCHAR* AssetName = TEXT("PS_CkParticles_Template");
    }

    auto Build_TemplateSystem() -> UNiagaraSystem*
    {
        using namespace TemplateBuilderLocal;

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

        // ---- System + GPU emitter (default modules + sprite renderer come from InitializeEmitter) ----
        auto* System = NewObject<UNiagaraSystem>(Package, AssetName, RF_Public | RF_Standalone);
        UNiagaraSystemFactoryNew::InitializeSystem(System, /*bCreateDefaultNodes*/ true);

        auto* Emitter = NewObject<UNiagaraEmitter>(GetTransientPackage(), TEXT("CkParticles"), RF_Transactional);
        UNiagaraEmitterFactoryNew::InitializeEmitter(Emitter, /*bAddDefaultModulesAndRenderers*/ true);
        if (auto* EmitterData = Emitter->GetLatestEmitterData())
        {
            EmitterData->SimTarget = ENiagaraSimTarget::GPUComputeSim;
        }

        // Use the editor utility (exported) instead of the raw runtime UNiagaraSystem::AddEmitterHandle. The raw
        // call only appends to the EmitterHandles array; it does NOT build the system-script emitter nodes
        // (RebuildEmitterNodes -> simulation) or create the System Overview node
        // (SynchronizeOverviewGraphWithSystem -> visible emitter track). AddEmitterToSystem does all three, so the
        // emitter is both wired and visible. It copies the emitter into the System (bCreateCopy) and derives the
        // track name from the emitter's FName ("CkParticles").
        FNiagaraEditorUtilities::AddEmitterToSystem(*System, *Emitter, FGuid(), /*bCreateCopy*/ true);

        // ---- User parameters: BehaviorId int (generator patches it) + the DI as ParticleScript ----
        auto& Exposed = System->GetExposedParameters();

        const FNiagaraVariable BehaviorVar(FNiagaraTypeDefinition::GetIntDef(), TEXT("User.BehaviorId"));
        Exposed.AddParameter(BehaviorVar);
        constexpr auto AddIfMissing = true;
        Exposed.SetParameterValue<int32>(0, BehaviorVar, AddIfMissing);

        const FNiagaraVariable DiVar(FNiagaraTypeDefinition(UCkParticles_DataInterface::StaticClass()), TEXT("User.ParticleScript"));
        Exposed.AddParameter(DiVar);
        Exposed.SetDataInterface(NewObject<UCkParticles_DataInterface>(System), DiVar);

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

        ck::particles_editor::Log(
            TEXT("Built template SCAFFOLD [{}] (GPU emitter + sprite renderer + User.BehaviorId + User.ParticleScript). "
                 "Add the ExecuteStage call in Particle Update to finish it (see CkParticles/Claude.md)."),
            FString(AssetName));
        return System;
    }
}

// --------------------------------------------------------------------------------------------------------------------
