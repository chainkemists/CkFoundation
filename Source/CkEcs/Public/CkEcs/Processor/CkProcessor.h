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

// How TProcessorBase::Tick treats accumulated time spanning multiple tick-rate intervals in one frame (a
// hitch, or a wake after a long empty-view skip). Opt in on the derived processor with
//     static constexpr auto TickCatchUpPolicy = ECk_ProcessorTickCatchUp::SampleLatestOnly;
enum class ECk_ProcessorTickCatchUp : uint8
{
    // Replay DoTick once per elapsed whole interval — fixed-timestep integration semantics.
    ReplayMissedTicks,

    // Fire DoTick ONCE with the summed elapsed intervals (phase remainder preserved, no time lost). For
    // sampling — not integrating — processors, where re-sampling the same state N times is pure waste.
    SampleLatestOnly,
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    // Accumulates visited-entity counts across a composite processor's ORDERED sub-pumps — call Add in
    // pipeline order, since folding the sub Pump() calls into one argument pack would lose that order.
    // Propagates the -1 "unknown" sentinel (FTickable_Concept::Pump): once unknown, the total stays
    // unknown, so the scheduler keeps treating the composite's pump as having done work.
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

    // A processor with a per-type FIXED cadence declares ONE line and the base derives the rest (throttle,
    // ZeroSecond fast path, catch-up):
    //     static constexpr FCk_Time TickRate = ck::time::Hz(4);   // factories in CkCore/Time/CkTime.h
    // Misuse is a compile error (see the static_asserts in Get_TickRate). Known residual: two bases both
    // declaring TickRate makes lookup ambiguous, which reads as "absent" and silently degrades to every
    // tick — don't stack cadence mixins.

    // --------------------------------------------------------------------------------------------------------------------

    namespace processor
    {
        // What Get_MaxReplayedTicks answers when the derived processor declares no clamp: replay every elapsed
        // interval, which is exactly what every processor did before the trait existed.
        inline constexpr auto UnlimitedReplayedTicks = TNumericLimits<int32>::Max();

        // Declared here and defined out of line so the shared base does not pull a log header into every
        // processor translation unit.
        CKECS_API auto
        Report_ClampedCatchUpReplay(
            const FString& InProcessorName,
            int32 InMaxReplayedTicks,
            int32 InDroppedTicks) -> void;
    }

    // A ReplayMissedTicks processor may additionally bound how many DoTick calls ONE Tick replays after a hitch:
    //     static constexpr int32 MaxReplayedTicks = 4;
    // Whole intervals past the bound are drained WITHOUT ticking — dropped, not deferred — so a burst cannot
    // outlive the frame that caused it and a consumer woken by the processor cannot receive an unbounded
    // backlog in one go. Declaring nothing keeps today's unlimited replay, so this changes no existing
    // processor. It is orthogonal to TickCatchUpPolicy: SampleLatestOnly already collapses the backlog and
    // ignores the clamp.

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

        // The derived's compile-time TickRate trait, else ZeroSecond (every tick). There is no runtime setter.
        auto
        Get_TickRate() const -> TimeType;

    private:
        static constexpr auto
        Get_TickCatchUpPolicy() -> ECk_ProcessorTickCatchUp;

        // The derived's compile-time MaxReplayedTicks trait, else UnlimitedReplayedTicks.
        static constexpr auto
        Get_MaxReplayedTicks() -> int32;

    private:
        RegistryType _Registry;

        TimeType _RemainingDeltaTFromLastFrame;

    private:
        int32 _TotalTicks = 0;

    protected:
        HandleType _TransientEntity;

        // Entities visited by the most recent DoTick. A custom DoTick override that doesn't report leaves
        // the -1 sentinel Pump() sets, which the scheduler treats as "did work" (FTickable_Concept::Pump).
        int32 _LastVisitedCount = -1;

    private:
        CK_ENABLE_SFINAE_THIS(DerivedType);

    public:
        CK_PROPERTY_GET(_TotalTicks);
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

        // The scheduler's trait harvest compares &Derived::DoTick against this type to detect a custom
        // (shadowing) DoTick — only unshadowed processors are eligible for the empty-view skip.
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
    constexpr auto
        TProcessorBase<T_DerivedProcessor>::
        Get_MaxReplayedTicks()
        -> int32
    {
        static_assert(requires { DerivedType::MaxReplayedTicks; } or NOT requires { &DerivedType::MaxReplayedTicks; },
            "MaxReplayedTicks declared as an instance member — the base can only see a compile-time trait. "
            "Spell it: static constexpr int32 MaxReplayedTicks = N;");

        if constexpr (requires { DerivedType::MaxReplayedTicks; })
        {
            static_assert(std::is_same_v<std::remove_const_t<decltype(DerivedType::MaxReplayedTicks)>, int32>,
                "MaxReplayedTicks must be an int32 — it counts DoTick calls, not seconds");

            static_assert(DerivedType::MaxReplayedTicks > 0,
                "MaxReplayedTicks must be positive — a zero or negative bound would stop the processor ticking "
                "at all. Declare no trait for the unlimited default.");

            return DerivedType::MaxReplayedTicks;
        }
        else
        { return processor::UnlimitedReplayedTicks; }
    }

