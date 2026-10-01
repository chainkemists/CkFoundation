#include "CkPso/Drain/CkPso_DrainTracker.h"

#include "CkCore/Format/CkFormat.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck_test_pso_drain_tracker
{
    constexpr auto Tick = FCk_Time{0.25};
    constexpr auto QuietPeriod = FCk_Time{0.5};
    constexpr auto Disabled = FCk_Time{};

    auto
    TestIsFreshWindow(
        FAutomationTestBase& InTest,
        const FString& InWhen,
        const FCk_Pso_DrainTracker& InTracker)
        -> void
    {
        InTest.TestTrue(*ck::Format_UE(TEXT("{}: draining"), InWhen),
            InTracker.Get_State() == ECk_Pso_DrainState::Draining);
        InTest.TestTrue(*ck::Format_UE(TEXT("{}: no timeout reason"), InWhen),
            InTracker.Get_TimeoutReason() == ECk_Pso_DrainTimeoutReason::None);
        InTest.TestEqual(*ck::Format_UE(TEXT("{}: remaining cleared"), InWhen),
            static_cast<int32>(InTracker.Get_NumRemaining()), 0);
        InTest.TestEqual(*ck::Format_UE(TEXT("{}: peak cleared"), InWhen),
            static_cast<int32>(InTracker.Get_NumPeak()), 0);
        InTest.TestEqual(*ck::Format_UE(TEXT("{}: progress cleared"), InWhen),
            InTracker.Get_ProgressRatio(), 0.0f);
        InTest.TestEqual(*ck::Format_UE(TEXT("{}: elapsed cleared"), InWhen),
            InTracker.Get_Elapsed().Get_Seconds(), 0.0);
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_IdleIgnoresUpdates,
    "Ck.Pso.DrainTracker.IdleIgnoresUpdates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_CompletesAfterQuietPeriod,
    "Ck.Pso.DrainTracker.CompletesAfterQuietPeriod",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_WorkMidSettleRestartsQuietPeriod,
    "Ck.Pso.DrainTracker.WorkMidSettleRestartsQuietPeriod",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_ZeroWorkFastPath,
    "Ck.Pso.DrainTracker.ZeroWorkFastPath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_CompleteReopensWithinWindow,
    "Ck.Pso.DrainTracker.CompleteReopensWithinWindow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_ReopenSharesMaxWaitBudget,
    "Ck.Pso.DrainTracker.ReopenSharesMaxWaitBudget",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_MaxWaitTimesOut,
    "Ck.Pso.DrainTracker.MaxWaitTimesOut",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_StallTimesOut,
    "Ck.Pso.DrainTracker.StallTimesOut",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_TimedOutIsTerminal,
    "Ck.Pso.DrainTracker.TimedOutIsTerminal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_ProgressIsMonotonic,
    "Ck.Pso.DrainTracker.ProgressIsMonotonic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_PeakRaisedDuringReopenKeepsProgressMonotonic,
    "Ck.Pso.DrainTracker.PeakRaisedDuringReopenKeepsProgressMonotonic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_ZeroTimeoutsNeverTimeOut,
    "Ck.Pso.DrainTracker.ZeroTimeoutsNeverTimeOut",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_BeginRestartsWindowFromAnyState,
    "Ck.Pso.DrainTracker.BeginRestartsWindowFromAnyState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Pso_DrainTracker_ResetReturnsToIdle,
    "Ck.Pso.DrainTracker.ResetReturnsToIdle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// --------------------------------------------------------------------------------------------------------------------

bool FCkTest_Pso_DrainTracker_IdleIgnoresUpdates::RunTest(const FString&)
{
    auto Tracker = FCk_Pso_DrainTracker{};

    TestTrue(TEXT("a fresh tracker is idle"), Tracker.Get_State() == ECk_Pso_DrainState::Idle);
    TestFalse(TEXT("idle does not hold"), Tracker.Get_IsHolding());

    Tracker.Update(5, FCk_Time{1.0});

    TestTrue(TEXT("idle stays idle"), Tracker.Get_State() == ECk_Pso_DrainState::Idle);
    TestEqual(TEXT("idle records no count"), static_cast<int32>(Tracker.Get_NumRemaining()), 0);
    TestEqual(TEXT("idle records no peak"), static_cast<int32>(Tracker.Get_NumPeak()), 0);
    TestEqual(TEXT("idle accumulates no time"), Tracker.Get_Elapsed().Get_Seconds(), 0.0);
    TestEqual(TEXT("idle reports no progress"), Tracker.Get_ProgressRatio(), 0.0f);

    return true;
}

bool FCkTest_Pso_DrainTracker_CompletesAfterQuietPeriod::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    auto Tracker = FCk_Pso_DrainTracker{};
    Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{QuietPeriod, Disabled, Disabled});

    TestTrue(TEXT("begin starts draining"), Tracker.Get_State() == ECk_Pso_DrainState::Draining);
    TestTrue(TEXT("draining holds"), Tracker.Get_IsHolding());

    Tracker.Update(10, Tick);
    TestTrue(TEXT("remaining work keeps draining"), Tracker.Get_State() == ECk_Pso_DrainState::Draining);
    TestEqual(TEXT("remaining is the last value fed"), static_cast<int32>(Tracker.Get_NumRemaining()), 10);
    TestEqual(TEXT("peak tracks the high-water mark"), static_cast<int32>(Tracker.Get_NumPeak()), 10);

    Tracker.Update(5, Tick);
    TestEqual(TEXT("progress is 1 - remaining / peak"), Tracker.Get_ProgressRatio(), 0.5f);

    Tracker.Update(0, Tick);
    TestTrue(TEXT("reaching zero settles"), Tracker.Get_State() == ECk_Pso_DrainState::Settling);
    TestTrue(TEXT("settling holds"), Tracker.Get_IsHolding());
    TestEqual(TEXT("zero remaining is full progress"), Tracker.Get_ProgressRatio(), 1.0f);

    Tracker.Update(0, Tick);
    TestTrue(TEXT("half the quiet period is not enough"), Tracker.Get_State() == ECk_Pso_DrainState::Settling);

    Tracker.Update(0, Tick);
    TestTrue(TEXT("a met quiet period completes"), Tracker.Get_State() == ECk_Pso_DrainState::Complete);
    TestFalse(TEXT("complete does not hold"), Tracker.Get_IsHolding());
    TestTrue(TEXT("completion carries no timeout reason"),
        Tracker.Get_TimeoutReason() == ECk_Pso_DrainTimeoutReason::None);
    TestEqual(TEXT("complete reports full progress"), Tracker.Get_ProgressRatio(), 1.0f);
    TestEqual(TEXT("every held tick was charged"), Tracker.Get_Elapsed().Get_Seconds(), 1.25);

    return true;
}

