#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Processor/CkProcessor_AccessPolicy.h"
#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Tag/CkTag_EditorOnly.h"

#include "CkEcs/Scheduler/CkProcessorDescriptor.h"
#include "CkEcs/Scheduler/CkSchedulerDebugData.h"

#include "CkProfile/Stats/CkStats.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("ForEachEntity"), STATGROUP_CkProcessors_Details, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("Tick"), STATGROUP_CkProcessors, STATCAT_Advanced);

// --------------------------------------------------------------------------------------------------------------------

// Selects how TProcessorBase::Tick treats accumulated time spanning multiple tick-rate intervals in one
// frame (a hitch, or a processor woken after a long empty-view skip). Declared as an opt-in trait on the
// derived processor:
//
//     static constexpr auto TickCatchUpPolicy = ECk_ProcessorTickCatchUp::SampleLatestOnly;
//
// Absent (every shipped processor) => ReplayMissedTicks, the original catch-up loop, unchanged.
enum class ECk_ProcessorTickCatchUp : uint8
{
    // Replay DoTick once per elapsed whole interval — fixed-timestep integration semantics.
    ReplayMissedTicks,

    // Fire DoTick ONCE with the summed elapsed whole intervals; the phase remainder is preserved and no
    // time is lost. For sampling (non-integrating) processors — cadence buckets — where re-sampling the
    // same state N times after a hitch is pure waste.
    SampleLatestOnly,
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    // Accumulates visited-entity counts across a composite processor's ORDERED sub-pumps (call Add
    // in pipeline order — folding the sub Pump() calls into one argument pack would lose the
    // required evaluation order), propagating the -1 "unknown" sentinel (see FTickable_Concept::
    // Pump): once any sub-count is unknown the total stays unknown, so the scheduler keeps treating
    // the composite's pump as having done work.
    struct FPumpVisitedCountAccumulator
    {
        CK_GENERATED_BODY(FPumpVisitedCountAccumulator);

    private:
        int32 _Total = 0;

    public:
        auto Add(int32 InSubVisited) -> void
        {
            _Total = (_Total < 0 or InSubVisited < 0) ? -1 : _Total + InSubVisited;
        }

    public:
        CK_PROPERTY_GET(_Total);
    };

    // --------------------------------------------------------------------------------------------------------------------

    namespace detail
    {
        // Deliberately declared, never defined, and not constexpr: calling it poisons constant evaluation,
        // so an invalid tick-rate literal fails to COMPILE with this name in the diagnostic. Never callable
        // at runtime — every FTickRate construction is consteval.
        auto TickRate_MustBePositive() -> void;

        consteval auto
        TickRate_IntervalFromFrequency(
            double InCyclesPerSecond) -> double
        {
            // Reject before dividing — 0 Hz must hit the named diagnostic via the base ctor, not a
            // division-by-zero whose constant-evaluation handling is compiler-dependent.
            return InCyclesPerSecond > 0.0 ? 1.0 / InCyclesPerSecond : -1.0;
        }
    }

    // Compile-time tick-rate literals. FCk_Time is a reflected USTRUCT with no constexpr constructor, so
    // the trait an author declares must carry the rate in a constexpr-legal type; TProcessorBase converts
    // it to FCk_Time once per Tick (Get_TickRate). A processor with a per-type FIXED cadence declares ONE
    // line and the base derives everything else (throttle, ZeroSecond fast path, catch-up, registration —
    // all untouched):
    //
    //     static constexpr auto TickRate = ck::Hz{4};          // 4 evaluations per second
    //     static constexpr auto TickRate = ck::Seconds{0.25};  // the same rate, interval spelling
    //
    // Misuse is a compile error, not a silent fallback: zero/negative rates fail in the consteval ctor,
    // non-literal spellings (raw double, FCk_Time), non-static and non-constexpr declarations fail
    // static_asserts in Get_TickRate, and calling Set_TickRate on a trait-declaring processor is
    // ill-formed. Known residual: a processor inheriting TWO bases that both declare TickRate makes the
    // name lookup ambiguous, which the requires-probes report as "absent" — the processor degrades to the
    // every-tick default instead of erroring. Don't stack cadence mixins.
    struct FTickRate
    {
        consteval explicit FTickRate(
            double InIntervalSeconds)
            : _IntervalSeconds(InIntervalSeconds)
        {
            if (NOT (InIntervalSeconds > 0.0))
            { detail::TickRate_MustBePositive(); }
        }

