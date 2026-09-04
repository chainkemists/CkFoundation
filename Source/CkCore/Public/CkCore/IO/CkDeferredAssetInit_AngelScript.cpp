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
        if (ck::Is_NOT_Valid(CDO) || CDO == InInstance)
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

        // The engine's own exception callback (LogAngelscriptException, AngelscriptManager.cpp) prints the
        // AngelScript callstack; this adds the C++-side identity of what we were driving, which that
        // callback cannot know.
        const auto* ExceptionString = Result == asEXECUTION_EXCEPTION ? InContext->GetExceptionString() : nullptr;

        ck::core::Error(TEXT("[DeferredAssetInit] {} failed: asExecutionResult=[{}]{}"),
                        InContextLabel, static_cast<int32>(Result),
                        ExceptionString != nullptr
                            ? ck::Format_UE(TEXT(" exception=[{}]"), ANSI_TO_TCHAR(ExceptionString))
                            : FString{});
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
                ck::IsValid(ThisObj))
            {
                for (auto* CheckClass = ThisObj->GetClass();
                     ck::IsValid(CheckClass);
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
    // Pre-clear capture: a packaged boot from the precompiled script cache runs the literal __Inits and
    // CDO defaults during AS init (deferring every assets::load::), then ClearUnneededRuntimeData EMPTIES
    // FAngelscriptManager::ActiveModules and every module's globalFunctionList before the sweep can run —
    // both phases enumerate nothing and every deferred field stays null (bodyless NPCs, dead item
    // traits). The metadata is alive at OnPostReload, which fires on BOTH boot
    // paths pre-clear, so snapshot what the sweep needs there. asCScriptFunction pointers survive the
    // clear (it deletes only unused SYSTEM functions; the handle registry banks pointers the same way).
    // The capture runs OFF the game thread in cooked builds — pointers and strings only, no UObject work.
    // ----------------------------------------------------------------------------------------------------------------

    struct FCapturedLiteralInit
    {
        FString            AssetName;
        asCScriptFunction* GetterFunction = nullptr;
        asCScriptFunction* InitFunction   = nullptr;
    };

    TArray<TWeakObjectPtr<UASClass>> GPreClearCapturedClasses;
    TArray<FCapturedLiteralInit>     GPreClearCapturedLiterals;

    auto CapturePreClearHealSources() -> void
    {
        GPreClearCapturedClasses.Reset();
        GPreClearCapturedLiterals.Reset();

        auto ActiveModules = FAngelscriptManager::Get().GetActiveModules();

        ck::algo::ForEach(ActiveModules, [&](const TSharedRef<FAngelscriptModuleDesc>& Module)
        {
            ck::algo::ForEach(Module->Classes, [&](const TSharedRef<FAngelscriptClassDesc>& ClassDesc)
            {
                if (auto* ASClass = Cast<UASClass>(ClassDesc->Class))
                { GPreClearCapturedClasses.Emplace(ASClass); }
            });

            if (Module->ScriptModule == nullptr)
            { return; }

            // Names derive from PostInitFunctions' Get<Name> getters — the field the precompiled cache
            // RESTORES. DeclaredLiteralAssets is populated by the source-path preprocessor only.
            if (Module->PostInitFunctions.IsEmpty())
            { return; }

            const auto FunctionMap = BuildFunctionMap(Module->ScriptModule);

            ck::algo::ForEach(Module->PostInitFunctions, [&](const FString& GetterName)
            {
                static const auto GetterPrefix = FString{TEXT("Get")};
                if (NOT GetterName.StartsWith(GetterPrefix))
                { return; }

                const auto AssetName = GetterName.RightChop(GetterPrefix.Len());

                auto Captured = FCapturedLiteralInit{};
                Captured.AssetName      = AssetName;
                Captured.GetterFunction = FunctionMap.FindRef(GetterName);
                Captured.InitFunction   = FunctionMap.FindRef(ck::Format_UE(TEXT("__Init_{}"), AssetName));

                if (Captured.GetterFunction == nullptr || Captured.InitFunction == nullptr)
                { return; }

                GPreClearCapturedLiterals.Emplace(MoveTemp(Captured));
            });
        });
    }

    // ----------------------------------------------------------------------------------------------------------------
    // Phase 1 RESETS each CDO to its constructed state and then re-applies its defaults: script destructor
    // -> ConstructFunction -> the DefaultsFunction chain (collected child->parent, executed in reverse,
    // each in its own context so one failure does not skip siblings). Same three steps, same order, as the
    // engine's own UASClass::ReconstructScriptObject (ASClass.cpp) — spelled out here so every step's
    // result is checked; see WHICH DOOR below.
    //
    // THE RESET IS NOT OPTIONAL. A DefaultsFunction is an INITIALISER, not an idempotent apply:
    // `default _Arr.Add(x)` appends again on every re-run. An earlier revision skipped it on the grounds
    // that AS defaults are "almost" all scalar/object-ref assignments — and the exception cost a real
    // behaviour defect. A script class seeded a fixed-length work list with three
    // `default _List.Add(...)` statements; the doubled CDO list became six entries, and
    // UCk_ObjectPooling_Subsystem_UE::Request_ResetToArchetype copies script-only members from a CDO onto
    // every RECYCLED pooled instance, so every recycled instance ran its whole sequence twice. A consuming
    // project had 54 such sites across 23 files. Reproducible with StaticJIT off via
    // `ck.DeferredAssetInit.ForceFullHeal 1` — the JIT is not the cause, it just makes the full sweep
    // unconditional (attribution needs an AS callstack, and jitted frames push none). The SURGICAL path
    // double-applies too, so the reset is required in both modes.
    //
    // Phase 2 already guards its half of exactly this (ResetInstanceFromCDO, above). A CDO has no archetype
    // to copy from, so its pristine state has to be re-made — which is what the script constructor is for.
    //
    // WHICH DOOR TO THE DESTRUCTOR: `asIScriptObject::CallDestructor` is declared on the fork's
    // devirtualized, UNEXPORTED interface class — calling it from CkCore links in a monolithic Game build
    // and fails the modular editor DLL outright ("unresolved external symbol"). Use
    // `UASClass::RuntimeDestroyObject`, a public member of the exported UASClass whose entire body is the
    // null-check plus that same CallDestructor (ASClass.cpp). `UASClass::ReconstructScriptObject` also
    // links, but it runs the defaults chain itself AND discards every Execute() result — which would make
    // the boot line "Phase 1: re-initialized N CDO(s)" unable to distinguish a heal from a failure, and
    // would leave the abort hazard below undetectable. Same module-boundary constraint the ObjectPooling
    // reset hit from the other side, where the exported symbol it could reach was
    // asCScriptObject::PerformCopy.
    //
    // THE RESET IS DELIBERATELY PARTIAL, and that is load-bearing: the generated destructor skips
    // primitives, references and handles, and the generated constructor touches those only where they carry
    // an explicit initialiser. Containers, structs and strings come back pristine while handles KEEP their
    // prior value — which is why an actor CDO's DefaultComponents (written at their script VariableOffset
    // BEFORE the constructor runs) survive. Do not "improve" this into a full zeroing reset.
    //
    // The other thing a reset like this would wipe is a CDO value written after construction by something
    // other than defaults — LoadConfig is the canonical case. There are currently ZERO AngelScript
    // `UPROPERTY(Config)` / `UCLASS(Config=...)` declarations under any Script root, so that is latent
    // rather than live; if it ever changes, this needs to become a config-property-preserving reset.
    // ----------------------------------------------------------------------------------------------------------------

    // True when the CDO was reset and its defaults re-applied.
    auto ReRunClassDefaultsFor(UASClass* InASClass) -> bool
    {
        if (ck::Is_NOT_Valid(InASClass))
        { return false; }

        if (InASClass->DefaultsFunction == nullptr)
        { return false; }

        if (InASClass->HasAnyClassFlags(CLASS_Abstract | CLASS_NewerVersionExists))
        { return false; }

        // A UASClass with a null script type is a hot-reload corpse: nothing to destroy, nothing to
        // reconstruct. (Unreachable in practice: the sites that null ScriptTypePtr null DefaultsFunction
        // too.)
        if (InASClass->ScriptTypePtr == nullptr)
        { return false; }

        // Required to rebuild what the destructor below tears down. Bail BEFORE destroying, never after.
        if (InASClass->ConstructFunction == nullptr)
        { return false; }

        auto* CDO = InASClass->GetDefaultObject(ShouldCreateCDO);
        if (ck::Is_NOT_Valid(CDO))
        { return false; }

        // Names the classes the sweep actually touches. The COUNT alone cannot answer "could this sweep
        // have changed subsystem X's behaviour", which is the question every regression triage against
        // this code asks; the surgical set is small enough that the answer is a grep.
        ck::core::Verbose(TEXT("[DeferredAssetInit] Phase 1 re-init: '{}'"), InASClass->GetName());

        // ---- reset ------------------------------------------------------------------------------------
        // RuntimeDestroyObject is the destructor-only door CkCore can reach: its whole body is the
        // null-check plus asCScriptObject::CallDestructor (ASClass.cpp), and it is a public member of the
        // exported UASClass. Calling CallDestructor directly does NOT link the modular editor DLL.
        InASClass->RuntimeDestroyObject(CDO);

        // From here to the end of the constructor the CDO's script members are destroyed. There is no safe
        // bail inside that window - which is why the failure below is an ensure, not a quiet return.
        const auto ConstructSucceeded = [&]
        {
            auto Context = FAngelscriptContext{CDO};
            Context->Prepare(InASClass->ConstructFunction);
            Context->SetObject(CDO);
            return Execute_Logging(Context, ck::Format_UE(TEXT("ConstructFunction for class '{}'"),
                                                          InASClass->GetName()));
        }();

        CK_ENSURE_IF_NOT(ConstructSucceeded,
            TEXT("[DeferredAssetInit] ConstructFunction for class '{}' FAILED after its CDO's script members "
                 "were destroyed. That CDO is unusable and everything copying from it - every pooled recycle's "
                 "PerformCopy, every GetDefaultObject reader - now reads destroyed state"), InASClass->GetName())
        { return false; }

        // ---- re-apply defaults ------------------------------------------------------------------------
        auto DefaultsFunctions = TArray<asIScriptFunction*, TFixedAllocator<32>>{};
        for (auto* WalkClass = InASClass; ck::IsValid(WalkClass); WalkClass = Cast<UASClass>(WalkClass->GetSuperClass()))
        {
            if (WalkClass->DefaultsFunction != nullptr)
            { DefaultsFunctions.Add(WalkClass->DefaultsFunction); }
        }

        // Deliberately invoked independently - a failure in one must NOT skip the rest, matching the
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

        // Newly load-bearing because of the reset above: before it, an aborting statement left every LATER
        // target holding its previous, already-initialised value. Now it leaves them at constructed
        // defaults, so a partial chain is a half-initialised CDO rather than a mostly-correct one.
        CK_ENSURE_IF_NOT(AllOk,
            TEXT("[DeferredAssetInit] A DefaultsFunction in class '{}'s chain FAILED after its CDO was reset. "
                 "The CDO now holds the statements that ran and constructed defaults for the rest"),
            InASClass->GetName())
        { return false; }

        return true;
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

        if (ActiveModules.Num() == 0 && NOT GPreClearCapturedClasses.IsEmpty())
        {
            // Precompiled-cache boot: the module registry was emptied post-init — heal from the
            // pre-clear capture instead (see the capture block above).
            ck::algo::ForEach(GPreClearCapturedClasses, [&](const TWeakObjectPtr<UASClass>& WeakClass)
            {
                if (ReRunClassDefaultsFor(WeakClass.Get()))
                { ++SucceededCount; }
            });
        }

        return SucceededCount;
    }

    // Returns the number of distinct CDOs re-run.
    auto ReRunDeferredClassDefaults() -> int32
    {
        auto SucceededCount = int32{0};

        for (const auto& WeakCdo : GDeferredLoadCDOs)
        {
            auto* CDO = WeakCdo.Get();
            if (ck::Is_NOT_Valid(CDO))
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

    // Getter → cached instance, reset to CDO, then __Init_ re-executes the user's asset body.
    auto ReRunOneLiteralInit(
        const FString& InAssetName,
        asCScriptFunction* InGetterFunction,
        asCScriptFunction* InInitFunction) -> bool
    {
        UObject* AssetInstance = nullptr;
        {
            auto Context = FAngelscriptContext{};
            Context->Prepare(InGetterFunction);
            if (NOT Execute_Logging(Context, ck::Format_UE(TEXT("Get{}"), InAssetName)))
            { return false; }

            AssetInstance = *static_cast<UObject**>(Context->GetAddressOfReturnValue());
        }

        if (ck::Is_NOT_Valid(AssetInstance))
        {
            ck::core::Error(TEXT("[DeferredAssetInit] Literal asset '{}' — getter returned null/invalid instance"), InAssetName);
            return false;
        }

        ResetInstanceFromCDO(AssetInstance);

        {
            auto Context = FAngelscriptContext{};
            Context->Prepare(InInitFunction);
            Context->SetArgObject(0, AssetInstance);
            if (NOT Execute_Logging(Context, ck::Format_UE(TEXT("__Init_{}"), InAssetName)))
            { return false; }
        }

        return true;
    }

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

                if (ReRunOneLiteralInit(AssetName, GetterFunction, InitFunction))
                { ++Stats.Succeeded; }
            });
        });

        if (Stats.Declared == 0 && NOT GPreClearCapturedLiterals.IsEmpty())
        {
            // Precompiled-cache boot: DeclaredLiteralAssets is never restored and the module registry
            // is emptied post-init, so the walk above declared nothing — heal from the pre-clear
            // capture. The FULL captured set runs regardless of InFullHeal: surgical attribution
            // records names from an AS stack the cache path does not surface.
            ck::algo::ForEach(GPreClearCapturedLiterals, [&](const FCapturedLiteralInit& InCaptured)
            {
                ++Stats.Declared;
                if (ReRunOneLiteralInit(InCaptured.AssetName, InCaptured.GetterFunction, InCaptured.InitFunction))
                { ++Stats.Succeeded; }
            });
        }

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
        // A deferred count with ZERO heals means the enumeration came up empty and every deferred
        // field is still null (bodyless NPCs, dead item data) — that combination shipped silently
        // once and must never again. Loud in every config; the counts name the missing source.
        const auto SweepHealedSomething = CdoCount > 0 || LiteralAssetStats.Succeeded > 0;
        CK_ENSURE_IF_NOT(SweepHealedSomething,
            TEXT("[DeferredAssetInit] {} assets::load::* call(s) deferred before engine-safe but the sweep healed NOTHING "
                 "(CDOs=[{}], literals=[{}/{}], ActiveModules=[{}], captured classes=[{}], captured literals=[{}])"),
            PrematureCount, CdoCount, LiteralAssetStats.Succeeded, LiteralAssetStats.Declared,
            FAngelscriptManager::Get().GetActiveModules().Num(),
            ck_deferred_asset_init_angelscript::GPreClearCapturedClasses.Num(),
            ck_deferred_asset_init_angelscript::GPreClearCapturedLiterals.Num())
        {}

        // Display in EVERY config — the heal-vs-deferred delta is the one line that says whether AS
        // asset data survived boot, and Verbose hid it on the packaged build that shipped broken.
        ck::core::Display(TEXT("[DeferredAssetInit] {} assets::load::* call(s) deferred before engine-safe; sweep re-ran {} CDO(s) + {}/{} literal(s) (first deferred: '{}')"),
                          PrematureCount, CdoCount, LiteralAssetStats.Succeeded, LiteralAssetStats.Declared,
                          UCk_Utils_IO_UE::Get_FirstPrematureAssetLoadMessage());
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

    // Snapshot the sweep's enumeration sources while the module registry is still alive — a
    // precompiled-cache boot empties it (ClearUnneededRuntimeData) before ResolveAllPending runs.
    // Fires on both boot paths and on hot-reload, so the capture also refreshes stale pointers.
    ck_deferred_asset_init_angelscript::CapturePreClearHealSources();

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