    template <typename T_DerivedProcessor>
    auto
        TProcessorBase<T_DerivedProcessor>::
        Get_TickRate() const
        -> TimeType
    {
        static_assert(requires { DerivedType::TickRate; } or NOT requires { &DerivedType::TickRate; },
            "TickRate declared as an instance member — the base can only see a compile-time trait. "
            "Spell it: static constexpr FCk_Time TickRate = ck::time::Hz(N); (or ck::time::Seconds(S))");
        static_assert(NOT requires { typename DerivedType::TickRate; },
            "TickRate declared as a TYPE — the trait is a value. "
            "Spell it: static constexpr FCk_Time TickRate = ck::time::Hz(N); (or ck::time::Seconds(S))");

        if constexpr (requires { DerivedType::TickRate; })
        {
            static_assert(std::is_same_v<std::remove_const_t<decltype(DerivedType::TickRate)>, FCk_Time>,
                "TickRate must be an FCk_Time from ck::time::Hz(N) / ck::time::Seconds(S) — a raw number "
                "doesn't self-document its unit. Spell it: static constexpr FCk_Time TickRate = ck::time::Hz(N);");

            // A non-constexpr TickRate fails to initialize this constexpr, and the compiler names the read.
            constexpr FCk_Time TraitValue = DerivedType::TickRate;
            return TraitValue;
        }
        else
        { return TimeType::ZeroSecond(); }
    }

    template <typename T_DerivedProcessor>
    auto
        TProcessorBase<T_DerivedProcessor>::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        // Registry-teardown window (snapshot load level transition): once the transient entity is destroyed no
        // handle can be built for ANY iterated entity, so skip the tick entirely. Destruction-drain frames are
        // unaffected — the transient outlives the drain.
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
        else if constexpr (Get_MaxReplayedTicks() == processor::UnlimitedReplayedTicks)
        {
            while(AdjustedTickRate >= TickRate)
            {
                AdjustedTickRate -= TickRate;
                ++_TotalTicks;

                This()->DoTick(TickRate);
            }
        }
        else
        {
            constexpr auto MaxReplayedTicks = Get_MaxReplayedTicks();

            auto ReplayedTicks = int32{0};

            while (AdjustedTickRate >= TickRate && ReplayedTicks < MaxReplayedTicks)
            {
                AdjustedTickRate -= TickRate;
                ++ReplayedTicks;
                ++_TotalTicks;

                This()->DoTick(TickRate);
            }

            // Draining rather than leaving the backlog in the accumulator is what makes the clamp a bound
            // instead of a deferral: carrying it forward would replay the same burst next frame.
            auto DroppedTicks = int32{0};

            while (AdjustedTickRate >= TickRate)
            {
                AdjustedTickRate -= TickRate;
                ++DroppedTicks;
            }

            if (DroppedTicks > 0)
            {
                processor::Report_ClampedCatchUpReplay(
                    Get_RuntimeTypeToString<DerivedType>(), MaxReplayedTicks, DroppedTicks);
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
        // Registry-teardown window: report "no work" so the scheduler stops pumping a dying registry.
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

        // The scheduler's trait harvest compares &Derived::DoTick against this type to detect a custom
        // (shadowing) DoTick — only unshadowed processors are eligible for the empty-view skip.
        using GeneratedDoTickHost = TProcessor;

        // ----- TIgnoreInEditor dual-view fragment lists -----
        // Editor variant: drop TIgnoreInEditor<...> entries entirely, then require FTag_EditorOnlyEntity
        // so the view scopes to editor entities only.
        using EditorVariantFragments = entt::type_list_cat_t<
            std::conditional_t<
                ck::detail::TIsIgnoreInEditor<T_Fragments>::value,
                entt::type_list<>,
                entt::type_list<T_Fragments>
            >...,
            entt::type_list<ck::FTag_EditorOnlyEntity>
        >;

        // Runtime variant: unwrap TIgnoreInEditor<...> to its inner, then exclude FTag_EditorOnlyEntity
        // so the view scopes to runtime entities only.
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

        // TIgnoreInEditor dispatch helpers, instantiated only when TAnyIgnoreInEditor_v. The split exists so
        // the inner template sees all three packs (variant fragments, policies, components) at once.
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
            // The TransientEntity carries FTag_EditorOnlyEntity in editor worlds; run only the matching
            // variant. Both share one ForEachEntity body — TIgnoreInEditor changes which entities are
            // visited, never the parameter shape (enforced by the static_assert above).
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