        constexpr auto Get_IntervalSeconds() const -> double { return _IntervalSeconds; }

    private:
        double _IntervalSeconds;
    };

    struct Seconds : FTickRate
    {
        consteval explicit Seconds(
            double InSeconds)
            : FTickRate{InSeconds}
        { }
    };

    struct Hz : FTickRate
    {
        consteval explicit Hz(
            double InCyclesPerSecond)
            : FTickRate{detail::TickRate_IntervalFromFrequency(InCyclesPerSecond)}
        { }
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor>
    class TProcessorBase
    {
    public:
        CK_GENERATED_BODY(TProcessorBase);

    public:
        using EntityType = FCk_Entity;
        using TimeType = FCk_Time;
        using HandleType = FCk_Handle;
        using RegistryType = FCk_Registry;
        using DerivedType = T_DerivedProcessor;

    public:
        explicit TProcessorBase(
            const RegistryType& InRegistry);

    protected:
        ~TProcessorBase() = default;

    public:
        auto
        Tick(TimeType InDeltaT) -> void;

        auto
        Pump() -> int32;

        // The EFFECTIVE tick rate: the compile-time TickRate trait when the derived declares one (see the
        // ck::FTickRate literals above), else the runtime _TickRate member (default ZeroSecond = every
        // tick). NOT static constexpr — FCk_Time is a reflected USTRUCT with no constexpr ctor, so the
        // literal's DOUBLE is the compile-time artifact and this materializes it per call.
        auto
        Get_TickRate() const -> TimeType;

        // Runtime rate for processors WITHOUT the compile-time TickRate trait (e.g. an instance configured
        // by a registration factory). Calling this on a trait-declaring processor is a compile error — the
        // written value would be silently ignored.
        auto
        Set_TickRate(TimeType InTickRate) -> ThisType&;

        // Seeds the tick accumulator so a rated processor's FIRST fire lands (TickRate - Offset) after
        // this call instead of a full TickRate — staggering same-rate processors across frames. Meaningful
        // only when called before the first Tick (a registration factory or a derived ctor); calling later
        // re-phases the cadence from that moment. Default-unset => zero behavior change.
        auto
        Set_TickPhaseOffset(TimeType InPhaseOffset) -> ThisType&;

    private:
        static constexpr auto
        Get_TickCatchUpPolicy() -> ECk_ProcessorTickCatchUp;

    private:
        RegistryType _Registry;

        TimeType _TickRate;
        TimeType _TickPhaseOffset;
        TimeType _RemainingDeltaTFromLastFrame;

    private:
        int32 _TotalTicks = 0;

    protected:
        HandleType _TransientEntity;

        // Entities visited by the most recent DoTick. The standard DoTick bodies (TProcessor,
        // ck_exp::TProcessor, TParallelProcessor) write the exact count; a custom DoTick override
        // that doesn't report leaves the -1 sentinel Pump() sets, which the scheduler treats as
        // "did work" (see FTickable_Concept::Pump).
        int32 _LastVisitedCount = -1;

    private:
        CK_ENABLE_SFINAE_THIS(DerivedType);

    public:
        CK_PROPERTY_GET(_TotalTicks);
        CK_PROPERTY_GET(_TickPhaseOffset);
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename... T_Fragments>
    class TProcessor : public TProcessorBase<T_DerivedProcessor>
    {
        CK_GENERATED_BODY(TProcessor<T_DerivedProcessor COMMA T_Fragments...>);

        static_assert(
            ((detail::TIsExcludedPolicy<T_Fragments>::value ||
              detail::TIsEmptyPolicy<T_Fragments>::value ||
              detail::TIsAccessPolicyWrapped<T_Fragments>::value) && ...),
            "All non-excluded, non-empty fragments in TProcessor must be wrapped "
            "in ck::TReadOnly<T> or ck::TReadWrite<T> for explicit access intent.");

    public:
        CK_DEFINE_STAT(STAT_ForEachEntity, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors_Details);
        CK_DEFINE_STAT(STAT_Tick, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors);