bool FCkTest_Pso_DrainTracker_WorkMidSettleRestartsQuietPeriod::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    auto Tracker = FCk_Pso_DrainTracker{};
    Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{QuietPeriod, Disabled, Disabled});

    Tracker.Update(10, Tick);
    Tracker.Update(0, Tick);
    Tracker.Update(0, Tick);
    TestTrue(TEXT("part-way through the quiet period"), Tracker.Get_State() == ECk_Pso_DrainState::Settling);

    Tracker.Update(3, Tick);
    TestTrue(TEXT("work reappearing mid-settle drains again"), Tracker.Get_State() == ECk_Pso_DrainState::Draining);

    Tracker.Update(0, Tick);
    TestTrue(TEXT("settles again"), Tracker.Get_State() == ECk_Pso_DrainState::Settling);

    Tracker.Update(0, Tick);
    TestTrue(TEXT("the earlier partial quiet time does not carry over"),
        Tracker.Get_State() == ECk_Pso_DrainState::Settling);

    Tracker.Update(0, Tick);
    TestTrue(TEXT("a full uninterrupted quiet period completes"),
        Tracker.Get_State() == ECk_Pso_DrainState::Complete);

    return true;
}

bool FCkTest_Pso_DrainTracker_ZeroWorkFastPath::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    {
        auto Tracker = FCk_Pso_DrainTracker{};
        Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{QuietPeriod, Disabled, Disabled});

        Tracker.Update(0, Tick);
        TestTrue(TEXT("no work settles on the first update"), Tracker.Get_State() == ECk_Pso_DrainState::Settling);
        TestEqual(TEXT("no work is full progress"), Tracker.Get_ProgressRatio(), 1.0f);
        TestEqual(TEXT("no work leaves no peak"), static_cast<int32>(Tracker.Get_NumPeak()), 0);

        Tracker.Update(0, Tick);
        Tracker.Update(0, Tick);
        TestTrue(TEXT("no work completes after the quiet period"),
            Tracker.Get_State() == ECk_Pso_DrainState::Complete);
        TestEqual(TEXT("no work costs one tick plus the quiet period"), Tracker.Get_Elapsed().Get_Seconds(), 0.75);
    }

    {
        auto Tracker = FCk_Pso_DrainTracker{};
        Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{Disabled, Disabled, Disabled});

        Tracker.Update(0, Tick);
        TestTrue(TEXT("a zero quiet period completes on the first update with no work"),
            Tracker.Get_State() == ECk_Pso_DrainState::Complete);
    }

    return true;
}

