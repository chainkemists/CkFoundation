#include "CkDeferredAssetInit_AngelScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/CkCoreLog.h"
#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/IO/CkIO_Utils.h"

#include <Misc/CoreDelegates.h>
#include <HAL/IConsoleManager.h>
#include <UObject/FastReferenceCollector.h>
#include <UObject/Package.h>
#include <UObject/UObjectArray.h>
#include <UObject/UObjectGlobals.h>
#include <UObject/UObjectHash.h>

#if WITH_ANGELSCRIPT_CK
#include <AngelscriptManager.h>
#include <ClassGenerator/AngelscriptClassGenerator.h>
#include <ClassGenerator/ASClass.h>
#include <as_context.h>
#endif

// --------------------------------------------------------------------------------------------------------------------
// The hot-reload bind is essential: the AS plugin re-runs __Init_<Name> on cached instances without
// resetting, so `_Arr.Add(...)` in asset bodies would accumulate across reloads.
// --------------------------------------------------------------------------------------------------------------------

namespace ck_deferred_asset_init_angelscript_registrar
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

#if !WITH_EDITOR
            // Why this must run before EVERY collection: see RootAngelscriptDisregardViolations.
            FCoreUObjectDelegates::GetPreGarbageCollectDelegate().AddStatic(
                &UCk_DeferredAssetInit_UE::OnPreGarbageCollect);
#endif
#endif
        }
    };

    static FDeferredAssetInitRegistrar GDeferredAssetInitRegistrar;
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_ANGELSCRIPT_CK

namespace ck_deferred_asset_init_angelscript
{
    // ----------------------------------------------------------------------------------------------------------------
    // Phase 2 copies CDO defaults over the cached instance before re-running __Init_, so `_Arr.Add(...)`
    // bodies don't double-apply. Bare Instanced object refs must be PRESERVED (orphaning the subobject
    // breaks `default _Comp.Foo = ...`), but Instanced CONTAINERS must reset — bodies recreate contents.
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

    // One build per module replaces an O(M·N) linear scan of globalFunctionList per getter/init pair.
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
    // Surgical-heal attribution: re-running ALL ~1200 CDO defaults and literal __Inits to heal the
    // handful that actually deferred is AS-execution bound, so we record exactly which deferred and
    // re-run only those. An unreadable AS context sets GAttributionUncertain → full sweep; never under-heal.
    // ----------------------------------------------------------------------------------------------------------------

    TSet<TWeakObjectPtr<UObject>> GDeferredLoadCDOs;     // Phase 1: CDOs whose defaults deferred a load
    TSet<FString>                 GDeferredLiteralNames;  // Phase 2: literal assets whose __Init deferred
    bool                          GAttributionUncertain = false;

    bool GForceFullAssetHeal = false;
    static FAutoConsoleVariableRef CVar_ForceFullAssetHeal(
        TEXT("ck.DeferredAssetInit.ForceFullHeal"),
        GForceFullAssetHeal,
        TEXT("Re-run ALL Angelscript CDO defaults during the deferred-asset heal sweep instead of only "
             "those with deferred loads. Safety fallback if a startup asset ever comes up null."),
        ECVF_Default);

