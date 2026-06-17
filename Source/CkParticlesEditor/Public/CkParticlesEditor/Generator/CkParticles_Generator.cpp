#include "CkParticlesEditor/Generator/CkParticles_Generator.h"

#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition.h"
#include "CkParticles/ScriptDefinition/CkParticles_ScriptDefinition_Naming.h"
#include "CkParticlesEditor_Log.h"

#include "CkCore/Validation/CkIsValid.h"

#include "NiagaraSystem.h"
#include "NiagaraTypes.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Modules/ModuleManager.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "UObject/UObjectGlobals.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::particles_editor
{
    auto Generate_ParticleSystem(UCkParticles_ScriptDefinition* InDef) -> UNiagaraSystem*
    {
        if (ck::Is_NOT_Valid(InDef, ck::IsValid_Policy_NullptrOnly{}))
        {
            ck::particles_editor::Warning(TEXT("Null ScriptDefinition"));
            return nullptr;
        }

        const auto ScriptName = InDef->Get_EffectiveScriptName();

        // ---- Resolve the template system to duplicate ----
        UNiagaraSystem* Template = InDef->_TemplateSystem.LoadSynchronous();
        if (ck::Is_NOT_Valid(Template, ck::IsValid_Policy_NullptrOnly{}))
        {
            Template = LoadObject<UNiagaraSystem>(nullptr, *ck::particles::Get_DefaultTemplateSystemObjectPath());
        }
        if (ck::Is_NOT_Valid(Template, ck::IsValid_Policy_NullptrOnly{}))
        {
            ck::particles_editor::Warning(
                TEXT("ScriptDefinition [{}] has no template and the default template [{}] was not found. "
                     "Create the seed template first (see seed-template setup notes)."),
                ScriptName, ck::particles::Get_DefaultTemplateSystemObjectPath());
            return nullptr;
        }

        // ---- Create package + duplicate (idempotent refresh, mirrors CkUsf generator) ----
        const auto PkgPath = ck::particles::Get_GeneratedSystemPackagePath(ScriptName);
        const auto AssetName = FString::Printf(TEXT("PS_CkParticles_%s"), *ScriptName.ToString());

        UPackage* Package = FPackageName::DoesPackageExist(PkgPath)
            ? LoadPackage(nullptr, *PkgPath, LOAD_None)
            : nullptr;
        if (Package == nullptr) { Package = CreatePackage(*PkgPath); }

        if (auto* Old = StaticFindObject(UNiagaraSystem::StaticClass(), Package, *AssetName))
        {
            Old->ClearFlags(RF_Standalone | RF_Public);
            Old->Rename(nullptr, GetTransientPackage(),
                REN_DontCreateRedirectors | REN_NonTransactional);
        }

        auto* System = Cast<UNiagaraSystem>(
            StaticDuplicateObject(Template, Package, *AssetName, RF_Public | RF_Standalone));
        if (ck::Is_NOT_Valid(System, ck::IsValid_Policy_NullptrOnly{}))
        {
            ck::particles_editor::Error(TEXT("Failed to duplicate template for [{}]"), ScriptName);
            return nullptr;
        }
        System->SetFlags(RF_Public | RF_Standalone);

        // ---- Patch the BehaviorId user parameter on the duplicated system ----
        // Exposed/User parameters are addressed with the "User." prefix in the parameter store, and
        // SetParameterValue matches by exact name. Accept either the bare ("BehaviorId") or fully-qualified
        // ("User.BehaviorId") form on the ScriptDefinition by normalising to the prefixed name here.
        auto& ExposedParams = System->GetExposedParameters();
        auto BehaviorParamName = InDef->_BehaviorIdParameterName.ToString();
        if (NOT BehaviorParamName.StartsWith(TEXT("User.")))
        { BehaviorParamName = FString(TEXT("User.")) + BehaviorParamName; }

        const FNiagaraVariable BehaviorVar(FNiagaraTypeDefinition::GetIntDef(), FName(*BehaviorParamName));
        constexpr auto DontAddIfMissing = false;
        const auto bSet = ExposedParams.SetParameterValue<int32>(InDef->_BehaviorId, BehaviorVar, DontAddIfMissing);
        if (NOT bSet)
        {
            ck::particles_editor::Warning(
                TEXT("Template for [{}] has no int User parameter named [{}] — BehaviorId {} NOT applied. "
                     "Add that int User parameter to the template system."),
                ScriptName, BehaviorParamName, InDef->_BehaviorId);
        }

        // ---- Recompile (incl. GPU shaders) so the patched system is ready, then save ----
        constexpr auto ForceCompile = true;
        System->RequestCompile(ForceCompile);
        constexpr auto IncludingGpuShaders = true;
        constexpr auto ShowProgress = false;
        System->WaitForCompilationComplete(IncludingGpuShaders, ShowProgress);

        System->MarkPackageDirty();
        FAssetRegistryModule::AssetCreated(System);

        // ---- Save ----
        const auto FileName = FPackageName::LongPackageNameToFilename(
            PkgPath, FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        UPackage::SavePackage(Package, System, *FileName, SaveArgs);

        ck::particles_editor::Log(TEXT("Generated particle system [{}] (BehaviorId {})"),
            ScriptName, InDef->_BehaviorId);
        return System;
    }

    auto Generate_AllParticleSystems() -> FGenerateResult
    {
        FGenerateResult Result;
        const auto& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();
        TArray<FAssetData> Assets;
        ARM.GetAssetsByClass(UCkParticles_ScriptDefinition::StaticClass()->GetClassPathName(), Assets);
        for (const auto& A : Assets)
        {
            auto* Def = Cast<UCkParticles_ScriptDefinition>(A.GetAsset());
            auto* System = Generate_ParticleSystem(Def);
            if (ck::Is_NOT_Valid(System, ck::IsValid_Policy_NullptrOnly{}))
            { ++Result.NumSkipped; continue; }

            ++Result.NumGenerated;
        }
        ck::particles_editor::Log(TEXT("CkParticles generate: {} generated, {} skipped"),
            Result.NumGenerated, Result.NumSkipped);
        return Result;
    }
}

// --------------------------------------------------------------------------------------------------------------------