bool FCkTest_Pso_DrainTracker_CompleteReopensWithinWindow::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    auto Tracker = FCk_Pso_DrainTracker{};
    Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{QuietPeriod, Disabled, Disabled});

    Tracker.Update(8, Tick);
    Tracker.Update(0, Tick);
    Tracker.Update(0, Tick);
    Tracker.Update(0, Tick);
    TestTrue(TEXT("completed"), Tracker.Get_State() == ECk_Pso_DrainState::Complete);
    TestEqual(TEXT("elapsed at completion"), Tracker.Get_Elapsed().Get_Seconds(), 1.0);

    Tracker.Update(0, FCk_Time{10.0});
    TestTrue(TEXT("no work keeps it complete"), Tracker.Get_State() == ECk_Pso_DrainState::Complete);
    TestEqual(TEXT("complete accumulates no time"), Tracker.Get_Elapsed().Get_Seconds(), 1.0);

    Tracker.Update(4, FCk_Time{10.0});
    TestTrue(TEXT("new work re-opens to draining"), Tracker.Get_State() == ECk_Pso_DrainState::Draining);
    TestTrue(TEXT("a re-opened window holds"), Tracker.Get_IsHolding());
    TestEqual(TEXT("the interval spent complete is not charged"), Tracker.Get_Elapsed().Get_Seconds(), 1.0);
    TestEqual(TEXT("the peak survives the re-open"), static_cast<int32>(Tracker.Get_NumPeak()), 8);
    TestEqual(TEXT("remaining is the re-opening count"), static_cast<int32>(Tracker.Get_NumRemaining()), 4);

    Tracker.Update(4, Tick);
    TestEqual(TEXT("elapsed keeps accumulating once held again"), Tracker.Get_Elapsed().Get_Seconds(), 1.25);

    Tracker.Update(0, Tick);
    Tracker.Update(0, Tick);
    Tracker.Update(0, Tick);
    TestTrue(TEXT("a re-opened window completes again"), Tracker.Get_State() == ECk_Pso_DrainState::Complete);

    return true;
}

bool FCkTest_Pso_DrainTracker_ReopenSharesMaxWaitBudget::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    auto Tracker = FCk_Pso_DrainTracker{};
    Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{QuietPeriod, FCk_Time{2.0}, Disabled});

    Tracker.Update(8, Tick);
    Tracker.Update(0, Tick);
    Tracker.Update(0, Tick);
    Tracker.Update(0, Tick);
    TestTrue(TEXT("completed inside the budget"), Tracker.Get_State() == ECk_Pso_DrainState::Complete);

    Tracker.Update(4, FCk_Time{10.0});
    Tracker.Update(3, Tick);
    Tracker.Update(2, Tick);
    Tracker.Update(1, Tick);
    TestTrue(TEXT("still inside the shared budget"), Tracker.Get_State() == ECk_Pso_DrainState::Draining);

    Tracker.Update(1, Tick);
    TestTrue(TEXT("the re-opened window exhausts the budget it shares"),
        Tracker.Get_State() == ECk_Pso_DrainState::TimedOut);
    TestTrue(TEXT("the reason is the max wait"),
        Tracker.Get_TimeoutReason() == ECk_Pso_DrainTimeoutReason::MaxWait);
    TestEqual(TEXT("the budget was spent across both halves of the window"),
        Tracker.Get_Elapsed().Get_Seconds(), 2.0);

    return true;
}

