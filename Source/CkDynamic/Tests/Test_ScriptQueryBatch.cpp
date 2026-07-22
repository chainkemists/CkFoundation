// Tests for the script-query batch mixin (UCk_ScriptQueryBatch_Mixin_UE over FCk_ScriptQueryBatch).
//
// Coverage here is the PURE state/generation logic that needs no registry: Num reflects the snapshotted entity
// count while the batch's captured generation is registered with the process-lifetime resolver, and reports 0
// (graceful empty, no crash) for a stale-generation or null-state batch.
//
// GetHandle and the wildcard Get require a live entt registry + storage and exercise the ensure+sentinel error
// paths (out-of-range, wrong-type, removed-mid-batch, stashed-batch). Those are covered end-to-end by the AS
// AutoTests (typed iteration + mid-batch-remove / stashed-batch), which run against a real world.

#include "CkDynamic/CkDynamic_ScriptQueryBatch.h"

#include "CkEcs/Processor/CkProcessor_ScriptQuery_Data.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ScriptQueryBatch_NumAndGenerationGuard,
    "Ck.CkDynamic.ScriptQueryBatch.NumAndGenerationGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ScriptQueryBatch_NumAndGenerationGuard::RunTest(const FString&)
{
    auto State = FCk_ScriptQueryBatchState{};
    State._Entities.Add(static_cast<entt::entity>(1));
    State._Entities.Add(static_cast<entt::entity>(2));
    State._Entities.Add(static_cast<entt::entity>(3));

    // Live batch: Open registers this exact state/generation pair with the resolver.
    const auto LiveGeneration = ck::dynamic::Open_ScriptQueryBatchState(State);
    TestNotEqual(TEXT("open returns a live generation"), LiveGeneration, uint64{0});

    auto Live = FCk_ScriptQueryBatch{};
    Live._State = &State;
    Live._Generation = LiveGeneration;
    TestEqual(TEXT("live batch Num == entity count"), UCk_ScriptQueryBatch_Mixin_UE::Num(Live), 3);

    // A different generation at the same address must not resolve while the live pair is registered.
    auto Stale = FCk_ScriptQueryBatch{};
    Stale._State = &State;
    Stale._Generation = LiveGeneration + 1;
    TestEqual(TEXT("stale-generation batch Num == 0"), UCk_ScriptQueryBatch_Mixin_UE::Num(Stale), 0);

    // Default-constructed batch: null state -> Num reports 0.
    const auto Empty = FCk_ScriptQueryBatch{};
    TestEqual(TEXT("null-state batch Num == 0"), UCk_ScriptQueryBatch_Mixin_UE::Num(Empty), 0);

    // Close unregisters the pair before the host returns, retroactively staleifying a batch that script stashed.
    ck::dynamic::Close_ScriptQueryBatchState(State, LiveGeneration);
    TestEqual(TEXT("post-close the once-live batch Num == 0"), UCk_ScriptQueryBatch_Mixin_UE::Num(Live), 0);

    return true;
}

#endif
