#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkDeferredAssetInit_AngelScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// assets::load:: returns nullptr during AS __InitDefaults (blocking loads aren't safe that early), and
// hot-reload re-runs __Init_<Name> on the same cached instance. Phase 1 (boot only) re-runs each AS
// class's DefaultsFunction on its CDO; Phase 2 resets each literal asset from its CDO and re-inits it.
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
    // assets::load::* that returns null before engine-safe: captures the CDO whose DefaultsFunction is
    // executing, so the heal sweep re-runs ONLY those CDOs instead of all ~1200. Runs in cook too.
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