bool FCkTest_Pso_DrainTracker_MaxWaitTimesOut::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    {
        auto Tracker = FCk_Pso_DrainTracker{};
        Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{QuietPeriod, FCk_Time{1.0}, Disabled});

        Tracker.Update(100, Tick);
        Tracker.Update(75, Tick);
        Tracker.Update(50, Tick);
        TestTrue(TEXT("steady progress inside the budget keeps draining"),
            Tracker.Get_State() == ECk_Pso_DrainState::Draining);

        Tracker.Update(25, Tick);
        TestTrue(TEXT("steady progress still times out at the budget"),
            Tracker.Get_State() == ECk_Pso_DrainState::TimedOut);
        TestTrue(TEXT("the reason is the max wait"),
            Tracker.Get_TimeoutReason() == ECk_Pso_DrainTimeoutReason::MaxWait);
        TestFalse(TEXT("timed out does not hold"), Tracker.Get_IsHolding());
    }

    {
        auto Tracker = FCk_Pso_DrainTracker{};
        Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{FCk_Time{10.0}, QuietPeriod, Disabled});

        Tracker.Update(0, Tick);
        TestTrue(TEXT("settling inside the budget"), Tracker.Get_State() == ECk_Pso_DrainState::Settling);

        Tracker.Update(0, Tick);
        TestTrue(TEXT("the budget also bounds a settle that has not met its quiet period"),
            Tracker.Get_State() == ECk_Pso_DrainState::TimedOut);
        TestTrue(TEXT("the reason is the max wait"),
            Tracker.Get_TimeoutReason() == ECk_Pso_DrainTimeoutReason::MaxWait);
    }

    return true;
}

bool FCkTest_Pso_DrainTracker_StallTimesOut::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    constexpr auto StallTimeout = FCk_Time{1.0};

    {
        auto Tracker = FCk_Pso_DrainTracker{};
        Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{QuietPeriod, Disabled, StallTimeout});

        Tracker.Update(10, FCk_Time{0.5});
        Tracker.Update(20, StallTimeout);
        Tracker.Update(30, StallTimeout);
        TestTrue(TEXT("a rising count is work still arriving, not a stall"),
            Tracker.Get_State() == ECk_Pso_DrainState::Draining);

        Tracker.Update(30, FCk_Time{0.5});
        Tracker.Update(30, Tick);
        TestTrue(TEXT("an unchanged count short of the stall timeout keeps draining"),
            Tracker.Get_State() == ECk_Pso_DrainState::Draining);

        Tracker.Update(30, Tick);
        TestTrue(TEXT("an unchanged count for the stall timeout times out"),
            Tracker.Get_State() == ECk_Pso_DrainState::TimedOut);
        TestTrue(TEXT("the reason is a stall"),
            Tracker.Get_TimeoutReason() == ECk_Pso_DrainTimeoutReason::Stalled);
    }

    {
        auto Tracker = FCk_Pso_DrainTracker{};
        Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{QuietPeriod, Disabled, StallTimeout});

        Tracker.Update(10, FCk_Time{0.5});
        Tracker.Update(9, FCk_Time{0.5});
        Tracker.Update(9, FCk_Time{0.5});
        Tracker.Update(9, Tick);
        TestTrue(TEXT("a decrease resets the stall timer"), Tracker.Get_State() == ECk_Pso_DrainState::Draining);

        Tracker.Update(9, Tick);
        TestTrue(TEXT("a full stall timeout after the last decrease times out"),
            Tracker.Get_State() == ECk_Pso_DrainState::TimedOut);
        TestTrue(TEXT("the reason is a stall"),
            Tracker.Get_TimeoutReason() == ECk_Pso_DrainTimeoutReason::Stalled);
    }

    {
        auto Tracker = FCk_Pso_DrainTracker{};
        Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{FCk_Time{100.0}, Disabled, QuietPeriod});

        Tracker.Update(0, Tick);
        Tracker.Update(0, FCk_Time{5.0});
        TestTrue(TEXT("settling never stalls"), Tracker.Get_State() == ECk_Pso_DrainState::Settling);
        TestTrue(TEXT("settling carries no timeout reason"),
            Tracker.Get_TimeoutReason() == ECk_Pso_DrainTimeoutReason::None);
    }

    return true;
}

