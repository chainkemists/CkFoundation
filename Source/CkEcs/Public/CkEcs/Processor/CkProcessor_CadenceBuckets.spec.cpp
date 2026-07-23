// Pure coverage for the cadence-bucket quantization and the compile-time tick-rate literals — no world,
// no entities, no scheduler. The RUNTIME semantics (a rated processor firing at its declared rate, the
// immediate first eval, the empty-view accumulator freeze) are pinned hermetically in CkTests
// (Test_Processor_TickRateTrait.cpp) and at the PIE level by Ck_AutoTest_VisibleRange_CadenceGatesUpdates.
//
// The MISUSE surface is compile-time by design and therefore self-testing — each of these fails the BUILD,
// so no runtime spec can (or needs to) exercise them:
//   - ck::Seconds(0) / ck::Seconds(-1) / ck::Hz(0) / ck::Hz(-4)  -> consteval factory poisons constant
//     evaluation via ck::detail::TickRate_MustBePositive (declared, never defined, not constexpr).
//   - TickRate as a raw double or any non-FCk_Time type          -> static_assert in
//     TProcessorBase::Get_TickRate ("must be an FCk_Time").
//   - TickRate as a non-static (instance) member                 -> static_assert ("instance member").
//   - TickRate as a type alias                                   -> static_assert ("declared as a TYPE").
//   - TickRate as a non-constexpr static                         -> fails to initialize the constexpr
//     TraitValue read in Get_TickRate.
//   - TickCatchUpPolicy as an instance member / wrong type        -> static_asserts in Get_TickCatchUpPolicy.

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
// Compile-time facts — a broken one fails the build of this TU. (Interval VALUES are checked at runtime below:
// FCk_Time::Get_Seconds is not constexpr, so a literal's value can't be static_assert'd.)

static_assert(std::is_same_v<std::remove_const_t<decltype(ck::Hz(4))>, FCk_Time>,
    "ck::Hz / ck::Seconds produce an FCk_Time — the type a processor's TickRate trait carries");
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

    // Exact bucket matches land on their own bucket.
    TestEqual(TEXT("0 -> bucket 0 (every tick)"), Get_QuantizedBucketIndex(FCk_Time{0.0}), 0);
    TestEqual(TEXT("0.1 exact -> bucket 1"), Get_QuantizedBucketIndex(FCk_Time{0.1}), 1);
    TestEqual(TEXT("0.25 exact -> bucket 2"), Get_QuantizedBucketIndex(FCk_Time{0.25}), 2);
    TestEqual(TEXT("0.5 exact -> bucket 3"), Get_QuantizedBucketIndex(FCk_Time{0.5}), 3);
    TestEqual(TEXT("1.0 exact -> bucket 4"), Get_QuantizedBucketIndex(FCk_Time{1.0}), 4);
    TestEqual(TEXT("2.0 exact -> bucket 5"), Get_QuantizedBucketIndex(FCk_Time{2.0}), 5);
    TestEqual(TEXT("4.0 exact -> bucket 6"), Get_QuantizedBucketIndex(FCk_Time{4.0}), 6);

    // Between two buckets rounds toward FASTER (the smaller interval) — an entity never updates
    // slower than it requested.
    TestEqual(TEXT("0.3 -> bucket 2 (0.25s), not bucket 3 (0.5s)"), Get_QuantizedBucketIndex(FCk_Time{0.3}), 2);
    TestEqual(TEXT("0.7 -> bucket 3 (0.5s)"), Get_QuantizedBucketIndex(FCk_Time{0.7}), 3);
    TestEqual(TEXT("1.5 -> bucket 4 (1s)"), Get_QuantizedBucketIndex(FCk_Time{1.5}), 4);
    TestEqual(TEXT("3.999 -> bucket 5 (2s)"), Get_QuantizedBucketIndex(FCk_Time{3.999}), 5);

    // Below the smallest nonzero bucket (and any non-positive request) -> bucket 0, every tick.
    TestEqual(TEXT("0.05 (below smallest nonzero) -> bucket 0"), Get_QuantizedBucketIndex(FCk_Time{0.05}), 0);
    TestEqual(TEXT("negative -> bucket 0"), Get_QuantizedBucketIndex(FCk_Time{-1.0}), 0);

    // Above the largest bucket clamps to the largest (never slower than the set offers, but the
    // set's slowest is the floor).
    TestEqual(TEXT("100 -> bucket 6 (the slowest bucket)"), Get_QuantizedBucketIndex(FCk_Time{100.0}), 6);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_CadenceBuckets_TickRateLiterals,
    "Ck.CkEcs.CadenceBuckets.TickRateLiterals",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_CadenceBuckets_TickRateLiterals::RunTest(const FString&)
{
    // ck::Hz / ck::Seconds produce the FCk_Time interval a rated processor's Get_TickRate returns directly.
    TestTrue(TEXT("Hz(4) == FCk_Time{0.25}"),         ck::Hz(4) == FCk_Time{0.25});
    TestTrue(TEXT("Seconds(0.25) == FCk_Time{0.25}"), ck::Seconds(0.25) == FCk_Time{0.25});
    TestTrue(TEXT("Hz(2) == FCk_Time{0.5}"),          ck::Hz(2) == FCk_Time{0.5});
    TestTrue(TEXT("Hz(1) == FCk_Time{1.0}"),          ck::Hz(1) == FCk_Time{1.0});
    TestTrue(TEXT("Hz(4) == Seconds(0.25)"),          ck::Hz(4) == ck::Seconds(0.25));

    // Every nonzero bucket's trait is exactly its interval-table FCk_Time.
    TestTrue(TEXT("bucket 3 trait == interval table"),
        ck::detail::TCadenceBucketRateTraits<3>::TickRate == FCk_Time{ck::cadence::BucketIntervalsSeconds[3]});

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
