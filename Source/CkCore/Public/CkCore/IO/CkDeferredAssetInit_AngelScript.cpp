#include "CkDeferredAssetInit_AngelScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/CkCoreLog.h"
#include "CkCore/IO/CkIO_Utils.h"

#include <Misc/CoreDelegates.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptManager.h>
#include <ClassGenerator/AngelscriptClassGenerator.h>
#include <ClassGenerator/ASClass.h>
#include <as_context.h>
#endif

// --------------------------------------------------------------------------------------------------------------------
// Registrar — bind both the boot-complete and AS-hot-reload entry points. The hot-reload bind
// is essential: the AS plugin re-runs __Init_<Name> on cached instances without resetting,
// so `_Arr.Add(...)` in asset bodies would accumulate across reloads.
// --------------------------------------------------------------------------------------------------------------------

namespace
{
    struct FDeferredAssetInitRegistrar
    {
        FDeferredAssetInitRegistrar()
        {
            FCoreDelegates::OnFEngineLoopInitComplete.AddStatic(
                &UCk_DeferredAssetInit_UE::ResolveAllPending);

#if WITH_ANGELSCRIPT_CK
            FAngelscriptClassGenerator::OnPostReload.AddStatic(
                &UCk_DeferredAssetInit_UE::OnAngelscriptPostReload);
#endif
        }
    };

    static FDeferredAssetInitRegistrar GDeferredAssetInitRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK

namespace
{
    // ----------------------------------------------------------------------------------------------------------------
    // Phase 2 instance reset — copy CDO defaults over the cached instance before re-running
    // __Init_, so `_Arr.Add(...)` style body statements don't double-apply.
    //
    // Skip-list nuance: bare Instanced object refs (`UPROPERTY(Instanced) UMyComp*`) must be
    // preserved — orphaning the subobject would break any `default _Comp.Foo = ...;` writes.
    // But Instanced *containers* (`UPROPERTY(Instanced) TArray<TObjectPtr<UFoo>>`) MUST reset:
    // asset bodies recreate their contents via NewObject, so without the reset the array
    // accumulates new subobjects every reload. Orphans from prior runs are GC'd.
    // ----------------------------------------------------------------------------------------------------------------

    constexpr auto ShouldCreateCDO = false;

    auto ShouldSkipReset(const FProperty* InProp) -> bool
    {
        constexpr auto TransientFlags =
            CPF_Transient |
            CPF_DuplicateTransient |
            CPF_NonPIEDuplicateTransient;

        if (InProp->HasAnyPropertyFlags(TransientFlags))
        { return true; }

        constexpr auto InstancedFlags =
            CPF_InstancedReference |
            CPF_ContainsInstancedReference |
            CPF_PersistentInstance |
            CPF_ExportObject;

        if (NOT InProp->HasAnyPropertyFlags(InstancedFlags))
        { return false; }

        const auto IsContainer = InProp->IsA<FArrayProperty>()
                              || InProp->IsA<FSetProperty>()
                              || InProp->IsA<FMapProperty>();
        return NOT IsContainer;
    }