bool FCkTest_Pso_DrainTracker_TimedOutIsTerminal::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    auto Tracker = FCk_Pso_DrainTracker{};
    Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{QuietPeriod, QuietPeriod, Disabled});

    Tracker.Update(10, Tick);
    Tracker.Update(5, Tick);
    TestTrue(TEXT("timed out"), Tracker.Get_State() == ECk_Pso_DrainState::TimedOut);
    TestEqual(TEXT("timing out leaves progress where it was"), Tracker.Get_ProgressRatio(), 0.5f);

    Tracker.Update(0, Tick);
    TestTrue(TEXT("no work does not leave timed out"), Tracker.Get_State() == ECk_Pso_DrainState::TimedOut);

    Tracker.Update(20, Tick);
    TestTrue(TEXT("new work does not re-open a timed-out window"),
        Tracker.Get_State() == ECk_Pso_DrainState::TimedOut);
    TestFalse(TEXT("a timed-out window never holds again"), Tracker.Get_IsHolding());
    TestTrue(TEXT("the reason survives later updates"),
        Tracker.Get_TimeoutReason() == ECk_Pso_DrainTimeoutReason::MaxWait);
    TestEqual(TEXT("updates after timing out are not recorded"),
        static_cast<int32>(Tracker.Get_NumRemaining()), 5);
    TestEqual(TEXT("the peak is not raised after timing out"), static_cast<int32>(Tracker.Get_NumPeak()), 10);
    TestEqual(TEXT("timed out accumulates no time"), Tracker.Get_Elapsed().Get_Seconds(), 0.5);
    TestEqual(TEXT("progress does not move after timing out"), Tracker.Get_ProgressRatio(), 0.5f);

    return true;
}

bool FCkTest_Pso_DrainTracker_ProgressIsMonotonic::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    auto Tracker = FCk_Pso_DrainTracker{};
    Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{QuietPeriod, Disabled, Disabled});

    TestEqual(TEXT("a new window starts at zero progress"), Tracker.Get_ProgressRatio(), 0.0f);

    Tracker.Update(100, Tick);
    TestEqual(TEXT("the first count is the peak - zero progress"), Tracker.Get_ProgressRatio(), 0.0f);

    Tracker.Update(50, Tick);
    TestEqual(TEXT("half drained"), Tracker.Get_ProgressRatio(), 0.5f);

    Tracker.Update(400, Tick);
    TestEqual(TEXT("a raised peak lifts the high-water mark"), static_cast<int32>(Tracker.Get_NumPeak()), 400);
    TestEqual(TEXT("a raised peak does not move progress backwards"), Tracker.Get_ProgressRatio(), 0.5f);

    Tracker.Update(200, Tick);
    TestEqual(TEXT("progress holds until the raw ratio passes it"), Tracker.Get_ProgressRatio(), 0.5f);

    Tracker.Update(100, Tick);
    TestEqual(TEXT("progress resumes against the raised peak"), Tracker.Get_ProgressRatio(), 0.75f);

    Tracker.Update(0, Tick);
    TestEqual(TEXT("fully drained"), Tracker.Get_ProgressRatio(), 1.0f);

    return true;
}

bool FCkTest_Pso_DrainTracker_PeakRaisedDuringReopenKeepsProgressMonotonic::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    auto Tracker = FCk_Pso_DrainTracker{};
    Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{Disabled, Disabled, Disabled});

    Tracker.Update(100, Tick);
    Tracker.Update(0, Tick);
    TestTrue(TEXT("completed"), Tracker.Get_State() == ECk_Pso_DrainState::Complete);
    TestEqual(TEXT("complete forces full progress"), Tracker.Get_ProgressRatio(), 1.0f);

    Tracker.Update(300, Tick);
    TestTrue(TEXT("re-opened"), Tracker.Get_State() == ECk_Pso_DrainState::Draining);
    TestEqual(TEXT("the re-opening count raises the peak"), static_cast<int32>(Tracker.Get_NumPeak()), 300);
    TestEqual(TEXT("a re-open does not move progress backwards"), Tracker.Get_ProgressRatio(), 1.0f);

    Tracker.Update(150, Tick);
    TestEqual(TEXT("progress stays at its high-water mark while re-opened"), Tracker.Get_ProgressRatio(), 1.0f);

    Tracker.Update(0, Tick);
    TestTrue(TEXT("completed again"), Tracker.Get_State() == ECk_Pso_DrainState::Complete);
    TestEqual(TEXT("still full progress"), Tracker.Get_ProgressRatio(), 1.0f);

    return true;
}

