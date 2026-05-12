#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

// --------------------------------------------------------------------------------------------------------------------

// AS bootstrap self-heal dispatcher (Rev 10).
//
// Bridges the error parser's typed roots to the appropriate recovery action.
// Two surface concerns are folded into one class:
//
//   * Pure classification (Classify, BuildActionPlan) — testable without any
//     engine or editor dependencies. Tests live in Test_Dispatcher.cpp.
//   * The OnReloadHadErrors entry point — invoked synchronously by the
//     AngelscriptCode plugin when a compile attempt fails. Pulls diagnostics,
//     classifies them, and *queues* the resulting actions for deferred apply
//     on the Slate modal-tick pump (FSlateApplication::GetOnModalLoopTickEvent).
//
// Why deferred — finding 2026-05-12: the hot-reload checker thread has not yet
// started when OnReloadHadErrors fires during initial compile failure. It
// starts when Hazelight opens its AS-failure modal, AFTER our hook returns.
// Its first scan establishes mtime baselines for every .as file. Any file we
// write before that first scan is invisible to the thread — the baseline IS
// our new content, so no future scan flags it as changed, and the modal never
// triggers a retry compile. By deferring file mutations to the modal-tick
// pump (which fires only after the modal is open and the thread is running),
// our writes happen between scans and are picked up cleanly on the next pass.
//
// Cycle cap: hard-capped at MaxCycles=3 per editor session (CTO Rev 10
// pushback #2). Each deferred-apply tick that successfully runs at least
// one strategy increments the counter; if the counter reaches the cap,
// subsequent OnReloadHadErrors invocations bail out immediately with a
// terminal banner. Reset_CyclesRun is called once from StartupModule when
// the hook is armed.
//
// v1 scope:
//   * SynthesizeStub_EntitySpawnParams is wired and active.
//   * KickGenerator_DynamicHandle and KickGenerator_AssetRegistry are
//     classified but their Apply_* path logs "deferred — manual
//     intervention required" and returns false. Future commits add the
//     async-pump machinery (CTO pushback #3) so these can call the real
//     generators and await their completion before retrying.

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
        // Hard cap on recovery cycles per editor session (CTO Rev 10 #2).
        static constexpr int32 MaxCycles = 3;

        // ---- Pure-logic surface ----------------------------------------------------

        // Returns the strategy that fits this root cause. Routes by error
        // Kind + identifier-shape heuristics (e.g. "U..." namespace prefix
        // means entity-script class; "FCk_Handle_..." identifier means
        // dynamic-handle; "assets" / "assets::..." namespace means asset
        // registry). Anything that doesn't fit returns Unrecognized.
        static auto
        Classify(
            const FCk_AsParsedError& InError) -> ECk_RecoveryStrategy;

        // Composes per-root actions from already-deduped roots. One action
        // per root; preserves the parser's deduplicated ordering. Unrecognized
        // actions are included so callers can surface them via banner.
        static auto
        BuildActionPlan(
            const TArray<FCk_AsParsedError>& InDedupedRoots) -> TArray<FCk_RecoveryAction>;

        // ---- Live entry point ------------------------------------------------------

        // Production hook handler. Called from the OnReloadHadErrors delegate
        // wire-up in StartupModule. Pulls live diagnostics via
        // FAngelscriptManager::Get().FormatDiagnostics(), parses them, builds
        // an action plan, and queues actions on an internal pending-list. A
        // subscription to FSlateApplication::Get().GetOnModalLoopTickEvent()
        // is established (idempotently) so the queue is drained from the modal-
        // tick pump after the Hazelight hot-reload thread has stabilized — see
        // the file-header comment for the timing rationale.
        //
        // Honors the cycle cap and bails out with a terminal-banner log when:
        //   * cycles >= MaxCycles, OR
        //   * the diagnostics yield zero recognized roots, OR
        //   * (at apply time) every queued action's Apply_* returned false.
        static auto
        OnAngelscriptReloadHadErrors() -> void;

        // ---- Cycle accounting (visible for tests + StartupModule arming) -----------

        static auto Get_CyclesRun() -> int32;
        static auto Reset_CyclesRun() -> void;

        // ---- Deferred JSON regen on PostEngineInit ---------------------------------

        // Was a DynamicHandleTypes.json stub synthesized during this editor session?
        // The stub has only TypeName + ShortName populated correctly; Description /
        // SourceAsset / RequiredFragments are placeholders. The session works (AS
        // compile succeeds, editor loads) but `RequiredFragments=[]` makes
        // CreateMultiFragmentValidator emit a PERMISSIVE validator — any handle
        // casts via As_<ShortName>() succeed regardless of actual fragments. To
        // restore strict validation, the JSON has to be regenerated (via
        // GenerateHandleTypeRegistry) and the editor restarted (the AS-side
        // registry skips re-registrations of existing types, so the in-memory
        // permissive validator persists for the current session).
        //
        // The Module's OnPostEngineInit callback reads this flag to decide
        // whether to call UCkDynamicHandleSubsystem::GenerateHandleTypeRegistry
        // (GEditor IS available at PostEngineInit, unlike at the modal-tick
        // recovery time). That makes next launch clean.
        static auto Did_SynthesizeJsonStub_ThisSession() -> bool;
        static auto Mark_JsonStubSynthesized()           -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
