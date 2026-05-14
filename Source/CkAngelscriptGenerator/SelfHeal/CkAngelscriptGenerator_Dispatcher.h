#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

// --------------------------------------------------------------------------------------------------------------------

// AS bootstrap self-heal dispatcher (Rev 10). Bridges parsed AS-compile-error
// roots to recovery actions and defers their apply.
//
// Why deferred — empirical finding 2026-05-12: the hot-reload checker thread
// has not yet started when OnReloadHadErrors fires during initial-compile
// failure. It starts when Hazelight opens its AS-failure modal, AFTER our hook
// returns. Its first scan establishes mtime baselines for every .as file, so
// any file we write before that first scan is invisible — the baseline IS
// our new content, no future scan flags it as changed, and the modal never
// triggers a retry compile.
//   * Bootstrap: hook FSlateApplication::GetOnModalLoopTickEvent — fires
//     after the modal is open and the thread is running.
//   * Mid-session (Issue #7, 2026-05-13): hot-reload failures don't open a
//     modal, so use FTSTicker instead.
//
// See CkAngelscriptGenerator/Claude.md for the full architecture.

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
        // Bootstrap-only. Cold-start runaway would wedge the editor behind
        // the AS modal indefinitely; hard-stop after MaxCycles. Mid-session
        // bypasses the cap (user can intervene) and resets it at
        // Mark_BootstrapComplete.
        static constexpr int32 MaxCycles = 3;

        static auto Classify       (const FCk_AsParsedError& InError) -> ECk_RecoveryStrategy;
        static auto BuildActionPlan(const TArray<FCk_AsParsedError>& InDedupedRoots) -> TArray<FCk_RecoveryAction>;

        // Hook handler for FAngelscriptCodeModule::GetReloadHadErrors. Parses,
        // classifies, queues, and subscribes to modal-tick (bootstrap) or
        // FTSTicker (mid-session). Terminal banner on cycle-cap / zero roots /
        // all-actions-failed.
        static auto OnAngelscriptReloadHadErrors() -> void;

        static auto Get_CyclesRun  () -> int32;
        static auto Reset_CyclesRun() -> void;

        // Routes OnAngelscriptReloadHadErrors to modal-tick vs FTSTicker.
        // Flipped by the Module's OnFEngineLoopInitComplete lambda.
        static auto Is_BootstrapMode     () -> bool;
        static auto Mark_BootstrapComplete() -> void;

        // DynamicHandle: synthesized JSON stub has empty RequiredFragments,
        // which produces a PERMISSIVE validator until OnPostEngineInit regen
        // + UpdateExistingDynamicHandle upgrade it to strict. Sub-second
        // window. Module reads this to decide whether to call
        // GenerateHandleTypeRegistry deferred.
        static auto Did_SynthesizeJsonStub_ThisSession() -> bool;
        static auto Mark_JsonStubSynthesized          () -> void;

        // AssetRegistry: even Tier 1/2 success may have landed the stub in
        // the wrong file (file-scan picks first match before asset-on-disk
        // lookup picks the owner). Module reads this to decide whether to
        // call GenerateAllAssetRegistries deferred for reshuffle.
        static auto Did_SynthesizeAssetRegistryStub_ThisSession() -> bool;
        static auto Mark_AssetRegistryStubSynthesized          () -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