bool FCkTest_Pso_DrainTracker_ZeroTimeoutsNeverTimeOut::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    constexpr auto LongTick = FCk_Time{1000.0};

    auto Tracker = FCk_Pso_DrainTracker{};
    Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{QuietPeriod, Disabled, Disabled});

    Tracker.Update(10, LongTick);
    Tracker.Update(10, LongTick);
    Tracker.Update(20, LongTick);
    Tracker.Update(20, LongTick);

    TestTrue(TEXT("disabled timeouts keep a stuck count draining"),
        Tracker.Get_State() == ECk_Pso_DrainState::Draining);
    TestTrue(TEXT("disabled timeouts report no reason"),
        Tracker.Get_TimeoutReason() == ECk_Pso_DrainTimeoutReason::None);
    TestEqual(TEXT("elapsed still accumulates"), Tracker.Get_Elapsed().Get_Seconds(), 4000.0);

    return true;
}

bool FCkTest_Pso_DrainTracker_BeginRestartsWindowFromAnyState::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    const auto Params = FCk_Pso_DrainTracker_Params{Disabled, QuietPeriod, Disabled};

    {
        auto Tracker = FCk_Pso_DrainTracker{};
        Tracker.Request_Begin(Params);
        ck_test_pso_drain_tracker::TestIsFreshWindow(*this, TEXT("from idle"), Tracker);
    }

    {
        auto Tracker = FCk_Pso_DrainTracker{};
        Tracker.Request_Begin(Params);
        Tracker.Update(10, Tick);
        Tracker.Request_Begin(Params);
        ck_test_pso_drain_tracker::TestIsFreshWindow(*this, TEXT("from draining"), Tracker);
    }

    {
        auto Tracker = FCk_Pso_DrainTracker{};
        Tracker.Request_Begin(Params);
        Tracker.Update(0, Tick);
        TestTrue(TEXT("completed before restarting"), Tracker.Get_State() == ECk_Pso_DrainState::Complete);
        Tracker.Request_Begin(Params);
        ck_test_pso_drain_tracker::TestIsFreshWindow(*this, TEXT("from complete"), Tracker);
    }

    {
        auto Tracker = FCk_Pso_DrainTracker{};
        Tracker.Request_Begin(Params);
        Tracker.Update(10, Tick);
        Tracker.Update(5, Tick);
        TestTrue(TEXT("timed out before restarting"), Tracker.Get_State() == ECk_Pso_DrainState::TimedOut);
        Tracker.Request_Begin(Params);
        ck_test_pso_drain_tracker::TestIsFreshWindow(*this, TEXT("from timed out"), Tracker);

        Tracker.Update(10, Tick);
        TestTrue(TEXT("a restarted window gets a fresh budget"), Tracker.Get_State() == ECk_Pso_DrainState::Draining);
    }

    return true;
}

bool FCkTest_Pso_DrainTracker_ResetReturnsToIdle::RunTest(const FString&)
{
    using namespace ck_test_pso_drain_tracker;

    auto Tracker = FCk_Pso_DrainTracker{};
    Tracker.Request_Begin(FCk_Pso_DrainTracker_Params{QuietPeriod, Disabled, Disabled});
    Tracker.Update(10, Tick);
    Tracker.Update(5, Tick);

    Tracker.Request_Reset();

    TestTrue(TEXT("reset is idle"), Tracker.Get_State() == ECk_Pso_DrainState::Idle);
    TestFalse(TEXT("reset does not hold"), Tracker.Get_IsHolding());
    TestTrue(TEXT("reset clears the timeout reason"),
        Tracker.Get_TimeoutReason() == ECk_Pso_DrainTimeoutReason::None);
    TestEqual(TEXT("reset clears remaining"), static_cast<int32>(Tracker.Get_NumRemaining()), 0);
    TestEqual(TEXT("reset clears the peak"), static_cast<int32>(Tracker.Get_NumPeak()), 0);
    TestEqual(TEXT("reset clears progress"), Tracker.Get_ProgressRatio(), 0.0f);
    TestEqual(TEXT("reset clears elapsed"), Tracker.Get_Elapsed().Get_Seconds(), 0.0);

    Tracker.Update(5, Tick);
    TestTrue(TEXT("updates after a reset are ignored"), Tracker.Get_State() == ECk_Pso_DrainState::Idle);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif
