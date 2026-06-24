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

#if !WITH_EDITOR
            // Root the disregard-for-GC violation targets right before each collection. See
            // OnPreGarbageCollect / RootAngelscriptDisregardViolations for the why.
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
    // Surgical-heal attribution.
    //
    // The full sweep re-runs ALL ~1200 CDO DefaultsFunctions (Phase 1) and ALL literal __Inits
    // (Phase 2) just to heal the handful that actually deferred a (null) asset load during first-pass
    // — and measurement showed the sweep is dominated by that AS execution, not the IO. So we record
    // the EXACT entities that deferred and re-run only those: CDOs in GDeferredLoadCDOs (Phase 1),
    // literal asset names in GDeferredLiteralNames (Phase 2).
    //
    // Attribution is captured in Note_DeferredAssetLoad_FromActiveContext (called from the AS
    // premature-load helper, ungated so it also runs during cook) via CaptureDeferredAttribution,
    // which walks the active AS call stack ONCE. It mirrors the engine's own GetASConstructionScriptObject
    // (Bind_UObject.cpp): a frame whose `this` class chain owns the executing DefaultsFunction is a
    // CDO default; a frame running a __Init_<Name> global function is a literal asset body.
    //
    // Safety: a load that maps to neither (some other first-pass AS code) was never healed by the
    // original sweep either, so ignoring it is not a regression. If we cannot read the active context
    // at all, GAttributionUncertain forces BOTH phases to the full sweep. The
    // ck.DeferredAssetInit.ForceFullHeal CVar is a manual escape hatch. We NEVER under-heal.
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

    // Walk the active AS call stack ONCE and attribute the deferred load to (a) the CDO whose
    // DefaultsFunction is running [Phase 1] and/or (b) the literal __Init_<Name> running [Phase 2].
    // Either, both, or neither may be present in a given stack:
    //   - direct CDO default load        → DefaultsFunction frame (this == CDO)
    //   - literal referenced by a CDO     → both the CDO frame AND the __Init_<Name> frame
    //   - standalone literal init         → __Init_<Name> frame only (global function, no `this`)
    //   - anything else (not a default/literal) → neither; the original sweep never healed those.
    // We only flag uncertainty (→ full fallback) when the active context can't be read at all.
    auto CaptureDeferredAttribution() -> void
    {
        // FAngelscriptManager::GetCurrentScriptContext() is Hazelight's exported wrapper over
        // asGetActiveContext() — the raw library free function isn't exported to CkCore, but this
        // member is (same path as FAngelscriptManager::Get()/GetActiveModules() used here already).
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

    // Re-run one class's DefaultsFunction super-chain on its CDO. Returns true if all ran cleanly.
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
                                                           InASClass->GetName())))
            { AllOk = false; }
        }

        return AllOk;
    }

    // Full sweep: re-run every AS class's DefaultsFunction. Used when surgical attribution is
    // unavailable/uncertain (safe fallback — never under-heals).
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

    // Surgical sweep: re-run ONLY the CDOs whose DefaultsFunction actually deferred a load during
    // first-pass (captured in GDeferredLoadCDOs). Returns the number of distinct CDOs re-run.
    auto ReRunDeferredClassDefaults() -> int32
    {
        auto SucceededCount = int32{0};

        for (const auto& WeakCdo : GDeferredLoadCDOs)
        {
            auto* CDO = WeakCdo.Get();
            if (ck::Is_NOT_Valid(CDO, ck::IsValid_Policy_NullptrOnly{}))
            { continue; }

            // Only objects that are genuinely a class CDO (the `this` of a DefaultsFunction).
            if (NOT CDO->HasAnyFlags(RF_ClassDefaultObject))
            { continue; }

            if (ReRunClassDefaultsFor(Cast<UASClass>(CDO->GetClass())))
            { ++SucceededCount; }
        }

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

    // Phase 2 sweep. When InFullHeal is false, re-run ONLY the literals whose __Init deferred a load
    // during first-pass (GDeferredLiteralNames) — the rest had nothing to heal. Full heal is used as
    // the safety fallback and for hot-reloads (where first-pass attribution doesn't apply).
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

            // Cross-validate against the engine's PostInitFunctions list (these are the
            // Get<Name> getter names, per AngelscriptPreprocessor.cpp:4027). If Get<AssetName>
            // is missing from it, the preprocessor's naming convention has drifted.
            const auto PostInitFunctionSet = TSet<FString>{Module->PostInitFunctions};
            const auto FunctionMap         = BuildFunctionMap(Module->ScriptModule);

            ck::algo::ForEach(Module->DeclaredLiteralAssets, [&](const FString& AssetName)
            {
                // Surgical: skip literals that never deferred a load during first-pass.
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

    // ----------------------------------------------------------------------------------------------------------------
    // Disregard-for-GC retention (the fix).
    //
    // Bug: AS `asset … of …` owners + AS CDOs are created during AS InitialCompile, BEFORE FEngineLoop closes the
    //   disregard-for-GC set, so they land in the permanent pool — which GC never traverses (it assumes disregard
    //   objects only reference other permanent objects). The refresh above then attaches normal-pool objects under
    //   them (minted item-trait/key-settings sub-objects, and `assets::load::`'d cooked assets on owners/CDO
    //   components), so the first GC reclaims them out from under the untraversed owner → dangling ptr → crash.
    // Fix: AddToRoot those targets so IsRooted() is true — the engine verifier's accept-test
    //   (GarbageCollectionVerification.cpp:110: IsRooted || IsDisregardForGC || OwnerIndex>0 || ClusterRoot).
    // Via the GC reference collector, NOT FReferenceFinder: the AS runtime GC-tracks script-class UObject members
    //   (incl. non-UPROPERTY resolved-hard-ref config fields like `UStaticMesh StandBodyMeshAsset;`) only through
    //   the autogenerated schema the real GC + verifier use; FReferenceFinder's legacy token-stream walk misses
    //   them. CollectReferences over the AS disregard objects enumerates exactly what the verifier checks.
    // Pre-GC (not boot/reload only): some refs resolve lazily as actors stream in, after any boot sweep — re-rooting
    //   right before each collection catches whatever is referenced when the GC that would reclaim it runs.
    // Root-only, never unroot: AddToRoot is a boolean flag (not refcounted), so unrooting could clear a root another
    //   system set on a shared asset. We only root not-already-rooted targets. (Cost: sub-objects orphaned by a
    //   packaged -as-development-mode hot-reload stay rooted — dev-only; shipping bakes AS so there's no reload.)
    // !WITH_EDITOR: the bug doesn't manifest in-editor (asset registry/Content Browser keep things alive), verifiers
    //   are off there, and rooting cooked assets every GC could interfere with editor asset GC.
    // ----------------------------------------------------------------------------------------------------------------

#if !WITH_EDITOR
    // Count of distinct targets we've rooted this process — for the log line / diagnostics only. We
    // never unroot (see the design note above), so a running total is all this needs to be.
    int32 GTotalRooted = 0;

    // Roots normal-pool objects referenced by a disregard-for-GC object — exactly the edges the engine's
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

            // Only references emanating FROM a disregard object break the invariant; everything else is traced
            // normally by GC. (Guards correctness even if the collector ever visits non-disregard referencers.)
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
        // The single-context collector below runs synchronously on the calling thread, and AddToRoot
        // (in the processor) is not thread-safe. The pre-GC delegate is game-thread — assert it so a
        // future move off-thread (or to a parallel collector) trips loudly instead of racing silently.
        CK_ENSURE_IF_NOT(IsInGameThread(),
            TEXT("[DeferredAssetInit] disregard-for-GC rooting must run on the game thread"))
        { return; }

        static const TCHAR* AsPackageNames[] =
        {
            TEXT("/Script/AngelscriptAssets"),
            TEXT("/Script/Angelscript"),
        };

        // Initial set = the AS disregard objects (owners, CDOs, and their disregard sub-components). With
        // FSimpleReferenceProcessorBase the collector reports only the DIRECT references of these objects (same
        // usage as the engine's VerifyGCAssumptions), so every edge handled is a disregard→X edge.
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
    CaptureDeferredAttribution();
#endif
}

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

    // Phase 1: re-run AS class CDO DefaultsFunctions. The full sweep re-runs all ~1200 CDOs just to
    // heal the handful that deferred a load — and measurement showed the sweep is AS-EXECUTION bound,
    // not IO bound. So by default we re-run ONLY the CDOs that actually deferred a load during
    // first-pass (GDeferredLoadCDOs). Fall back to the full sweep if attribution was uncertain or the
    // CVar forces it — Phase 2 below is always full, and this never leaves a CDO asset null.
    const auto UseFullHeal = GForceFullAssetHeal || GAttributionUncertain;
    const auto HealMode    = UseFullHeal ? FString(TEXT("full")) : FString(TEXT("surgical"));
    const auto CdoCount    = UseFullHeal ? ReRunAllClassDefaults() : ReRunDeferredClassDefaults();

#if WITH_EDITOR
    ck::core::Display(TEXT("[DeferredAssetInit] Phase 1: re-initialized {} Angelscript CDO(s) [{}]"), CdoCount, HealMode);
#else
    ck::core::Verbose(TEXT("[DeferredAssetInit] Phase 1: re-initialized {} Angelscript CDO(s) [{}]"), CdoCount, HealMode);
#endif

    // Phase 2: same surgical/full choice as Phase 1. Surgical re-runs only literals whose __Init
    // deferred during first-pass (GDeferredLiteralNames); full is the fallback.
    const auto LiteralAssetStats = ReRunLiteralAssetInits(UseFullHeal);
    if (LiteralAssetStats.Succeeded < LiteralAssetStats.Declared)
    {
        // Always Warning — a discrepancy here is a real signal worth surfacing in every config.
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

    // ONE aggregated line for assets::load::* calls that ran before engine-safe (first-pass CDO/
    // literal init), instead of a per-call stack-walking ensure (the old ~15ms-each storm). These
    // are expected and healed by this sweep, so it's informational — a non-zero count that surprises
    // you points at a soft-ref candidate. The cheap counter still catches genuine early misuse.
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
    GDeferredLoadCDOs.Reset();
    GDeferredLiteralNames.Reset();
    GAttributionUncertain = false;

    // Rooting of the resulting disregard→normal-pool refs happens in OnPreGarbageCollect (pre-GC), which
    // also catches refs that resolve lazily later in the session.

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

    // The initial-compile post-reload fires BEFORE OnFEngineLoopInitComplete (blocking loads still
    // unsafe). Running the sweep here heals nothing — every assets::load::* returns null — and would
    // fire a stack-walking ensure per call (the measured ~2.7s startup ensure storm). ResolveAllPending
    // (also bound to OnFEngineLoopInitComplete) is a strict superset and does the real heal. Use the
    // side-effect-free peek so this guard never trips the WasBlockingLoadQueriedWhileUnsafe short-circuit.
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

    // Hot-reload always re-runs ALL literals: first-pass attribution doesn't apply here (the reload
    // happens engine-safe, so accessors load directly and Note never fires), and the full re-run is
    // what fixes the `_Arr.Add(...)` doubling the reload would otherwise cause.
    constexpr auto FullHeal = true;
    const auto Stats = ReRunLiteralAssetInits(FullHeal);
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

    // Rooting happens in OnPreGarbageCollect (pre-GC), not here.

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
    RootAngelscriptDisregardViolations();
#endif
#endif
}

// --------------------------------------------------------------------------------------------------------------------