    auto ResetInstanceFromCDO(UObject* InInstance) -> void
    {
        auto* Class = InInstance->GetClass();
        auto* CDO   = Class->GetDefaultObject(ShouldCreateCDO);
        if (ck::Is_NOT_Valid(CDO, ck::IsValid_Policy_NullptrOnly{}) || CDO == InInstance)
        { return; }

        for (TFieldIterator<FProperty> PropIt(Class, EFieldIteratorFlags::IncludeSuper); PropIt; ++PropIt)
        {
            auto* Prop = *PropIt;
            if (ShouldSkipReset(Prop))
            { continue; }

            Prop->CopyCompleteValue_InContainer(InInstance, CDO);
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Execution helpers
    // ----------------------------------------------------------------------------------------------------------------

    // Build a name→function map once per module instead of linear-scanning globalFunctionList
    // for every getter/init pair. For M literal assets and N total globals this replaces
    // O(M·N) per module with O(N) build + O(1) lookups.
    auto BuildFunctionMap(asCModule* InScriptModule) -> TMap<FString, asCScriptFunction*>
    {
        const auto Count = static_cast<int32>(InScriptModule->globalFunctionList.GetLength());

        auto Map = TMap<FString, asCScriptFunction*>{};
        Map.Reserve(Count);

        for (int32 i = 0; i < Count; ++i)
        {
            auto* Func = InScriptModule->globalFunctionList[i];
            Map.Add(FString{StringCast<TCHAR>(Func->name.AddressOf()).Get()}, Func);
        }
        return Map;
    }

    auto Execute_Logging(asIScriptContext* InContext, const FString& InContextLabel) -> bool
    {
        const auto Result = InContext->Execute();
        if (Result == asEXECUTION_FINISHED)
        { return true; }

        ck::core::Error(TEXT("[DeferredAssetInit] {} failed: asExecutionResult=[{}]"),
                        InContextLabel, static_cast<int32>(Result));
        return false;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Phase 1: re-run DefaultsFunction on every AS class CDO.
    //
    // Matches the engine's ExecuteDefaultsFunctions pattern in ASClass.cpp — walk the super chain
    // collecting DefaultsFunctions child→parent, then execute in reverse so parents run first.
    // No CDO pre-reset: first-pass init already populated the CDO, and re-running over the top
    // is idempotent for scalar/object-ref assignments (which is what AS class defaults almost
    // always are). Array/Map/Set `default _X.Add(...)` patterns on class defaults would double
    // here — that's a known limitation; users should prefer container assignment if needed.
    //
    // Scope: we iterate AS classes via FAngelscriptManager::GetActiveModules() → Module->Classes
    // (FAngelscriptClassDesc::Class) instead of TObjectIterator<UClass>, which would also scan
    // every non-AS UClass in the process (thousands) just to Cast<UASClass> them away.
    // ----------------------------------------------------------------------------------------------------------------

    auto ReRunAllClassDefaults() -> int32
    {
        auto SucceededCount = int32{0};

        auto ActiveModules = FAngelscriptManager::Get().GetActiveModules();

        ck::algo::ForEach(ActiveModules, [&](const TSharedRef<FAngelscriptModuleDesc>& Module)
        {
            ck::algo::ForEach(Module->Classes, [&](const TSharedRef<FAngelscriptClassDesc>& ClassDesc)
            {
                auto* ASClass = Cast<UASClass>(ClassDesc->Class);
                if (ck::Is_NOT_Valid(ASClass, ck::IsValid_Policy_NullptrOnly{}))
                { return; }

                if (ASClass->DefaultsFunction == nullptr)
                { return; }

                if (ASClass->HasAnyClassFlags(CLASS_Abstract | CLASS_NewerVersionExists))
                { return; }

                auto* CDO = ASClass->GetDefaultObject(ShouldCreateCDO);
                if (ck::Is_NOT_Valid(CDO, ck::IsValid_Policy_NullptrOnly{}))
                { return; }

                auto DefaultsFunctions = TArray<asIScriptFunction*, TFixedAllocator<32>>{};
                for (auto* WalkClass = ASClass; ck::IsValid(WalkClass, ck::IsValid_Policy_NullptrOnly{}); WalkClass = Cast<UASClass>(WalkClass->GetSuperClass()))
                {
                    if (WalkClass->DefaultsFunction != nullptr)
                    { DefaultsFunctions.Add(WalkClass->DefaultsFunction); }
                }

                // Each DefaultsFunction in the super chain is invoked independently — a failure
                // in one does NOT skip the rest. Mirrors the engine's ExecuteDefaultsFunctions
                // (ASClass.cpp:1070-1077) where every function gets its own context and is
                // executed regardless of sibling success.
                auto AllOk = true;
                for (auto i = DefaultsFunctions.Num() - 1; i >= 0; --i)
                {
                    auto Context = FAngelscriptContext{CDO};
                    Context->Prepare(DefaultsFunctions[i]);
                    Context->m_executeVirtualCall = false;
                    Context->SetObject(CDO);

                    if (NOT Execute_Logging(Context, ck::Format_UE(TEXT("DefaultsFunction for class '{}'"),
                                                                   ASClass->GetName())))
                    { AllOk = false; }
                }

                if (AllOk)
                { ++SucceededCount; }
            });
        });

        return SucceededCount;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Phase 2: re-run __Init_<AssetName> on every literal asset.
    //
    // Literal asset preprocessor generates (from AngelscriptPreprocessor.cpp ~4003):
    //     {Type} __Asset_{Name};
    //     {Type} Get{Name}() property       // creates on first call, caches, returns cached thereafter
    //     void __Init_{Name}({Type} {Name}) // the user-written asset block body
    //
    // The getter caches on first call, so calling it again doesn't re-run __Init_. We call
    // __Init_ directly to force re-execution on the existing cached instance, having first
    // reset the instance from its CDO (so `.Add()`-style statements in the body don't double).
    //
    // For the init function name we use the engine's public PostInitFunctions list as a sanity
    // check (each entry is `Get<Name>` — getter names, not init names, per the preprocessor).
    // The `__Init_` prefix is a stable preprocessor convention; if it ever changes, the sanity
    // check will log the drift so we find out loudly rather than silently failing.
    // ----------------------------------------------------------------------------------------------------------------

    struct FPhase2Stats
    {
        int32 Succeeded = 0;
        int32 Declared  = 0;
    };

    auto ReRunAllLiteralAssetInits() -> FPhase2Stats
    {
        auto Stats = FPhase2Stats{};

        auto ActiveModules = FAngelscriptManager::Get().GetActiveModules();

        ck::algo::ForEach(ActiveModules, [&](const TSharedRef<FAngelscriptModuleDesc>& Module)
        {
            if (Module->ScriptModule == nullptr)
            { return; }

            if (Module->DeclaredLiteralAssets.IsEmpty())
            { return; }

            // Cross-validate against the engine's PostInitFunctions list (these are the
            // Get<Name> getter names, per AngelscriptPreprocessor.cpp:4027). If Get<AssetName>
            // is missing from it, the preprocessor's naming convention has drifted.
            const auto PostInitFunctionSet = TSet<FString>{Module->PostInitFunctions};
            const auto FunctionMap         = BuildFunctionMap(Module->ScriptModule);

            ck::algo::ForEach(Module->DeclaredLiteralAssets, [&](const FString& AssetName)
            {
                ++Stats.Declared;

                const auto InitFunctionName = ck::Format_UE(TEXT("__Init_{}"), AssetName);
                const auto GetterName       = ck::Format_UE(TEXT("Get{}"),     AssetName);

                if (NOT PostInitFunctionSet.Contains(GetterName))
                {
                    ck::core::Error(TEXT("[DeferredAssetInit] Literal asset '{}' getter '{}' not found in engine's PostInitFunctions — AS preprocessor naming convention may have drifted"),
                                    AssetName, GetterName);
                    return;
                }

                auto* InitFunction   = FunctionMap.FindRef(InitFunctionName);
                auto* GetterFunction = FunctionMap.FindRef(GetterName);

                if (InitFunction == nullptr || GetterFunction == nullptr)
                {
                    ck::core::Error(TEXT("[DeferredAssetInit] Literal asset '{}' — failed to resolve globalFunctionList entries (Init=[{}], Getter=[{}])"),
                                    AssetName,
                                    InitFunction   != nullptr,
                                    GetterFunction != nullptr);
                    return;
                }

                // Retrieve the existing cached instance via the getter
                UObject* AssetInstance = nullptr;
                {
                    auto Context = FAngelscriptContext{};
                    Context->Prepare(GetterFunction);
                    if (NOT Execute_Logging(Context, ck::Format_UE(TEXT("Get{}"), AssetName)))
                    { return; }

                    AssetInstance = *static_cast<UObject**>(Context->GetAddressOfReturnValue());
                }

                if (ck::Is_NOT_Valid(AssetInstance, ck::IsValid_Policy_NullptrOnly{}))
                {
                    ck::core::Error(TEXT("[DeferredAssetInit] Literal asset '{}' — getter returned null/invalid instance"), AssetName);
                    return;
                }

                ResetInstanceFromCDO(AssetInstance);

                // Re-run __Init_<AssetName>(AssetInstance) on the existing instance
                {
                    auto Context = FAngelscriptContext{};
                    Context->Prepare(InitFunction);
                    Context->SetArgObject(0, AssetInstance);
                    if (NOT Execute_Logging(Context, ck::Format_UE(TEXT("__Init_{}"), AssetName)))
                    { return; }
                }

                ++Stats.Succeeded;
            });
        });

        return Stats;
    }
}

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DeferredAssetInit_UE::
    ResolveAllPending()
    -> void
{
#if WITH_ANGELSCRIPT_CK

    // Force the blocking-load safety flag true before Phase 2 runs. Both this callback and
    // the flag-setting lambda in CkIO_Utils.cpp bind to OnFEngineLoopInitComplete; delegates
    // fire in add-order, and cross-TU static init order is unspecified. If the flag-setter
    // happens to fire after us, assets::load:: would return nullptr during Phase 2 — defeating
    // the entire point of the re-run. We know blocking-loads are safe at this point, so just
    // ensure the flag reflects that before we proceed.
    UCk_Utils_IO_UE::MarkEngineSafeForBlockingLoads();

    // Short-circuit: if no caller ever queried IsEngineSafeForBlockingLoads() while the flag
    // was still false, nothing was deferred this boot — skip the entire sweep. This is the
    // common case for projects that don't use assets::load:: directly in class defaults or
    // literal-asset bodies, and keeps startup cost near zero when there's no work to do.
    if (NOT UCk_Utils_IO_UE::WasBlockingLoadQueriedWhileUnsafe())
    {
        ck::core::Verbose(TEXT("[DeferredAssetInit] No blocking loads were deferred — skipping re-init"));
        return;
    }

#if WITH_EDITOR
    ck::core::Display(TEXT("[DeferredAssetInit] Engine init complete — re-running Angelscript default inits"));
#else
    ck::core::Verbose(TEXT("[DeferredAssetInit] Engine init complete — re-running Angelscript default inits"));
#endif

    const auto CdoCount = ReRunAllClassDefaults();

#if WITH_EDITOR
    ck::core::Display(TEXT("[DeferredAssetInit] Phase 1: re-initialized {} Angelscript CDO(s)"), CdoCount);
#else
    ck::core::Verbose(TEXT("[DeferredAssetInit] Phase 1: re-initialized {} Angelscript CDO(s)"), CdoCount);
#endif

    const auto LiteralAssetStats = ReRunAllLiteralAssetInits();
    if (LiteralAssetStats.Succeeded < LiteralAssetStats.Declared)
    {
        // Always Warning — a discrepancy here is a real signal worth surfacing in every config.
        ck::core::Warning(TEXT("[DeferredAssetInit] Phase 2: re-initialized {}/{} literal asset(s) — missing entries indicate AS preprocessor drift"),
                          LiteralAssetStats.Succeeded, LiteralAssetStats.Declared);
    }
    else
    {
#if WITH_EDITOR
        ck::core::Display(TEXT("[DeferredAssetInit] Phase 2: re-initialized {}/{} literal asset(s)"),
                          LiteralAssetStats.Succeeded, LiteralAssetStats.Declared);
#else
        ck::core::Verbose(TEXT("[DeferredAssetInit] Phase 2: re-initialized {}/{} literal asset(s)"),
                          LiteralAssetStats.Succeeded, LiteralAssetStats.Declared);
#endif
    }

#endif // WITH_ANGELSCRIPT_CK
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DeferredAssetInit_UE::
    OnAngelscriptPostReload(
        bool InFullReload)
    -> void
{
#if WITH_ANGELSCRIPT_CK

#if WITH_EDITOR
    ck::core::Display(TEXT("[DeferredAssetInit] Angelscript post-reload (FullReload=[{}]) — re-running literal asset inits"),
                      InFullReload);
#else
    ck::core::Verbose(TEXT("[DeferredAssetInit] Angelscript post-reload (FullReload=[{}]) — re-running literal asset inits"),
                      InFullReload);
#endif

    const auto Stats = ReRunAllLiteralAssetInits();
    if (Stats.Succeeded < Stats.Declared)
    {
        ck::core::Warning(TEXT("[DeferredAssetInit] Post-reload: re-initialized {}/{} literal asset(s) — missing entries indicate AS preprocessor drift"),
                          Stats.Succeeded, Stats.Declared);
    }
    else
    {
#if WITH_EDITOR
        ck::core::Display(TEXT("[DeferredAssetInit] Post-reload: re-initialized {}/{} literal asset(s)"),
                          Stats.Succeeded, Stats.Declared);
#else
        ck::core::Verbose(TEXT("[DeferredAssetInit] Post-reload: re-initialized {}/{} literal asset(s)"),
                          Stats.Succeeded, Stats.Declared);
#endif
    }

#endif // WITH_ANGELSCRIPT_CK
}

// --------------------------------------------------------------------------------------------------------------------
