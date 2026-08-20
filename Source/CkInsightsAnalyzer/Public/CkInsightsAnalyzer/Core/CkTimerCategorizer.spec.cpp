// Categories are matched in registration order with first-keyword-hit-wins, so a keyword added to
// a high-priority category silently re-homes rows that already had one. Both directions are pinned:
// the scope names that must leave "Other", and the near-miss attributions that must not move.
//
// Every name below is verbatim from a packaged capture (20260818_102122_331DF0 and
// 20260814_131755_222E00) — none are invented.

#include "CkInsightsAnalyzer/Core/CkTimerCategorizer.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ck_timer_categorizer_tests
{
    struct FExpectation
    {
        const TCHAR* TimerName;
        const TCHAR* ExpectedCategory;
    };

    // Landed in "Other" before the keyword coverage was widened — together ~3 ms of a 21.8 ms frame
    // reported as unattributed engine work.
    const TArray<FExpectation> MustLeaveOther
    {
        // Slate scopes whose names carry no other Slate token.
        {TEXT("SConstraintCanvas"),                TEXT("Slate/UI")},
        {TEXT("UpdateExceptVisibilityAttributes"), TEXT("Slate/UI")},
        {TEXT("UpdateVisibilityAttributes"),       TEXT("Slate/UI")},
        {TEXT("Text Layout"),                      TEXT("Slate/UI")},
        {TEXT("Paint: Game UI"),                   TEXT("Slate/UI")},
        {TEXT("GatherWindowElements"),             TEXT("Slate/UI")},
        {TEXT("Update Tooltip Time"),              TEXT("Slate/UI")},
        {TEXT("DrawStringInternal_RuntimeCache"),  TEXT("Slate/UI")},
        {TEXT("QueryCursor"),                      TEXT("Slate/UI")},
        {TEXT("ProcessMouseButtonUp"),             TEXT("Slate/UI")},

        // Ck scopes declared with a bare subsystem prefix instead of a ck:: / Ck_ token.
        {TEXT("Scheduler::Dispatch"),              TEXT("ECS (CK)")},
        {TEXT("Scheduler::EmptyViewCheck"),        TEXT("ECS (CK)")},
        {TEXT("SmTask::Tick (proc)"),              TEXT("ECS (CK)")},
        {TEXT("Sm::ComputeNetContext"),            TEXT("ECS (CK)")},
        {TEXT("EntityTag::ForEach_Entity"),        TEXT("ECS (CK)")},
        {TEXT("Record::ForEach_Entry"),            TEXT("ECS (CK)")},
        {TEXT("Ism::UpdateInstanceTransform"),     TEXT("ECS (CK)")},
        {TEXT("DestroyEntities"),                  TEXT("ECS (CK)")},

        // Game scopes.
        {TEXT("Get_BehaviorLeafClass"),            TEXT("BusterBlock Game")},
        {TEXT("Is_LocomotionLeaf"),                TEXT("BusterBlock Game")},
        {TEXT("Minimap::DiffSignals"),             TEXT("BusterBlock Game")},
        {TEXT("Write_HeadCutaway"),                TEXT("BusterBlock Game")},

        // Engine scopes with an obvious home.
        {TEXT("JoltWorld_Step"),                   TEXT("Physics (Jolt)")},
        {TEXT("JoltContacts_DrainQueue"),          TEXT("Physics (Jolt)")},
        {TEXT("JoltContacts_Persisted"),           TEXT("Physics (Jolt)")},
        {TEXT("JoltContacts_Added"),               TEXT("Physics (Jolt)")},
        {TEXT("UnknownSceneQuery"),                TEXT("Scene Queries")},
        {TEXT("SceneQueryTotal"),                  TEXT("Scene Queries")},
        {TEXT("AddPrimitive (GT)"),                TEXT("Rendering")},
        {TEXT("UPrimitiveComponent::GetStreamingRenderAssetInfoWithNULLRemoval"), TEXT("Rendering")},
    };

    // The near-misses of the keywords above. Re-homing a row that already had a category breaks
    // A/B continuity against previously captured reports, which is worse than leaving it in Other.
    const TArray<FExpectation> MustNotMove
    {
        // "JoltWorld_Step" stays exact: a broader "JoltWorld" outranks ECS and would claim this.
        {TEXT("ck::FProcessor_JoltWorld_WaitForAsync"),   TEXT("ECS (CK)")},

        // "Locomotion" / "BehaviorLeaf" sit in the last category and must not shadow the AI and
        // animation scopes that own those words.
        {TEXT("BehaviorTreeComponent Tick"),             TEXT("Tick Overhead")},
        {TEXT("RefreshBoneTransforms"),                  TEXT("Animation")},

        // Ck processors keep their own category rather than being claimed by domain keywords.
        {TEXT("ck::FProcessor_Transform_HandleRequests"), TEXT("ECS (CK)")},
        {TEXT("ck::FProcessor_Timer_Update"),             TEXT("ECS (CK)")},
        {TEXT("ck::FProcessor_IskmCrowd_Advance"),        TEXT("ECS (CK)")},

        // Unclaimed engine tail stays unclaimed — an over-broad keyword surfacing here would mean
        // "Other" had been made to look smaller than it is.
        {TEXT("FViewport_Draw"),                          TEXT("Other")},
        {TEXT("WinPumpMessages"),                         TEXT("Other")},
        {TEXT("STAT_FTicker_Tick"),                       TEXT("Other")},
    };
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_TimerCategorizer_RealScopeNamesLeaveOther,
    "Ck.CkInsightsAnalyzer.TimerCategorizer.RealScopeNamesLeaveOther",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_TimerCategorizer_RealScopeNamesLeaveOther::RunTest(const FString&)
{
    const auto Categorizer = FCk_TimerCategorizer{};

    ck::algo::ForEach(ck_timer_categorizer_tests::MustLeaveOther,
        [&](const ck_timer_categorizer_tests::FExpectation& InExpectation)
        {
            TestEqual(
                *FString::Printf(TEXT("'%s' categorises as '%s'"),
                    InExpectation.TimerName, InExpectation.ExpectedCategory),
                Categorizer.Categorize(InExpectation.TimerName),
                FString{InExpectation.ExpectedCategory});
        });

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_TimerCategorizer_KeywordsDoNotStealExistingRows,
    "Ck.CkInsightsAnalyzer.TimerCategorizer.KeywordsDoNotStealExistingRows",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_TimerCategorizer_KeywordsDoNotStealExistingRows::RunTest(const FString&)
{
    const auto Categorizer = FCk_TimerCategorizer{};

    ck::algo::ForEach(ck_timer_categorizer_tests::MustNotMove,
        [&](const ck_timer_categorizer_tests::FExpectation& InExpectation)
        {
            TestEqual(
                *FString::Printf(TEXT("'%s' stays '%s'"),
                    InExpectation.TimerName, InExpectation.ExpectedCategory),
                Categorizer.Categorize(InExpectation.TimerName),
                FString{InExpectation.ExpectedCategory});
        });

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
