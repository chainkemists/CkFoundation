// Pure coverage for the cadence-bucket quantization and the compile-time tick-rate literals — no world,
// no entities, no scheduler. The RUNTIME semantics (a rated processor firing at its declared rate, the
// immediate first eval, the empty-view accumulator freeze) are pinned hermetically in CkTests
// (Test_Processor_TickRateTrait.cpp) and at the PIE level by Ck_AutoTest_VisibleRange_CadenceGatesUpdates.
//
// The MISUSE surface is compile-time by design and therefore self-testing — each of these fails the BUILD,
// so no runtime spec can (or needs to) exercise them:
//   - ck::Seconds{0} / ck::Seconds{-1} / ck::Hz{0} / ck::Hz{-4}  -> consteval ctor poisons constant
//     evaluation via ck::detail::TickRate_MustBePositive (declared, never defined, not constexpr).
//   - TickRate as a raw double / FCk_Time / any non-FTickRate type -> static_assert in
//     TProcessorBase::Get_TickRate ("must be a ck tick-rate literal").
//   - TickRate as a non-static (instance) member                  -> static_assert ("instance member").
//   - TickRate as a type alias                                    -> static_assert ("declared as a TYPE").
//   - TickRate as a non-constexpr static                          -> the positivity static_assert cannot
//     constant-evaluate the read.
//   - Set_TickRate called on a TickRate-trait-declaring processor -> static_assert in Set_TickRate's body
//     (instantiated only when called).
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
// Compile-time facts about the literal types — a broken one fails the build of this TU.

static_assert(ck::Hz{4}.Get_IntervalSeconds() == 0.25,
    "Hz is cycles-per-second: Hz{4} is a 0.25s interval");
static_assert(ck::Seconds{0.25}.Get_IntervalSeconds() == 0.25,
    "Seconds carries the interval verbatim");
static_assert(ck::Hz{4}.Get_IntervalSeconds() == ck::Seconds{0.25}.Get_IntervalSeconds(),
    "Hz{4} and Seconds{0.25} are the same rate");
static_assert(ck::Hz{10}.Get_IntervalSeconds() == 0.1,
    "Hz{10} is a 0.1s interval");
static_assert(std::is_base_of_v<ck::FTickRate, ck::Hz> && std::is_base_of_v<ck::FTickRate, ck::Seconds>,
    "both literal spellings share the FTickRate carrier TProcessorBase::Get_TickRate detects");

// The bucket rate-trait mixin mirrors the interval table exactly; bucket 0 declares NO trait at all,
// so bucket-0 processors hit the same ZeroSecond every-tick fast path as any unrated processor.
static_assert(ck::detail::TCadenceBucketRateTraits<1>::TickRate.Get_IntervalSeconds() == ck::cadence::BucketIntervalsSeconds[1],
    "bucket 1 trait mirrors the interval table");
static_assert(ck::detail::TCadenceBucketRateTraits<6>::TickRate.Get_IntervalSeconds() == ck::cadence::BucketIntervalsSeconds[6],
    "bucket 6 trait mirrors the interval table");
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
    // The FCk_Time a rated processor's Tick materializes from the literal (Get_TickRate does
    // FCk_Time{Literal.Get_IntervalSeconds()}) equals the directly-constructed FCk_Time.
    TestTrue(TEXT("Hz{4} materializes as FCk_Time{0.25}"),
        FCk_Time{ck::Hz{4}.Get_IntervalSeconds()} == FCk_Time{0.25});
    TestTrue(TEXT("Seconds{0.25} materializes as FCk_Time{0.25}"),
        FCk_Time{ck::Seconds{0.25}.Get_IntervalSeconds()} == FCk_Time{0.25});
    TestTrue(TEXT("Hz{2} materializes as FCk_Time{0.5}"),
        FCk_Time{ck::Hz{2}.Get_IntervalSeconds()} == FCk_Time{0.5});
    TestTrue(TEXT("Hz{1} materializes as FCk_Time{1.0}"),
        FCk_Time{ck::Hz{1}.Get_IntervalSeconds()} == FCk_Time{1.0});

    // Every nonzero bucket's trait materializes as exactly its interval-table FCk_Time.
    TestTrue(TEXT("bucket 3 trait == FCk_Time{0.5}"),
        FCk_Time{ck::detail::TCadenceBucketRateTraits<3>::TickRate.Get_IntervalSeconds()} == FCk_Time{ck::cadence::BucketIntervalsSeconds[3]});

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
