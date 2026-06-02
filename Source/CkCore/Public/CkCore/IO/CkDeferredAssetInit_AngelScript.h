#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkDeferredAssetInit_AngelScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Deferred Asset Init
//
// Fixes assets::load:: returning nullptr when called during AS __InitDefaults / literal-asset
// init (engine init isn't yet safe for blocking loads), and the matching hot-reload case where
// the AS plugin re-runs __Init_<Name> on the same cached instance — `_Arr.Add(...)` in the
// body would otherwise double on every reload.
//
//   Phase 1 (boot only) — re-run each AS class's DefaultsFunction chain on its CDO. No
//     pre-reset: scalar/object-ref defaults are idempotent, and clearing first risks
//     abandoning state if a mid-chain statement aborts.
//   Phase 2 (boot + hot-reload) — for each literal asset, reset the cached instance from its
//     CDO then call __Init_<Name> directly (bypassing the getter's first-call cache).
// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCORE_API UCk_DeferredAssetInit_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DeferredAssetInit_UE);

    // OnFEngineLoopInitComplete — runs Phase 1 + Phase 2 with blocking-loads now safe.
    static void ResolveAllPending();

    // FAngelscriptClassGenerator::OnPostReload — runs Phase 2 only. Phase 1 is skipped because
    // the AS plugin updates CDOs in place during reload.
    static void OnAngelscriptPostReload(bool InFullReload);

    // Called from the AS premature-load helper (ck::EnsureIfNot_PrematureAssetLoad) on every
    // assets::load::* that returns null before engine-safe. Captures the exact CDO whose
    // DefaultsFunction is executing (via the active AS call stack) so the heal sweep can re-run
    // ONLY those CDOs instead of all ~1200. Ungated (runs in cook too). Cheap — no stack symbolication.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DeferredAssetInit",
              DisplayName = "[Ck] Note Deferred Asset Load (Active Context)")
    static void
    Note_DeferredAssetLoad_FromActiveContext();

    // FCoreUObjectDelegates::GetPreGarbageCollectDelegate — re-roots the disregard-for-GC violation targets
    // right before each collection (non-editor only; catches lazily-resolved refs). See the .cpp for the why.
    static void OnPreGarbageCollect();
};

// --------------------------------------------------------------------------------------------------------------------
