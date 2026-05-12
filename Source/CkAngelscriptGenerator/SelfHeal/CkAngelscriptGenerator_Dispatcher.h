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
//     AngelscriptCode plugin when a compile attempt fails. This is the
//     production path; it pulls live diagnostics from FAngelscriptManager
//     and applies whatever strategies fit.
//
// Cycle cap: hard-capped at MaxCycles=3 per editor session (CTO Rev 10
// pushback #2). Each successful dispatch increments the counter; if the
// counter reaches the cap, subsequent OnReloadHadErrors invocations bail
// out immediately with a terminal banner. Reset_CyclesRun is called once
// from StartupModule when the hook is armed.
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
        // an action plan, applies the implemented strategies, and (on at
        // least one successful application) calls
        // FAngelscriptManager::CheckForHotReload(FullReload) to retry compile.
        //
        // Honors the cycle cap and bails out with a terminal-banner log when:
        //   * cycles >= MaxCycles, OR
        //   * the diagnostics yield zero recognized roots, OR
        //   * every root mapped to a strategy whose Apply_* returned false
        //     (e.g. all Unrecognized, or all deferred-strategies).
        static auto
        OnAngelscriptReloadHadErrors() -> void;

        // ---- Cycle accounting (visible for tests + StartupModule arming) -----------

        static auto Get_CyclesRun() -> int32;
        static auto Reset_CyclesRun() -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
