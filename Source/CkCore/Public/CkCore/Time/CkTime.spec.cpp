// Pure coverage for the ck::time FCk_Time factories (Seconds / Milliseconds / Minutes / Hz).
//
// The MISUSE surface is compile-time by design (consteval factories) and therefore self-testing — each fails
// the BUILD, so no runtime spec can (or needs to) exercise it: ck::time::Seconds(0) / Seconds(-1) / Hz(0) /
// Hz(-4) reject a non-positive interval via ck::time::detail::Interval_MustBePositive.

#include "CkCore/Time/CkTime.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// Compile-time facts — a broken one fails the build of this TU. Each factory is consteval and yields an
// FCk_Time usable in a constant expression. (Interval VALUES are checked at runtime below: FCk_Time::Get_Seconds
// is not constexpr, so a literal's value can't be static_assert'd.)
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
