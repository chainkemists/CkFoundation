// Pure coverage for the cadence-bucket quantization and the per-bucket TickRate traits — no world, no
// entities, no scheduler. RUNTIME semantics are pinned in CkTests (Test_Processor_TickRateTrait.cpp) and by
// Ck_AutoTest_VisibleRange_CadenceGatesUpdates; the ck::time factories in CkCore (CkTime.spec.cpp). The
// MISUSE surface is compile-time by design — every misspelling of TickRate fails the BUILD, so no runtime
// spec can exercise it (consteval factory + the static_asserts in TProcessorBase::Get_TickRate).

#include "CkEcs/Processor/CkProcessor_CadenceBuckets.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ck_cadence_buckets_spec
{
    // Naming a member of a CONCRETE type inside a requires-expression hard-errors on MSVC (C2039)
    // instead of yielding false; a dependent template restores the SFINAE-friendly detection.
    template <typename T>
    inline constexpr bool HasTickRateMember = requires { T::TickRate; };
}

// --------------------------------------------------------------------------------------------------------------------
// Interval VALUES are checked at runtime below — FCk_Time::Get_Seconds is not constexpr.

static_assert(std::is_same_v<std::remove_const_t<decltype(ck::detail::TCadenceBucketRateTraits<1>::TickRate)>, FCk_Time>,
    "a nonzero bucket's TickRate trait is an FCk_Time");
static_assert(NOT ck_cadence_buckets_spec::HasTickRateMember<ck::detail::TCadenceBucketRateTraits<0>>,
    "bucket 0 must declare no TickRate trait (every-tick fast path)");

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_CadenceBuckets_QuantizeTowardFaster,
    "Ck.CkEcs.CadenceBuckets.QuantizeTowardFaster",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_CadenceBuckets_QuantizeTowardFaster::RunTest(const FString&)
{
    using namespace ck::cadence;

    TestEqual(TEXT("0 -> bucket 0 (every tick)"), Get_QuantizedBucketIndex(FCk_Time{0.0}), 0);
    TestEqual(TEXT("0.1 exact -> bucket 1"), Get_QuantizedBucketIndex(FCk_Time{0.1}), 1);
    TestEqual(TEXT("0.25 exact -> bucket 2"), Get_QuantizedBucketIndex(FCk_Time{0.25}), 2);
    TestEqual(TEXT("0.5 exact -> bucket 3"), Get_QuantizedBucketIndex(FCk_Time{0.5}), 3);
    TestEqual(TEXT("1.0 exact -> bucket 4"), Get_QuantizedBucketIndex(FCk_Time{1.0}), 4);
    TestEqual(TEXT("2.0 exact -> bucket 5"), Get_QuantizedBucketIndex(FCk_Time{2.0}), 5);
    TestEqual(TEXT("4.0 exact -> bucket 6"), Get_QuantizedBucketIndex(FCk_Time{4.0}), 6);

    TestEqual(TEXT("0.3 -> bucket 2 (0.25s), not bucket 3 (0.5s)"), Get_QuantizedBucketIndex(FCk_Time{0.3}), 2);
    TestEqual(TEXT("0.7 -> bucket 3 (0.5s)"), Get_QuantizedBucketIndex(FCk_Time{0.7}), 3);
    TestEqual(TEXT("1.5 -> bucket 4 (1s)"), Get_QuantizedBucketIndex(FCk_Time{1.5}), 4);
    TestEqual(TEXT("3.999 -> bucket 5 (2s)"), Get_QuantizedBucketIndex(FCk_Time{3.999}), 5);

    TestEqual(TEXT("0.05 (below smallest nonzero) -> bucket 0"), Get_QuantizedBucketIndex(FCk_Time{0.05}), 0);
    TestEqual(TEXT("negative -> bucket 0"), Get_QuantizedBucketIndex(FCk_Time{-1.0}), 0);

    TestEqual(TEXT("100 -> bucket 6 (the slowest bucket)"), Get_QuantizedBucketIndex(FCk_Time{100.0}), 6);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_CadenceBuckets_BucketTraitMatchesIntervalTable,
    "Ck.CkEcs.CadenceBuckets.BucketTraitMatchesIntervalTable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_CadenceBuckets_BucketTraitMatchesIntervalTable::RunTest(const FString&)
{
    // Value check only — the trait's TYPE is pinned by the static_assert above.
    TestTrue(TEXT("bucket 3 trait == interval table"),
        ck::detail::TCadenceBucketRateTraits<3>::TickRate == FCk_Time{ck::cadence::BucketIntervalsSeconds[3]});

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
