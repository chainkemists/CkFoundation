// Tests for the script-processor query mixin: slot admission, all-or-nothing validation, generation resolver.

#include "CkDynamic/CkDynamic_ScriptQuery.h"
#include "CkDynamic/CkDynamic_ScriptQueryBatch.h"

#include "CkCore/Math/ValueRange/CkValueRange.h"

#include "CkEcs/Processor/CkProcessor_ScriptQuery_Data.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ScriptQuery_SlotAccumulationAndAccess,
    "Ck.CkDynamic.ScriptQuery.SlotAccumulationAndAccess",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ScriptQuery_RejectedSlotFailsWholeQuery,
    "Ck.CkDynamic.ScriptQuery.RejectedSlotFailsWholeQuery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ScriptQuery_SafeThenInvalidValidationIsAtomic,
    "Ck.CkDynamic.ScriptQuery.SafeThenInvalidValidationIsAtomic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ScriptQueryBatch_StaleGenerationNeverResolves,
    "Ck.CkDynamic.ScriptQuery.Batch.StaleGenerationNeverResolves",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ScriptQuery_SlotAccumulationAndAccess::RunTest(const FString&)
{
    // GC-independent reflected structs: slot admission rejects a struct holding strong UObject references.
    const auto* StructA = FCk_FloatRange::StaticStruct();
    const auto* StructB = FCk_IntRange::StaticStruct();
    const auto* StructC = FCk_FloatRange_0to1::StaticStruct();
    const auto* StructD = FCk_FloatRange_Minus1to1::StaticStruct();

    auto Query = FCk_ScriptProcessorQuery{};
    UCk_ScriptProcessorQuery_Mixin_UE::ReadWrite(Query, StructA);
    UCk_ScriptProcessorQuery_Mixin_UE::ReadOnly (Query, StructB);
    UCk_ScriptProcessorQuery_Mixin_UE::Require  (Query, StructC);
    UCk_ScriptProcessorQuery_Mixin_UE::Exclude  (Query, StructD);

    TestEqual(TEXT("four slots accumulated"), Query._Slots.Num(), 4);
    if (Query._Slots.Num() == 4)
    {
        TestTrue(TEXT("slot 0 = A / ReadWrite"),
            Query._Slots[0]._StructType == StructA && Query._Slots[0]._Access == ECk_ScriptQueryAccess::ReadWrite);
        TestTrue(TEXT("slot 1 = B / ReadOnly"),
            Query._Slots[1]._StructType == StructB && Query._Slots[1]._Access == ECk_ScriptQueryAccess::ReadOnly);
        TestTrue(TEXT("slot 2 = C / Require"),
            Query._Slots[2]._StructType == StructC && Query._Slots[2]._Access == ECk_ScriptQueryAccess::Require);
        TestTrue(TEXT("slot 3 = D / Exclude"),
            Query._Slots[3]._StructType == StructD && Query._Slots[3]._Access == ECk_ScriptQueryAccess::Exclude);
    }
    TestFalse(TEXT("NoEntities flag default false"), Query._NoEntities);
    TestFalse(TEXT("successful declarations leave admission valid"), Query._AdmissionFailed);

    auto NoEntQuery = FCk_ScriptProcessorQuery{};
    UCk_ScriptProcessorQuery_Mixin_UE::NoEntities(NoEntQuery);
    TestTrue (TEXT("NoEntities sets the escape flag"), NoEntQuery._NoEntities);
    TestEqual(TEXT("NoEntities adds no slots"), NoEntQuery._Slots.Num(), 0);

    return true;
}

bool FCkTest_ScriptQuery_RejectedSlotFailsWholeQuery::RunTest(const FString&)
{
    auto Query = FCk_ScriptProcessorQuery{};
    UCk_ScriptProcessorQuery_Mixin_UE::ReadOnly(Query, FCk_FloatRange::StaticStruct());

    // The query carrier itself owns a reflected TObjectPtr array — deliberately unsafe for untraced storage.
    AddExpectedError(TEXT("Unsafe Dynamic Fragment schema"), EAutomationExpectedErrorFlags::Contains, 0);
    UCk_ScriptProcessorQuery_Mixin_UE::Require(Query, FCk_ScriptProcessorQuery::StaticStruct());

    TestTrue(TEXT("rejected slot poisons the complete query"), Query._AdmissionFailed);
    TestEqual(TEXT("rejected slot is not appended"), Query._Slots.Num(), 1);

    return true;
}

