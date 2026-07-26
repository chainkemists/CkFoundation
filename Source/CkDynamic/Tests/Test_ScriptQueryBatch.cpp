// Tests for the script-query batch mixin — the PURE state/generation logic that needs no registry.
// GetHandle and the wildcard Get need a live registry; their error paths are covered by the AS AutoTests.

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

    const auto LiveGeneration = ck::dynamic::Open_ScriptQueryBatchState(State);
    TestNotEqual(TEXT("open returns a live generation"), LiveGeneration, uint64{0});

    auto Live = FCk_ScriptQueryBatch{};
    Live._State = &State;
    Live._Generation = LiveGeneration;
    TestEqual(TEXT("live batch Num == entity count"), UCk_ScriptQueryBatch_Mixin_UE::Num(Live), 3);

    auto Stale = FCk_ScriptQueryBatch{};
    Stale._State = &State;
    Stale._Generation = LiveGeneration + 1;
    TestEqual(TEXT("stale-generation batch Num == 0"), UCk_ScriptQueryBatch_Mixin_UE::Num(Stale), 0);

    const auto Empty = FCk_ScriptQueryBatch{};
    TestEqual(TEXT("null-state batch Num == 0"), UCk_ScriptQueryBatch_Mixin_UE::Num(Empty), 0);

    ck::dynamic::Close_ScriptQueryBatchState(State, LiveGeneration);
    TestEqual(TEXT("post-close the once-live batch Num == 0"), UCk_ScriptQueryBatch_Mixin_UE::Num(Live), 0);

    return true;
}

#endif
