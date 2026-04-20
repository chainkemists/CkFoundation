#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkDeferredAssetInit_AngelScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Deferred Asset Init
//
// Solves the problem of assets::load:: returning nullptr when called during Angelscript
// __InitDefaults / literal-asset init (engine init isn't yet safe for blocking loads).
//
// On FCoreDelegates::OnFEngineLoopInitComplete we:
//   - Force the blocking-load safety flag true (order-independent across cross-TU statics).
//   - Short-circuit if no caller ever queried IsEngineSafeForBlockingLoads() while unsafe —
//     nothing was deferred, no work to do.
//   - Phase 1: walk AS classes via FAngelscriptManager modules and re-run their DefaultsFunction
//     chain on each CDO. Scalar/object-ref assignments are idempotent so no pre-reset is done
//     (pre-resetting class-owned properties risked clobbering state when a mid-chain statement
//     aborted before the reassignment).
//   - Phase 2: iterate each module's DeclaredLiteralAssets, reset the cached instance from its
//     CDO (so .Add()-style statements in the body don't double), and call __Init_<AssetName>
//     directly — bypassing the getter's first-call cache.
//
// The AS plugin's own hot-reload path already recreates CDOs/literal assets with working
// blocking-loads, so we don't bind a separate hook for that.
// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCORE_API UCk_DeferredAssetInit_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DeferredAssetInit_UE);

    // Called on OnFEngineLoopInitComplete. Re-runs Phase 1 (CDO defaults) and Phase 2
    // (literal asset __Init_ functions) with blocking-loads now safe.
    static void ResolveAllPending();
};

// --------------------------------------------------------------------------------------------------------------------
