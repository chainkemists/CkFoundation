// Tests for the script-processor query mixin (UCk_ScriptProcessorQuery_Mixin_UE over FCk_ScriptProcessorQuery).
//
// Coverage: the four slot mutators accumulate in call order with the correct access, and NoEntities sets the
// escape flag without adding slots.
//
// NOTE: duplicate-fragment rejection is guarded by CK_ENSURE in DoAddSlot and is deliberately NOT asserted here.
// This codebase's C++ automation tests avoid triggering CK_ENSURE (it routes through ck::ensure::Do_HandleFail,
// whose capture-as-test-error behavior is context-dependent). The reject path is verified by inspection.

#include "CkDynamic/CkDynamic_ScriptQuery.h"
#include "CkDynamic/CkDynamic_Fragment_Data.h"

#include "CkEcs/Processor/CkProcessor_ScriptQuery_Data.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ScriptQuery_SlotAccumulationAndAccess,
    "Ck.CkDynamic.ScriptQuery.SlotAccumulationAndAccess",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ScriptQuery_SlotAccumulationAndAccess::RunTest(const FString&)
{
    // Four distinct reflected structs to fill four slots (identity/order is what matters, not the struct roles).
    const auto* StructA = FCk_Fragment_DynamicFragment_Data::StaticStruct();
    const auto* StructB = FCk_DynamicFragment_RepNotifyInfo::StaticStruct();
    const auto* StructC = FCk_ScriptQuerySlot::StaticStruct();
    const auto* StructD = FCk_ScriptProcessorQuery::StaticStruct();

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

    auto NoEntQuery = FCk_ScriptProcessorQuery{};
    UCk_ScriptProcessorQuery_Mixin_UE::NoEntities(NoEntQuery);
    TestTrue (TEXT("NoEntities sets the escape flag"), NoEntQuery._NoEntities);
    TestEqual(TEXT("NoEntities adds no slots"), NoEntQuery._Slots.Num(), 0);

    return true;
}

#endif