    public:
        using Super = TProcessorBase<T_DerivedProcessor>;
        using EntityType = FCk_Entity;
        using TimeType = FCk_Time;
        using HandleType = FCk_Handle;
        using RegistryType = FCk_Registry;
        using DerivedType = T_DerivedProcessor;
        using FragmentList = entt::type_list<T_Fragments...>;

        // The class that declares the template-generated DoTick. The scheduler's trait harvest
        // compares &Derived::DoTick against this type to detect a custom (shadowing) DoTick —
        // only unshadowed processors are eligible for the main pass' empty-view skip (see
        // ECk_ProcessorEmptyViewPolicy in CkProcessorDescriptor.h).
        using GeneratedDoTickHost = TProcessor;

    public:
        explicit TProcessor(
            const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType InDeltaT) -> void;

    private:
        template <typename... T_PoliciesOnly, typename... T_ComponentsOnly>
        auto DoTick(
            TimeType InDeltaT,
            entt::type_list<T_PoliciesOnly...>,
            entt::type_list<T_ComponentsOnly...>) -> void;

    private:
        CK_ENABLE_SFINAE_THIS(DerivedType);
    };
}

// --------------------------------------------------------------------------------------------------------------------
// Definitions

namespace ck
{
    template <typename T_DerivedProcessor>
    TProcessorBase<T_DerivedProcessor>::
        TProcessorBase(
            const RegistryType& InRegistry)
        : _Registry(InRegistry)
    {
        _TransientEntity = UCk_Utils_EntityLifetime_UE::Get_TransientEntity(_Registry);
    }

    template <typename T_DerivedProcessor>
    auto
        TProcessorBase<T_DerivedProcessor>::
        Set_TickPhaseOffset(
            TimeType InPhaseOffset)
        -> ThisType&
    {
        _TickPhaseOffset = InPhaseOffset;
        _RemainingDeltaTFromLastFrame = InPhaseOffset;

        return *this;
    }

    template <typename T_DerivedProcessor>
    constexpr auto
        TProcessorBase<T_DerivedProcessor>::
        Get_TickCatchUpPolicy()
        -> ECk_ProcessorTickCatchUp
    {
        static_assert(requires { DerivedType::TickCatchUpPolicy; } or NOT requires { &DerivedType::TickCatchUpPolicy; },
            "TickCatchUpPolicy declared as an instance member — the base can only see a compile-time trait. "
            "Spell it: static constexpr auto TickCatchUpPolicy = ECk_ProcessorTickCatchUp::SampleLatestOnly;");

        if constexpr (requires { DerivedType::TickCatchUpPolicy; })
        {
            static_assert(std::is_same_v<std::remove_const_t<decltype(DerivedType::TickCatchUpPolicy)>, ECk_ProcessorTickCatchUp>,
                "TickCatchUpPolicy must be an ECk_ProcessorTickCatchUp value");

            return DerivedType::TickCatchUpPolicy;
        }
        else
        { return ECk_ProcessorTickCatchUp::ReplayMissedTicks; }
    }

    template <typename T_DerivedProcessor>
    auto
        TProcessorBase<T_DerivedProcessor>::
        Get_TickRate() const
        -> TimeType
    {
        static_assert(requires { DerivedType::TickRate; } or NOT requires { &DerivedType::TickRate; },
            "TickRate declared as an instance member — the base can only see a compile-time trait. "
            "Spell it: static constexpr auto TickRate = ck::Hz{N}; (or ck::Seconds{S})");
        static_assert(NOT requires { typename DerivedType::TickRate; },
            "TickRate declared as a TYPE — the trait is a value. "
            "Spell it: static constexpr auto TickRate = ck::Hz{N}; (or ck::Seconds{S})");

        if constexpr (requires { DerivedType::TickRate; })
        {
            constexpr auto TraitIsTickRateLiteral =
                std::is_base_of_v<FTickRate, std::remove_const_t<decltype(DerivedType::TickRate)>>;

            static_assert(TraitIsTickRateLiteral,
                "TickRate must be a ck tick-rate literal — a raw number doesn't self-document its unit and "
                "FCk_Time has no constexpr ctor. Spell it: static constexpr auto TickRate = ck::Hz{N}; "
                "(or ck::Seconds{S})");

            if constexpr (TraitIsTickRateLiteral)
            {
                // Also proves the trait is constexpr-READABLE: a non-constexpr static of literal type
                // fails this assert with the compiler naming the non-constant read.
                static_assert(DerivedType::TickRate.Get_IntervalSeconds() > 0.0,
                    "TickRate must be positive — for an every-tick processor declare no TickRate at all "
                    "(every-tick is the default)");

                return TimeType{DerivedType::TickRate.Get_IntervalSeconds()};
            }
            else
            { return _TickRate; }
        }
        else
        { return _TickRate; }
    }

