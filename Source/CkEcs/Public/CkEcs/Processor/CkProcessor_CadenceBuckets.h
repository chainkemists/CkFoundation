#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcs/Tag/CkTag.h"

#include <utility>

// --------------------------------------------------------------------------------------------------------------------
// Sub-instanced bucketed-cadence processors. A per-entity re-evaluation interval is quantized at Add into a
// fixed bucket set and tagged onto the entity; ONE sub-processor per bucket carries that bucket's interval as
// its compile-time TickRate. "Due-ness" is view membership — no per-entity chrono poll. Rationale, authoring
// recipe and wake-alignment analysis: Source/CkEcs/DESIGN_SubInstancedCadenceProcessors.md + CkEcs/CLAUDE.md.
//
// Phase contract (load-bearing for cadence-sensitive tests): a bucket's accumulator FREEZES while its view is
// provably empty, so its phase aligns to when the view last became non-empty. Hence no phase offset — one
// would break that wake-alignment.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::cadence
{
    inline constexpr int32 BucketCount = 7;

    // Seconds; index 0 = every tick. Tune only alongside the [EDITOR-VERIFY] benchmark in the design doc.
    // Doubles rather than FCk_Time because FCk_Time is a reflected USTRUCT with no constexpr ctor.
    inline constexpr double BucketIntervalsSeconds[BucketCount] = { 0.0, 0.1, 0.25, 0.5, 1.0, 2.0, 4.0 };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Bucket-membership view filter. T_CadenceKey is the feature's typesafe handle type, doubling as the
    // cadence family key so one feature's buckets never collide with another's.
    template <typename T_CadenceKey, int32 T_BucketIndex>
    struct TTag_CadenceBucket : public TTag<TTag_CadenceBucket<T_CadenceKey, T_BucketIndex>>
    {
        static_assert(T_BucketIndex >= 0 && T_BucketIndex < cadence::BucketCount,
            "Bucket index outside cadence::BucketIntervalsSeconds");
    };

    // Pending immediate first evaluation — only ever coexists with a TRANSIENT bucket-0 membership.
    template <typename T_CadenceKey>
    struct TTag_CadenceFirstEval : public TTag<TTag_CadenceFirstEval<T_CadenceKey>> { };
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::cadence
{
    // Largest bucket <= the requested interval, so an entity never updates SLOWER than it asked for.
    inline auto
    Get_QuantizedBucketIndex(
        const FCk_Time& InRequestedInterval) -> int32
    {
        const auto RequestedSeconds = InRequestedInterval.Get_Seconds();

        auto QuantizedIndex = int32{0};
        for (auto Index = int32{1}; Index < BucketCount; ++Index)
        {
            if (BucketIntervalsSeconds[Index] <= RequestedSeconds)
            { QuantizedIndex = Index; }
        }

        return QuantizedIndex;
    }

    // ----------------------------------------------------------------------------------------------------------------

    namespace detail
    {
        template <typename T_CadenceKey, int32 T_BucketIndex>
        auto
        AddBucketTagIfMatching(
            FCk_Handle& InHandle,
            int32 InBucketIndex) -> void
        {
            if (InBucketIndex == T_BucketIndex)
            { InHandle.Add<TTag_CadenceBucket<T_CadenceKey, T_BucketIndex>>(); }
        }

        template <typename T_CadenceKey, int32... T_BucketIndices>
        auto
        AddBucketTag(
            FCk_Handle& InHandle,
            int32 InBucketIndex,
            std::integer_sequence<int32, T_BucketIndices...>) -> void
        {
            (AddBucketTagIfMatching<T_CadenceKey, T_BucketIndices>(InHandle, InBucketIndex), ...);
        }
    }

    // Add-time bucketing: quantizes, tags the entity with its bucket, and — for nonzero buckets — arms the
    // immediate first evaluation (transient bucket-0 membership + the first-eval tag). The consumer's shared
    // eval body MUST call TryConsume_FirstEval at its top or bucket 0 pump-drains the armed entities forever.
    template <typename T_CadenceKey>
    auto
    AddCadenceTags(
        FCk_Handle& InHandle,
        const FCk_Time& InRequestedInterval) -> int32
    {
        const auto BucketIndex = Get_QuantizedBucketIndex(InRequestedInterval);

        detail::AddBucketTag<T_CadenceKey>(InHandle, BucketIndex, std::make_integer_sequence<int32, BucketCount>{});

        if (BucketIndex != 0)
        {
            InHandle.Add<TTag_CadenceBucket<T_CadenceKey, 0>>();
            InHandle.Add<TTag_CadenceFirstEval<T_CadenceKey>>();
        }

        return BucketIndex;
    }

    // Call at the TOP of the shared eval body, every bucket. Removing a view-include tag mid-iteration is
    // the established MarkedDirtyBy-consumption pattern — fragment pools are in_place_delete, so it is safe.
    template <typename T_CadenceKey>
    auto
    TryConsume_FirstEval(
        FCk_Handle& InHandle) -> void
    {
        if (NOT InHandle.Has<TTag_CadenceFirstEval<T_CadenceKey>>())
        { return; }

        InHandle.Remove<TTag_CadenceFirstEval<T_CadenceKey>>();
        InHandle.Remove<TTag_CadenceBucket<T_CadenceKey, 0>>();
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace detail
    {
        // Buckets 1..N carry NO dirty marker: TProcessorBase::Pump() bypasses the tick rate entirely, so a
        // pump-eligible nonzero bucket would evaluate its whole population outside its cadence.
        template <typename T_CadenceKey, int32 T_BucketIndex>
        struct TCadenceBucketDirtyTraits { };

        // Bucket 0 drains freshly-armed first evaluations in the same frame's pump passes — bounded because
        // the eval body consumes the marker via cadence::TryConsume_FirstEval.
        template <typename T_CadenceKey>
        struct TCadenceBucketDirtyTraits<T_CadenceKey, 0>
        {
            using MarkedDirtyBy = TTag_CadenceFirstEval<T_CadenceKey>;
        };

        // Bucket 0 declares NOTHING, so it hits the exact ZeroSecond every-tick fast path.
        template <int32 T_BucketIndex>
        struct TCadenceBucketRateTraits
        {
            static constexpr FCk_Time TickRate = ck::time::Seconds(cadence::BucketIntervalsSeconds[T_BucketIndex]);
        };

        template <>
        struct TCadenceBucketRateTraits<0> { };
    }

    // ----------------------------------------------------------------------------------------------------------------
    // The per-bucket sub-processor base. The consumer declares ONE class template over the bucket index,
    // passing itself twice (CRTP instance + the template itself, for the RunAfter chain). Authoring recipe
    // (declaration shape, dependent-base ctor inheritance, registration order): CkEcs/CLAUDE.md; reference
    // consumer: CkVisibleRange. T_HandleType doubles as the cadence family key, so two independent cadence
    // families over one handle type are unsupported (and see the TickRate ambiguity note in CkProcessor.h).
    // ----------------------------------------------------------------------------------------------------------------
    template <
        typename T_DerivedProcessor,
        template <int32> class T_DerivedProcessorTemplate,
        int32 T_BucketIndex,
        typename T_HandleType,
        typename... T_Fragments>
    class TCadenceBucketProcessor
        : public ck_exp::TProcessor<
              T_DerivedProcessor,
              T_HandleType,
              T_Fragments...,
              TTag_CadenceBucket<T_HandleType, T_BucketIndex>>
        , public detail::TCadenceBucketDirtyTraits<T_HandleType, T_BucketIndex>
        , public detail::TCadenceBucketRateTraits<T_BucketIndex>
    {
    public:
        CK_GENERATED_BODY(TCadenceBucketProcessor);

    public:
        using Super = ck_exp::TProcessor<
            T_DerivedProcessor,
            T_HandleType,
            T_Fragments...,
            TTag_CadenceBucket<T_HandleType, T_BucketIndex>>;

        static constexpr auto BucketIndex = T_BucketIndex;

        // Cadence buckets SAMPLE — replaying DoTick per missed interval would re-evaluate the same state.
        static constexpr auto TickCatchUpPolicy = ECk_ProcessorTickCatchUp::SampleLatestOnly;

        // A total order over the sub-instances, so their shared read-write fragments never trip the graph
        // builder's write-write-conflict warnings/auto-edges.
        using RunAfter = std::conditional_t<
            T_BucketIndex == 0,
            TDepList<>,
            TDepList<T_DerivedProcessorTemplate<(T_BucketIndex > 0 ? T_BucketIndex - 1 : 0)>>>;

    public:
        using Super::Super;
    };

    // ----------------------------------------------------------------------------------------------------------------

    namespace detail
    {
        template <template <int32> class T_BucketedProcessor, typename T_Sequence, typename... T_ExtraDeps>
        struct TCadenceBucketDepListImpl;

        template <template <int32> class T_BucketedProcessor, int32... T_BucketIndices, typename... T_ExtraDeps>
        struct TCadenceBucketDepListImpl<T_BucketedProcessor, std::integer_sequence<int32, T_BucketIndices...>, T_ExtraDeps...>
        {
            using Type = TDepList<T_BucketedProcessor<T_BucketIndices>..., T_ExtraDeps...>;
        };
    }

    // A TDepList over every bucket instantiation (+ any extra deps) — for downstream processors that must
    // run after the whole bucket family, e.g. `using RunAfter = TCadenceBucketDepList<FProcessor_X_Bucket>;`.
    template <template <int32> class T_BucketedProcessor, typename... T_ExtraDeps>
    using TCadenceBucketDepList = typename detail::TCadenceBucketDepListImpl<
        T_BucketedProcessor,
        std::make_integer_sequence<int32, cadence::BucketCount>,
        T_ExtraDeps...>::Type;
}

// --------------------------------------------------------------------------------------------------------------------

#define CK_REGISTER_CADENCE_BUCKET_PROCESSORS(_ProcessorTemplate_)                                       \
    static_assert(ck::cadence::BucketCount == 7,                                                         \
        "CK_REGISTER_CADENCE_BUCKET_PROCESSORS registers exactly 7 buckets — keep it in lockstep with "  \
        "ck::cadence::BucketIntervalsSeconds");                                                          \
    CK_REGISTER_PROCESSOR(_ProcessorTemplate_<0>);                                                       \
    CK_REGISTER_PROCESSOR(_ProcessorTemplate_<1>);                                                       \
    CK_REGISTER_PROCESSOR(_ProcessorTemplate_<2>);                                                       \
    CK_REGISTER_PROCESSOR(_ProcessorTemplate_<3>);                                                       \
    CK_REGISTER_PROCESSOR(_ProcessorTemplate_<4>);                                                       \
    CK_REGISTER_PROCESSOR(_ProcessorTemplate_<5>);                                                       \
    CK_REGISTER_PROCESSOR(_ProcessorTemplate_<6>)

// --------------------------------------------------------------------------------------------------------------------