    // Attributes the deferred load to the CDO whose DefaultsFunction is running [Phase 1] and/or the
    // literal __Init_<Name> running [Phase 2]. Either, both or neither may appear in a given stack;
    // "neither" is safe, since the original full sweep never healed those cases either.
    auto CaptureDeferredAttribution() -> void
    {
        // Hazelight's exported wrapper over asGetActiveContext() — the raw library free function is not
        // exported to CkCore.
        auto* Context = FAngelscriptManager::GetCurrentScriptContext();
        if (Context == nullptr)
        {
            GAttributionUncertain = true;
            return;
        }

        static const auto LiteralInitPrefix = FString{TEXT("__Init_")};

        const auto StackDepth = static_cast<int32>(Context->GetCallstackSize());
        for (auto Frame = 0; Frame < StackDepth; ++Frame)
        {
            auto* Func = Context->GetFunction(Frame);
            if (Func == nullptr)
            { continue; }

            // (a) CDO default: a frame whose `this` class chain owns this DefaultsFunction.
            if (auto* ThisObj = static_cast<UObject*>(Context->GetThisPointer(Frame));
                ck::IsValid(ThisObj, ck::IsValid_Policy_NullptrOnly{}))
            {
                for (auto* CheckClass = ThisObj->GetClass();
                     ck::IsValid(CheckClass, ck::IsValid_Policy_NullptrOnly{});
                     CheckClass = CheckClass->GetSuperClass())
                {
                    const auto* ASClass = Cast<UASClass>(CheckClass);
                    if (ASClass != nullptr && ASClass->DefaultsFunction == Func)
                    {
                        GDeferredLoadCDOs.Add(ThisObj);
                        break;
                    }
                }
            }

            // (b) Literal asset: a frame running the preprocessor-generated __Init_<Name> body.
            const auto FuncName = FString{StringCast<TCHAR>(Func->GetName()).Get()};
            if (FuncName.StartsWith(LiteralInitPrefix))
            { GDeferredLiteralNames.Add(FuncName.RightChop(LiteralInitPrefix.Len())); }
        }
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Phase 1 matches the engine's ExecuteDefaultsFunctions (ASClass.cpp): collect DefaultsFunctions
    // child→parent up the super chain, execute in reverse so parents run first. No CDO pre-reset —
    // re-running over the top is idempotent for the scalar/object-ref assignments AS defaults almost are.
    // ----------------------------------------------------------------------------------------------------------------

    // True only when EVERY DefaultsFunction in the chain ran cleanly.
    auto ReRunClassDefaultsFor(UASClass* InASClass) -> bool
    {
        if (ck::Is_NOT_Valid(InASClass, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        if (InASClass->DefaultsFunction == nullptr)
        { return false; }

        if (InASClass->HasAnyClassFlags(CLASS_Abstract | CLASS_NewerVersionExists))
        { return false; }

        auto* CDO = InASClass->GetDefaultObject(ShouldCreateCDO);
        if (ck::Is_NOT_Valid(CDO, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        auto DefaultsFunctions = TArray<asIScriptFunction*, TFixedAllocator<32>>{};
        for (auto* WalkClass = InASClass; ck::IsValid(WalkClass, ck::IsValid_Policy_NullptrOnly{}); WalkClass = Cast<UASClass>(WalkClass->GetSuperClass()))
        {
            if (WalkClass->DefaultsFunction != nullptr)
            { DefaultsFunctions.Add(WalkClass->DefaultsFunction); }
        }

        // Deliberately invoked independently — a failure in one must NOT skip the rest, matching the
        // engine's ExecuteDefaultsFunctions (ASClass.cpp).
        auto AllOk = true;
        for (auto i = DefaultsFunctions.Num() - 1; i >= 0; --i)
        {
            auto Context = FAngelscriptContext{CDO};
            Context->Prepare(DefaultsFunctions[i]);
            Context->m_executeVirtualCall = false;
            Context->SetObject(CDO);

            if (NOT Execute_Logging(Context, ck::Format_UE(TEXT("DefaultsFunction for class '{}'"),
                                                           InASClass->GetName())))
            { AllOk = false; }
        }

        return AllOk;
    }

    // Safe fallback when surgical attribution is unavailable or uncertain.
    auto ReRunAllClassDefaults() -> int32
    {
        auto SucceededCount = int32{0};

        auto ActiveModules = FAngelscriptManager::Get().GetActiveModules();

        ck::algo::ForEach(ActiveModules, [&](const TSharedRef<FAngelscriptModuleDesc>& Module)
        {
            ck::algo::ForEach(Module->Classes, [&](const TSharedRef<FAngelscriptClassDesc>& ClassDesc)
            {
                if (ReRunClassDefaultsFor(Cast<UASClass>(ClassDesc->Class)))
                { ++SucceededCount; }
            });
        });

        return SucceededCount;
    }

    // Returns the number of distinct CDOs re-run.
    auto ReRunDeferredClassDefaults() -> int32
    {
        auto SucceededCount = int32{0};

        for (const auto& WeakCdo : GDeferredLoadCDOs)
        {
            auto* CDO = WeakCdo.Get();
            if (ck::Is_NOT_Valid(CDO, ck::IsValid_Policy_NullptrOnly{}))
            { continue; }

            if (NOT CDO->HasAnyFlags(RF_ClassDefaultObject))
            { continue; }

            if (ReRunClassDefaultsFor(Cast<UASClass>(CDO->GetClass())))
            { ++SucceededCount; }
        }

        return SucceededCount;
    }

    // ----------------------------------------------------------------------------------------------------------------
    // The literal-asset preprocessor emits `__Asset_{Name}`, a CACHING `Get{Name}()` property, and
    // `void __Init_{Name}({Type})` holding the user's asset body. Because the getter caches on first
    // call, Phase 2 calls __Init_ directly on the CDO-reset instance to force re-execution.
    // ----------------------------------------------------------------------------------------------------------------

    struct FPhase2Stats
    {
        int32 Succeeded = 0;
        int32 Declared  = 0;
    };

    // Full heal is the safety fallback and the hot-reload path, where first-pass attribution never applied.
    auto ReRunLiteralAssetInits(bool InFullHeal) -> FPhase2Stats
    {
        auto Stats = FPhase2Stats{};

        auto ActiveModules = FAngelscriptManager::Get().GetActiveModules();

        ck::algo::ForEach(ActiveModules, [&](const TSharedRef<FAngelscriptModuleDesc>& Module)
        {
            if (Module->ScriptModule == nullptr)
            { return; }

            if (Module->DeclaredLiteralAssets.IsEmpty())
            { return; }

            // PostInitFunctions holds Get<Name> GETTER names; a missing entry means the preprocessor's
            // naming convention drifted, which the check below logs loudly rather than failing silently.
            const auto PostInitFunctionSet = TSet<FString>{Module->PostInitFunctions};
            const auto FunctionMap         = BuildFunctionMap(Module->ScriptModule);

            ck::algo::ForEach(Module->DeclaredLiteralAssets, [&](const FString& AssetName)
            {
                if (NOT InFullHeal && NOT GDeferredLiteralNames.Contains(AssetName))
                { return; }

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

    // ----------------------------------------------------------------------------------------------------------------
    // AS asset owners and CDOs are created before FEngineLoop closes the disregard-for-GC set, so GC never
    // traverses them and reclaims the normal-pool objects the sweep attached under them → dangling ptr →
    // crash. AddToRoot on those targets satisfies the verifier's accept-test; it must go through the GC
    // reference collector (FReferenceFinder's token-stream walk misses AS script-class members), must run
    // pre-GC (some refs resolve lazily), and must never unroot (AddToRoot is a flag, not a refcount).
    // ----------------------------------------------------------------------------------------------------------------

#if !WITH_EDITOR
    // Diagnostics only; we never unroot, so a running total is all this needs to be.
    int32 GTotalRooted = 0;

    // Roots the normal-pool objects referenced by a disregard-for-GC object — exactly the edges the engine's
    // disregard verifier flags. Runs single-threaded (DefaultOptions is not parallel), so AddToRoot is safe.
    class FCkAsDisregardRootingProcessor : public FSimpleReferenceProcessorBase
    {
    public:
        int32 NewlyRooted = 0;

        FORCEINLINE void HandleTokenStreamObjectReference(
            UE::GC::FWorkerContext& /*InContext*/,
            UObject*                InReferencingObject,
            UObject*&               InObject,
            UE::GC::FMemberId       /*InMemberId*/,
            UE::GC::EOrigin         /*InOrigin*/,
            bool                    /*bInAllowReferenceElimination*/)
        {
            if (InObject == nullptr || InReferencingObject == nullptr)
            { return; }

            // Only references emanating FROM a disregard object break the invariant; GC traces the rest.
            if (NOT GUObjectArray.IsDisregardForGC(InReferencingObject))
            { return; }

            // Mirror the verifier's accept-set: rooted / disregard / cluster member / cluster root are all fine.
            if (InObject->IsRooted() || GUObjectArray.IsDisregardForGC(InObject))
            { return; }

            if (InObject->HasAnyInternalFlags(EInternalObjectFlags::Garbage | EInternalObjectFlags::Unreachable))
            { return; }

            const auto* RefItem = GUObjectArray.ObjectToObjectItem(InObject);
            if (RefItem != nullptr && (RefItem->GetOwnerIndex() > 0 || RefItem->HasAnyFlags(EInternalObjectFlags::ClusterRoot)))
            { return; }

            InObject->AddToRoot();
            ++GTotalRooted;
            ++NewlyRooted;
        }
    };

    auto RootAngelscriptDisregardViolations() -> void
    {
        // AddToRoot in the processor is not thread-safe and the collector below runs on the calling
        // thread, so a future move off the game thread must trip loudly instead of racing silently.
        CK_ENSURE_IF_NOT(IsInGameThread(),
            TEXT("[DeferredAssetInit] disregard-for-GC rooting must run on the game thread"))
        { return; }

        static const TCHAR* AsPackageNames[] =
        {
            TEXT("/Script/AngelscriptAssets"),
            TEXT("/Script/Angelscript"),
        };

        // With FSimpleReferenceProcessorBase the collector reports only the DIRECT references of these objects
        // (same usage as the engine's VerifyGCAssumptions), so every edge handled is a disregard→X edge.
        auto InitialObjects = TArray<UObject*>{};
        for (const auto* PackageName : AsPackageNames)
        {
            auto* Package = FindObject<UPackage>(nullptr, PackageName);
            if (Package == nullptr)
            { continue; }

            constexpr auto bIncludeNestedObjects = true;
            ForEachObjectWithOuter(Package, [&](UObject* InObject)
            {
                if (GUObjectArray.IsDisregardForGC(InObject))
                { InitialObjects.Add(InObject); }
            }, bIncludeNestedObjects);
        }

        if (InitialObjects.IsEmpty())
        { return; }

        // SetInitialObjectsUnpadded pads by repeating the last element past the end; reserve so it can't realloc.
        InitialObjects.Reserve(InitialObjects.Num() + UE::GC::ObjectLookahead);

        auto Processor = FCkAsDisregardRootingProcessor{};
        auto Context   = UE::GC::FWorkerContext{};
        Context.SetInitialObjectsUnpadded(InitialObjects);
        CollectReferences(Processor, Context);

        // Runs every GC — only log when something new was rooted, so steady-state passes stay silent.
        if (Processor.NewlyRooted > 0)
        {
            ck::core::Display(TEXT("[DeferredAssetInit] Rooted {} new disregard-for-GC violation target(s) ({} total)"),
                              Processor.NewlyRooted, GTotalRooted);
        }
    }
#endif // !WITH_EDITOR
}

#endif // WITH_ANGELSCRIPT_CK

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DeferredAssetInit_UE::
    Note_DeferredAssetLoad_FromActiveContext()
    -> void
{
#if WITH_ANGELSCRIPT_CK
    ck_deferred_asset_init_angelscript::CaptureDeferredAttribution();
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_DeferredAssetInit_UE::
    ResolveAllPending()
    -> void
{
#if WITH_ANGELSCRIPT_CK

    // This callback and CkIO_Utils' flag-setting lambda both bind to OnFEngineLoopInitComplete, and
    // cross-TU static init order is unspecified — if the setter fires after us, assets::load:: returns
    // nullptr through all of Phase 2. Blocking loads ARE safe here, so make the flag say so.
    UCk_Utils_IO_UE::MarkEngineSafeForBlockingLoads();

    // Nobody queried the flag while it was false ⇒ nothing was deferred this boot ⇒ no sweep needed.
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

    const auto UseFullHeal = ck_deferred_asset_init_angelscript::GForceFullAssetHeal || ck_deferred_asset_init_angelscript::GAttributionUncertain;
    const auto HealMode    = UseFullHeal ? FString(TEXT("full")) : FString(TEXT("surgical"));
    const auto CdoCount    = UseFullHeal ? ck_deferred_asset_init_angelscript::ReRunAllClassDefaults() : ck_deferred_asset_init_angelscript::ReRunDeferredClassDefaults();

#if WITH_EDITOR
    ck::core::Display(TEXT("[DeferredAssetInit] Phase 1: re-initialized {} Angelscript CDO(s) [{}]"), CdoCount, HealMode);
#else
    ck::core::Verbose(TEXT("[DeferredAssetInit] Phase 1: re-initialized {} Angelscript CDO(s) [{}]"), CdoCount, HealMode);
#endif

    const auto LiteralAssetStats = ck_deferred_asset_init_angelscript::ReRunLiteralAssetInits(UseFullHeal);
    if (LiteralAssetStats.Succeeded < LiteralAssetStats.Declared)
    {
        // Warning in EVERY config — a discrepancy here is a real signal.
        ck::core::Warning(TEXT("[DeferredAssetInit] Phase 2: re-initialized {}/{} literal asset(s) [{}] — missing entries indicate AS preprocessor drift"),
                          LiteralAssetStats.Succeeded, LiteralAssetStats.Declared, HealMode);
    }
    else
    {
#if WITH_EDITOR
        ck::core::Display(TEXT("[DeferredAssetInit] Phase 2: re-initialized {}/{} literal asset(s) [{}]"),
                          LiteralAssetStats.Succeeded, LiteralAssetStats.Declared, HealMode);
#else
        ck::core::Verbose(TEXT("[DeferredAssetInit] Phase 2: re-initialized {}/{} literal asset(s) [{}]"),
                          LiteralAssetStats.Succeeded, LiteralAssetStats.Declared, HealMode);
#endif
    }

    // ONE aggregated line instead of the old per-call stack-walking ensure storm: these deferrals are
    // expected and healed by this sweep, so a surprising count just points at a soft-ref candidate.
    if (const auto PrematureCount = UCk_Utils_IO_UE::Get_PrematureAssetLoadCount();
        PrematureCount > 0)
    {
#if WITH_EDITOR
        ck::core::Display(TEXT("[DeferredAssetInit] {} assets::load::* call(s) deferred before engine-safe and resolved by this sweep (first: '{}')"),
                          PrematureCount, UCk_Utils_IO_UE::Get_FirstPrematureAssetLoadMessage());
#else
        ck::core::Verbose(TEXT("[DeferredAssetInit] {} assets::load::* call(s) deferred before engine-safe and resolved by this sweep (first: '{}')"),
                          PrematureCount, UCk_Utils_IO_UE::Get_FirstPrematureAssetLoadMessage());
#endif
    }
    UCk_Utils_IO_UE::Reset_PrematureAssetLoadReport();

    // Attribution is single-use per boot sweep — clear so a later hot-reload starts clean.
    ck_deferred_asset_init_angelscript::GDeferredLoadCDOs.Reset();
    ck_deferred_asset_init_angelscript::GDeferredLiteralNames.Reset();
    ck_deferred_asset_init_angelscript::GAttributionUncertain = false;

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

    // The initial-compile post-reload fires BEFORE OnFEngineLoopInitComplete, where the sweep would heal
    // nothing and storm ensures; ResolveAllPending is a strict superset of it. The peek is deliberate —
    // the querying getter would trip the WasBlockingLoadQueriedWhileUnsafe short-circuit.
    if (NOT UCk_Utils_IO_UE::Get_IsEngineSafeForBlockingLoads_Peek())
    {
        ck::core::Verbose(TEXT("[DeferredAssetInit] Post-reload before engine-safe — skipping (ResolveAllPending heals)"));
        return;
    }

#if WITH_EDITOR
    ck::core::Display(TEXT("[DeferredAssetInit] Angelscript post-reload (FullReload=[{}]) — re-running literal asset inits"),
                      InFullReload);
#else
    ck::core::Verbose(TEXT("[DeferredAssetInit] Angelscript post-reload (FullReload=[{}]) — re-running literal asset inits"),
                      InFullReload);
#endif

    // Always ALL literals: the reload happens engine-safe so first-pass attribution never fired, and the
    // full re-run is what undoes the `_Arr.Add(...)` doubling the reload causes.
    constexpr auto FullHeal = true;
    const auto Stats = ck_deferred_asset_init_angelscript::ReRunLiteralAssetInits(FullHeal);
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

auto
    UCk_DeferredAssetInit_UE::
    OnPreGarbageCollect()
    -> void
{
#if WITH_ANGELSCRIPT_CK
#if !WITH_EDITOR
    ck_deferred_asset_init_angelscript::RootAngelscriptDisregardViolations();
#endif
#endif
}

// --------------------------------------------------------------------------------------------------------------------
