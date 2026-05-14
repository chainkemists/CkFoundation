#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

// --------------------------------------------------------------------------------------------------------------------

// AS bootstrap self-heal dispatcher (Rev 10).
//
// Bridges the error parser's typed roots to the appropriate recovery action.
// Two surface concerns are folded into one class:
//   * Pure classification (Classify, BuildActionPlan) — testable without any
//     engine/editor dependencies (see Test_Dispatcher.cpp).
//   * OnReloadHadErrors entry point — invoked synchronously by the
//     AngelscriptCode plugin on compile failure. Pulls diagnostics, classifies,
//     and *queues* actions for deferred apply.
//
// Why deferred — empirical finding 2026-05-12: the hot-reload checker thread
// has not yet started when OnReloadHadErrors fires during initial-compile
// failure. It starts when Hazelight opens its AS-failure modal, AFTER our hook
// returns. Its first scan establishes mtime baselines for every .as file. Any
// file we write before that first scan is invisible to the thread — the
// baseline IS our new content, so no future scan flags it as changed, and the
// modal never triggers a retry compile. Bootstrap deferral hooks
// FSlateApplication::GetOnModalLoopTickEvent — fires only after the modal is
// open and the thread is running. Mid-session deferral (Issue #7, 2026-05-13)
// uses FTSTicker because hot-reload failures mid-session don't open a modal,
// so the modal-tick pump never fires.

namespace ck::angelscriptgenerator::self_heal
{
    enum class ECk_RecoveryStrategy : uint8
    {
        SynthesizeStub_EntitySpawnParams,
        KickGenerator_DynamicHandle,
        KickGenerator_AssetRegistry,
        Unrecognized,
    };

    struct CKANGELSCRIPTGENERATOR_API FCk_RecoveryAction
    {
        ECk_RecoveryStrategy Strategy = ECk_RecoveryStrategy::Unrecognized;
        FCk_AsParsedError    Error;
    };

    class CKANGELSCRIPTGENERATOR_API FCkAsRecoveryDispatcher
    {
    public:
        // Hard cap on recovery cycles per editor session. Bootstrap-only —
        // cold-start runaway wedges the editor behind the AS-failure modal
        // indefinitely, so hard-stop after MaxCycles. Mid-session is
        // interactive (user can intervene) so the cap is bypassed and the
        // counter is reset at Mark_BootstrapComplete.
        static constexpr int32 MaxCycles = 3;

        // ---- Pure-logic surface ----------------------------------------------------

        // Routes by error Kind + identifier-shape heuristics:
        //   "U..." namespace + "Params" function -> SynthesizeStub_EntitySpawnParams
        //   "FCk_Handle_..." missing identifier  -> KickGenerator_DynamicHandle
        //   "assets" / "assets::..." namespace   -> KickGenerator_AssetRegistry
        //   anything else                        -> Unrecognized
        static auto
        Classify(
            const FCk_AsParsedError& InError) -> ECk_RecoveryStrategy;

        // One action per deduped root, preserving the parser's deduplicated
        // ordering. Unrecognized actions are included so callers can surface
        // them via banner.
        static auto
        BuildActionPlan(
            const TArray<FCk_AsParsedError>& InDedupedRoots) -> TArray<FCk_RecoveryAction>;

        // ---- Live entry point ------------------------------------------------------

        // Hook handler called from FAngelscriptCodeModule::GetReloadHadErrors.
        // Pulls live diagnostics, parses, classifies, queues, and subscribes
        // to either the modal-tick pump (bootstrap) or FTSTicker (mid-session)
        // for deferred apply.
        //
        // Bails out with a terminal banner when cycle cap is exceeded
        // (bootstrap only), or when diagnostics yield zero recognized roots,
        // or when every queued action's Apply_* returned false.
        static auto
        OnAngelscriptReloadHadErrors() -> void;

        // ---- Cycle accounting ------------------------------------------------------

        static auto Get_CyclesRun() -> int32;
        static auto Reset_CyclesRun() -> void;

        // ---- Bootstrap-vs-mid-session mode -----------------------------------------

        // Returns true if the editor has not yet finished FEngineLoop init.
        // Used by OnAngelscriptReloadHadErrors to decide between modal-tick
        // (bootstrap — Hazelight modal active) and core-ticker (mid-session
        // hot-reload — no modal opens). The Module's
        // OnFEngineLoopInitComplete lambda calls Mark_BootstrapComplete to
        // flip this flag.
        static auto Is_BootstrapMode()      -> bool;
        static auto Mark_BootstrapComplete() -> void;

        // ---- Session-flag accessors (consumed by deferred regens) ------------------

        // Was a DynamicHandle stub synthesized this session? The synthesized
        // sibling JSON has empty RequiredFragments, which produces a PERMISSIVE
        // validator until the deferred OnPostEngineInit regen + UpdateExisting-
        // DynamicHandle path upgrades it to strict (sub-second window after
        // editor reaches main screen). The Module's OnPostEngineInit lambda
        // reads this flag to decide whether to call GenerateHandleTypeRegistry.
        static auto Did_SynthesizeJsonStub_ThisSession() -> bool;
        static auto Mark_JsonStubSynthesized()           -> void;

        // Was an AssetRegistry stub synthesized this session? Even Tier 1/2
        // success may have landed the stub in the wrong file (file-scan picks
        // the first namespace match before the asset-on-disk lookup picks the
        // owning file). The Module's OnPostEngineInit lambda reads this flag
        // to decide whether to invoke GenerateAllAssetRegistries, which
        // reshuffles into the correct file and overwrites the sibling stub.
        static auto Did_SynthesizeAssetRegistryStub_ThisSession() -> bool;
        static auto Mark_AssetRegistryStubSynthesized()           -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