    template <typename T_DerivedProcessor>
    auto
        TProcessorBase<T_DerivedProcessor>::
        Set_TickRate(
            TimeType InTickRate)
        -> ThisType&
    {
        static_assert(NOT requires { DerivedType::TickRate; },
            "This processor declares the compile-time TickRate trait — Set_TickRate would be silently "
            "ignored. Remove the call (or drop the trait if the rate must be runtime-configured).");

        _TickRate = InTickRate;
        return *this;
    }

    template <typename T_DerivedProcessor>
    auto
        TProcessorBase<T_DerivedProcessor>::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        // Registry-teardown window (snapshot load level transition): once the transient entity is destroyed,
        // no handle can be built for ANY iterated entity (MakeHandle ensures per entity, DoTick bodies that
        // walk _TransientEntity ensure too) — skip the tick entirely, matching FProcessor_ScriptHosted::Tick.
        // Destruction-drain frames are unaffected: the transient is alive until the drain completes.
        if (ck::Is_NOT_Valid(this->_TransientEntity, ck::IsValid_Policy_IncludePendingKill{}))
        { return; }

        const auto TickRate = Get_TickRate();

        auto AdjustedTickRate = InDeltaT + _RemainingDeltaTFromLastFrame;

        if (TickRate == TimeType::ZeroSecond())
        {
            ++_TotalTicks;
            This()->DoTick(InDeltaT);
            AdjustedTickRate = TimeType::ZeroSecond();
        }
        else if constexpr (Get_TickCatchUpPolicy() == ECk_ProcessorTickCatchUp::SampleLatestOnly)
        {
            auto FiredElapsed = TimeType::ZeroSecond();

            while (AdjustedTickRate >= TickRate)
            {
                AdjustedTickRate -= TickRate;
                FiredElapsed += TickRate;
            }

            if (FiredElapsed > TimeType::ZeroSecond())
            {
                ++_TotalTicks;
                This()->DoTick(FiredElapsed);
            }
        }
        else
        {
            while(AdjustedTickRate >= TickRate)
            {
                AdjustedTickRate -= TickRate;
                ++_TotalTicks;

                This()->DoTick(TickRate);
            }
        }

        _RemainingDeltaTFromLastFrame = AdjustedTickRate;
    }

    template <typename T_DerivedProcessor>
    auto
        TProcessorBase<T_DerivedProcessor>::
        Pump()
        -> int32
    {
        // Registry-teardown window: no handles can be built, DoTick is skipped entirely — report
        // "no work" so the scheduler doesn't schedule further pump passes on a dying registry.
        if (ck::Is_NOT_Valid(this->_TransientEntity, ck::IsValid_Policy_IncludePendingKill{}))
        { return 0; }

        ++_TotalTicks;

        _LastVisitedCount = -1;
        This()->DoTick(TimeType::ZeroSecond());
        return _LastVisitedCount;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedProcessor, typename ... T_Fragments>
    TProcessor<T_DerivedProcessor, T_Fragments...>::
        TProcessor(
            const RegistryType& InRegistry)
        : Super(InRegistry)
    { }

    template <typename T_DerivedProcessor, typename ... T_Fragments>
    auto
        TProcessor<T_DerivedProcessor, T_Fragments...>::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        using ViewType = decltype(this->_TransientEntity.template View<detail::UnwrapAccessPolicy_T<T_Fragments>...>());
        using ComponentsOnly = typename ViewType::template FragmentsOnly<detail::UnwrapAccessPolicy_T<T_Fragments>...>;
        using PoliciesOnly = detail::PoliciesOnly<T_Fragments...>;

        DoTick(InDeltaT, PoliciesOnly{}, ComponentsOnly{});
    }

