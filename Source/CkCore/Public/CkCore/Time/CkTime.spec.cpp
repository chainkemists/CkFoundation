// Coverage for the ck::time FCk_Time factories. The MISUSE surface (a non-positive interval) is rejected at
// compile time by ck::time::detail::Interval_MustBePositive, so no runtime spec can exercise it.

#include "CkCore/Time/CkTime.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Interval VALUES are checked at runtime below because FCk_Time::Get_Seconds is not constexpr.
static constexpr FCk_Time GTime_ConstexprProbe = ck::time::Hz(4);
static_assert(std::is_same_v<std::remove_const_t<decltype(GTime_ConstexprProbe)>, FCk_Time>,
    "ck::time factories produce an FCk_Time usable in a constant expression");

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_Time_Factories,
    "Ck.CkCore.Time.Factories",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_Time_Factories::RunTest(const FString&)
{
    TestTrue(TEXT("Hz(4) == Seconds(0.25)"),              ck::time::Hz(4) == ck::time::Seconds(0.25));
    TestTrue(TEXT("Hz(4) == FCk_Time{0.25}"),             ck::time::Hz(4) == FCk_Time{0.25});
    TestTrue(TEXT("Seconds(0.25) == FCk_Time{0.25}"),     ck::time::Seconds(0.25) == FCk_Time{0.25});
    TestTrue(TEXT("Hz(2) == FCk_Time{0.5}"),              ck::time::Hz(2) == FCk_Time{0.5});
    TestTrue(TEXT("Hz(1) == FCk_Time{1.0}"),              ck::time::Hz(1) == FCk_Time{1.0});
    TestTrue(TEXT("Milliseconds(250) == FCk_Time{0.25}"), ck::time::Milliseconds(250) == FCk_Time{0.25});
    TestTrue(TEXT("Minutes(2) == FCk_Time{120}"),         ck::time::Minutes(2) == FCk_Time{120.0});

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
