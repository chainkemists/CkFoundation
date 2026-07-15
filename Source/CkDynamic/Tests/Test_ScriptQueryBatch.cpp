// Tests for the script-query batch mixin (UCk_ScriptQueryBatch_Mixin_UE over FCk_ScriptQueryBatch).
//
// Coverage here is the PURE state/generation logic that needs no registry: Num reflects the snapshotted entity
// count when the batch's captured generation matches the live state, and reports 0 (graceful empty, no crash)
// for a stale-generation or null-state batch.
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
    State._Generation = 7;
    State._Entities.Add(static_cast<entt::entity>(1));
    State._Entities.Add(static_cast<entt::entity>(2));
    State._Entities.Add(static_cast<entt::entity>(3));

    // Live batch: captured generation matches the state -> Num reflects the snapshotted entity count.
    auto Live = FCk_ScriptQueryBatch{};
    Live._State = &State;
    Live._Generation = 7;
    TestEqual(TEXT("live batch Num == entity count"), UCk_ScriptQueryBatch_Mixin_UE::Num(Live), 3);

    // Stashed batch: captured generation is behind the live state -> Num reports 0 (graceful, no crash).
    auto Stale = FCk_ScriptQueryBatch{};
    Stale._State = &State;
    Stale._Generation = 6;
    TestEqual(TEXT("stale-generation batch Num == 0"), UCk_ScriptQueryBatch_Mixin_UE::Num(Stale), 0);

    // Default-constructed batch: null state -> Num reports 0.
    const auto Empty = FCk_ScriptQueryBatch{};
    TestEqual(TEXT("null-state batch Num == 0"), UCk_ScriptQueryBatch_Mixin_UE::Num(Empty), 0);

    // Bumping the live state's generation (as the host does after each ForEachBatch) retroactively staleifies the
    // previously-live batch — the core mechanism that makes a stashed batch safe.
    State._Generation = 8;
    TestEqual(TEXT("post-bump the once-live batch Num == 0"), UCk_ScriptQueryBatch_Mixin_UE::Num(Live), 0);

    return true;
}

#endif