    template <typename T_DerivedProcessor, typename ... T_Fragments>
    template <typename ... T_PoliciesOnly, typename ... T_ComponentsOnly>
    auto
        TProcessor<T_DerivedProcessor, T_Fragments...>::
        DoTick(
            TimeType InDeltaT,
            entt::type_list<T_PoliciesOnly...>,
            entt::type_list<T_ComponentsOnly...>)
        -> void
    {
        CK_STAT(STAT_Tick);

        auto EntityCount = int32{0};

        this->_TransientEntity.template View<detail::UnwrapAccessPolicy_T<T_Fragments>...>().ForEach(
            [&](EntityType InEntity, T_ComponentsOnly&... InComponents)
        {
            CK_STAT(STAT_ForEachEntity);

            ++EntityCount;

            auto Handle = ck::MakeHandle(InEntity, this->_TransientEntity);
            This()->ForEachEntity(InDeltaT, Handle,
                static_cast<typename detail::TResolveConstness<T_PoliciesOnly, T_ComponentsOnly>::Type>(InComponents)...);
        });

        this->_LastVisitedCount = EntityCount;

#if !UE_BUILD_SHIPPING
        GDebug_LastProcessedEntityCount = EntityCount;
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_exp
{
    template <typename T_DerivedProcessor, typename T_HandleType, typename... T_Fragments>
    requires(std::is_base_of_v<FCk_Handle, T_HandleType>)
    class TProcessor : public ck::TProcessorBase<T_DerivedProcessor>
    {
        CK_GENERATED_BODY(TProcessor<T_DerivedProcessor COMMA T_HandleType COMMA T_Fragments...>);

        // ----- Per-fragment validation -----
        // A fragment is acceptable iff it's an excluded set, an empty tag, an access-wrapped
        // fragment, or a TIgnoreInEditor wrapping one of {empty tag, TExclude<...>}. We
        // forbid TIgnoreInEditor around non-empty fragments because the ForEachEntity
        // signature can't conditionally drop a parameter based on the visited entity's world.
        template <typename T_Fragment>
        struct TIsValidFragment
        {
            static constexpr auto value =
                ck::detail::TIsExcludedPolicy<T_Fragment>::value ||
                ck::detail::TIsEmptyPolicy<T_Fragment>::value ||
                ck::detail::TIsAccessPolicyWrapped<T_Fragment>::value ||
                (ck::detail::TIsIgnoreInEditor<T_Fragment>::value &&
                 ck::detail::TIsValidIgnoreInEditorInner<
                     ck::detail::UnwrapIgnoreInEditor_T<T_Fragment>>::value);
        };

        static_assert(
            (TIsValidFragment<T_Fragments>::value && ...),
            "All non-excluded, non-empty fragments in ck_exp::TProcessor must be wrapped "
            "in ck::TReadOnly<T> or ck::TReadWrite<T> for explicit access intent. "
            "TIgnoreInEditor<T> is allowed only when T is an empty tag or a TExclude<...>.");

    public:
        CK_DEFINE_STAT(STAT_ForEachEntity, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors_Details);
        CK_DEFINE_STAT(STAT_Tick, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors);

    public:
        using Super = ck::TProcessorBase<T_DerivedProcessor>;
        using EntityType = FCk_Entity;
        using TimeType = FCk_Time;
        using HandleType = T_HandleType;
        using RegistryType = FCk_Registry;
        using DerivedType = T_DerivedProcessor;
        using FragmentList = entt::type_list<T_Fragments...>;

        // The class that declares the template-generated DoTick. The scheduler's trait harvest
        // compares &Derived::DoTick against this type to detect a custom (shadowing) DoTick —
        // only unshadowed processors are eligible for the main pass' empty-view skip (see
        // ECk_ProcessorEmptyViewPolicy in CkProcessorDescriptor.h).
        using GeneratedDoTickHost = TProcessor;

        // ----- TIgnoreInEditor dual-view fragment lists -----
        // Editor variant: drop TIgnoreInEditor<...> entries entirely (criteria not applied for
        // editor-world entities), then append FTag_EditorOnlyEntity as a required tag so the
        // view scopes to editor entities only.
        using EditorVariantFragments = entt::type_list_cat_t<
            std::conditional_t<
                ck::detail::TIsIgnoreInEditor<T_Fragments>::value,
                entt::type_list<>,
                entt::type_list<T_Fragments>
            >...,
            entt::type_list<ck::FTag_EditorOnlyEntity>
        >;

        // Runtime variant: unwrap TIgnoreInEditor<...> to its inner (criteria applied as if
        // the wrapper weren't there), then append TExclude<FTag_EditorOnlyEntity> so the view
        // scopes to runtime entities only.
        using RuntimeVariantFragments = entt::type_list_cat_t<
            entt::type_list<ck::detail::UnwrapIgnoreInEditor_T<T_Fragments>>...,
            entt::type_list<ck::TExclude<ck::FTag_EditorOnlyEntity>>
        >;

    public:
        explicit TProcessor(
            const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType InDeltaT) -> void;

    private:
        template <typename... T_PoliciesOnly, typename... T_ComponentsOnly>
        auto DoTick(
            TimeType InDeltaT,
            entt::type_list<T_PoliciesOnly...>,
            entt::type_list<T_ComponentsOnly...>) -> void;

        // TIgnoreInEditor dispatch helpers — invoked only when TAnyIgnoreInEditor_v is true.
        // DoTick_Variant takes a per-variant fragment pack (editor or runtime), derives the
        // policy/component sub-lists, and forwards to DoTick_Variant_Unpack which actually
        // builds the view and runs ForEachEntity. The split lets the inner template see all
        // three packs (variant fragments, policies, components) so View<> and the lambda
        // signature can be instantiated independently.
        template <typename... T_VariantFragments>
        auto DoTick_Variant(
            TimeType InDeltaT,
            entt::type_list<T_VariantFragments...>) -> void;

        template <typename... T_PoliciesOnly, typename... T_ComponentsOnly, typename... T_VariantFragments>
        auto DoTick_Variant_Unpack(
            TimeType InDeltaT,
            entt::type_list<T_PoliciesOnly...>,
            entt::type_list<T_ComponentsOnly...>,
            entt::type_list<T_VariantFragments...>) -> void;

    private:
        CK_ENABLE_SFINAE_THIS(DerivedType);
    };
}

// --------------------------------------------------------------------------------------------------------------------
// Definitions

namespace ck_exp
{
    template <typename T_DerivedProcessor, typename T_HandleType, typename ... T_Fragments>
    requires(std::is_base_of_v<FCk_Handle, T_HandleType>)
    TProcessor<T_DerivedProcessor, T_HandleType, T_Fragments...>::
        TProcessor(
            const RegistryType& InRegistry)
        : Super(InRegistry)
    {
    }

    template <typename T_DerivedProcessor, typename T_HandleType, typename ... T_Fragments>
    requires(std::is_base_of_v<FCk_Handle, T_HandleType>)
    auto
        TProcessor<T_DerivedProcessor, T_HandleType, T_Fragments...>::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        if constexpr (ck::detail::TAnyIgnoreInEditor_v<T_Fragments...>)
        {
            // Dual-view dispatch. The TransientEntity carries FTag_EditorOnlyEntity in editor
            // worlds; pick the variant that matches and run that view only. Both variants
            // share the same ForEachEntity body — TIgnoreInEditor only changes which entities
            // are visited, not the parameter shape (enforced by the static_assert above).
            if (this->_TransientEntity.template Has<ck::FTag_EditorOnlyEntity>())
            {
                DoTick_Variant(InDeltaT, EditorVariantFragments{});
            }
            else
            {
                DoTick_Variant(InDeltaT, RuntimeVariantFragments{});
            }
        }
        else
        {
            using ViewType = decltype(this->_TransientEntity.template View<ck::detail::UnwrapAccessPolicy_T<T_Fragments>...>());
            using ComponentsOnly = typename ViewType::template FragmentsOnly<ck::detail::UnwrapAccessPolicy_T<T_Fragments>...>;
            using PoliciesOnly = ck::detail::PoliciesOnly<T_Fragments...>;

            DoTick(InDeltaT, PoliciesOnly{}, ComponentsOnly{});
        }
    }

    template <typename T_DerivedProcessor, typename T_HandleType, typename ... T_Fragments>
    requires(std::is_base_of_v<FCk_Handle, T_HandleType>)
    template <typename ... T_PoliciesOnly, typename ... T_ComponentsOnly>
    auto
        TProcessor<T_DerivedProcessor, T_HandleType, T_Fragments...>::
        DoTick(
            TimeType InDeltaT,
            entt::type_list<T_PoliciesOnly...>,
            entt::type_list<T_ComponentsOnly...>)
        -> void
    {
        CK_STAT(STAT_Tick);

        auto EntityCount = int32{0};

        this->_TransientEntity.template View<ck::detail::UnwrapAccessPolicy_T<T_Fragments>...>().ForEach(
            [&](EntityType InEntity, T_ComponentsOnly&... InComponents)
        {
            CK_STAT(STAT_ForEachEntity);

            ++EntityCount;

            auto TypeSafeHandle = ck::StaticCast<HandleType>(ck::MakeHandle(InEntity, this->_TransientEntity));
            This()->ForEachEntity(InDeltaT, TypeSafeHandle,
                static_cast<typename ck::detail::TResolveConstness<T_PoliciesOnly, T_ComponentsOnly>::Type>(InComponents)...);
        });

        this->_LastVisitedCount = EntityCount;

#if !UE_BUILD_SHIPPING
        ck::GDebug_LastProcessedEntityCount = EntityCount;
#endif
    }

    template <typename T_DerivedProcessor, typename T_HandleType, typename ... T_Fragments>
    requires(std::is_base_of_v<FCk_Handle, T_HandleType>)
    template <typename ... T_VariantFragments>
    auto
        TProcessor<T_DerivedProcessor, T_HandleType, T_Fragments...>::
        DoTick_Variant(
            TimeType InDeltaT,
            entt::type_list<T_VariantFragments...>)
        -> void
    {
        // Derive policy/component sub-lists from the variant pack and forward. The view is
        // built on the variant pack inside DoTick_Variant_Unpack — both runtime and editor
        // variants instantiate the view template once each.
        using ViewType = decltype(this->_TransientEntity.template View<ck::detail::UnwrapAccessPolicy_T<T_VariantFragments>...>());
        using ComponentsOnly = typename ViewType::template FragmentsOnly<ck::detail::UnwrapAccessPolicy_T<T_VariantFragments>...>;
        using PoliciesOnly = ck::detail::PoliciesOnly<T_VariantFragments...>;

        DoTick_Variant_Unpack(InDeltaT, PoliciesOnly{}, ComponentsOnly{}, entt::type_list<T_VariantFragments...>{});
    }

    template <typename T_DerivedProcessor, typename T_HandleType, typename ... T_Fragments>
    requires(std::is_base_of_v<FCk_Handle, T_HandleType>)
    template <typename ... T_PoliciesOnly, typename ... T_ComponentsOnly, typename ... T_VariantFragments>
    auto
        TProcessor<T_DerivedProcessor, T_HandleType, T_Fragments...>::
        DoTick_Variant_Unpack(
            TimeType InDeltaT,
            entt::type_list<T_PoliciesOnly...>,
            entt::type_list<T_ComponentsOnly...>,
            entt::type_list<T_VariantFragments...>)
        -> void
    {
        CK_STAT(STAT_Tick);

        auto EntityCount = int32{0};

        this->_TransientEntity.template View<ck::detail::UnwrapAccessPolicy_T<T_VariantFragments>...>().ForEach(
            [&](EntityType InEntity, T_ComponentsOnly&... InComponents)
        {
            CK_STAT(STAT_ForEachEntity);

            ++EntityCount;

            auto TypeSafeHandle = ck::StaticCast<HandleType>(ck::MakeHandle(InEntity, this->_TransientEntity));
            This()->ForEachEntity(InDeltaT, TypeSafeHandle,
                static_cast<typename ck::detail::TResolveConstness<T_PoliciesOnly, T_ComponentsOnly>::Type>(InComponents)...);
        });

        this->_LastVisitedCount = EntityCount;

#if !UE_BUILD_SHIPPING
        ck::GDebug_LastProcessedEntityCount = EntityCount;
#endif
    }
}

// --------------------------------------------------------------------------------------------------------------------
