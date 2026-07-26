#pragma once

#include "CoreMinimal.h"

#include "CkAngelscriptGenerator/SelfHeal/CkAngelscriptGenerator_AsErrorParser.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::angelscriptgenerator::self_heal
{
    enum class ECk_RecoveryStrategy : uint8
    {
        SynthesizeStub_EntitySpawnParams,
        KickGenerator_DynamicHandle,
        KickGenerator_AssetRegistry,
        Quarantine_StaleEspCanonical,
        Author_FixupRequired_AdjacentStringLiteral,
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
        // Bootstrap-only cap; mid-session bypasses it (the user can intervene).
        static constexpr int32 MaxCycles = 3;

        static constexpr int32 MaxPerSignatureRepeats = 3;

        static auto Classify       (const FCk_AsParsedError& InError) -> ECk_RecoveryStrategy;
        static auto BuildActionPlan(const TArray<FCk_AsParsedError>& InDedupedRoots) -> TArray<FCk_RecoveryAction>;

        // Hook handler for FAngelscriptCodeModule::GetReloadHadErrors. Queues only —
        // applying strategies here is invisible to the hot-reload checker thread
        // (CkAngelscriptGenerator/CLAUDE.md § The modal-tick deferral).
        static auto OnAngelscriptReloadHadErrors() -> void;

        static auto Get_CyclesRun  () -> int32;
        static auto Reset_CyclesRun() -> void;

        // Drops queued-but-undrained actions and disarms the deferred drains. The
        // per-signature convergence trackers and blacklist deliberately survive.
        static auto Clear_PendingRecoveryState() -> void;

        static auto Is_BootstrapMode     () -> bool;
        static auto Mark_BootstrapComplete() -> void;

        static auto Did_SynthesizeJsonStub_ThisSession() -> bool;
        static auto Mark_JsonStubSynthesized          () -> void;

        static auto Did_SynthesizeAssetRegistryStub_ThisSession() -> bool;
        static auto Mark_AssetRegistryStubSynthesized          () -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