bool FCkTest_ScriptQuery_SafeThenInvalidValidationIsAtomic::RunTest(const FString&)
{
    auto Query = FCk_ScriptProcessorQuery{};

    auto SafeSlot = FCk_ScriptQuerySlot{};
    SafeSlot._StructType = FCk_FloatRange::StaticStruct();
    SafeSlot._Access = ECk_ScriptQueryAccess::ReadOnly;
    Query._Slots.Add(SafeSlot);

    TestTrue(TEXT("safe query validates before invalid slot is added"),
        ck::dynamic::Validate_ScriptQuerySlots(Query, TEXT("automation-safe")));

    // Models a reflected type invalidating after Configure — descriptor harvest and runtime share this preflight.
    auto InvalidSlot = FCk_ScriptQuerySlot{};
    InvalidSlot._Access = ECk_ScriptQueryAccess::Require;
    Query._Slots.Add(InvalidSlot);

    AddExpectedError(TEXT("contains an invalid fragment type"), EAutomationExpectedErrorFlags::Contains, 0);
    TestFalse(TEXT("safe-then-invalid query is rejected atomically"),
        ck::dynamic::Validate_ScriptQuerySlots(Query, TEXT("automation-invalid")));

    return true;
}

bool FCkTest_ScriptQueryBatch_StaleGenerationNeverResolves::RunTest(const FString&)
{
    auto State = FCk_ScriptQueryBatchState{};
    State._Entities.Add(static_cast<entt::entity>(7));

    const auto FirstGeneration = ck::dynamic::Open_ScriptQueryBatchState(State);
    auto FirstBatch = FCk_ScriptQueryBatch{};
    FirstBatch._State = &State;
    FirstBatch._Generation = FirstGeneration;

    TestEqual(TEXT("live batch resolves entity snapshot"),
        UCk_ScriptQueryBatch_Mixin_UE::Num(FirstBatch), 1);
    TestTrue(TEXT("live generation resolves exact state"),
        ck::dynamic::Resolve_ScriptQueryBatchState(FirstBatch) == &State);

    ck::dynamic::Close_ScriptQueryBatchState(State, FirstGeneration);
    TestEqual(TEXT("closed batch reports empty without dereferencing state"),
        UCk_ScriptQueryBatch_Mixin_UE::Num(FirstBatch), 0);
    TestNull(TEXT("closed generation no longer resolves"),
        ck::dynamic::Resolve_ScriptQueryBatchState(FirstBatch));

    const auto SecondGeneration = ck::dynamic::Open_ScriptQueryBatchState(State);
    auto SecondBatch = FCk_ScriptQueryBatch{};
    SecondBatch._State = &State;
    SecondBatch._Generation = SecondGeneration;

    TestNotEqual(TEXT("reopened state receives a new generation"), FirstGeneration, SecondGeneration);
    TestNull(TEXT("old batch cannot alias reopened state"),
        ck::dynamic::Resolve_ScriptQueryBatchState(FirstBatch));
    TestTrue(TEXT("new batch resolves reopened state"),
        ck::dynamic::Resolve_ScriptQueryBatchState(SecondBatch) == &State);

    ck::dynamic::Close_ScriptQueryBatchState(State, SecondGeneration);

    AddExpectedErrorPlain(
        TEXT("used past its ForEachBatch call (stale generation)"),
        EAutomationExpectedErrorFlags::Contains,
        0);
    const auto StaleHandle = UCk_ScriptQueryBatch_Mixin_UE::GetHandle(FirstBatch, 0);
    TestFalse(TEXT("stale GetHandle fails closed"), ck::IsValid(StaleHandle));

    const auto ThirdGeneration = ck::dynamic::Open_ScriptQueryBatchState(State);
    auto ThirdBatch = FCk_ScriptQueryBatch{};
    ThirdBatch._State = &State;
    ThirdBatch._Generation = ThirdGeneration;

    AddExpectedError(TEXT("closed with stale generation"), EAutomationExpectedErrorFlags::Contains, 0);
    ck::dynamic::Close_ScriptQueryBatchState(State, ThirdGeneration + 1);
    TestNull(TEXT("mismatched close removes the live entry instead of leaving a dangling registration"),
        ck::dynamic::Resolve_ScriptQueryBatchState(ThirdBatch));

    return true;
}

#endif
