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

    private:
        RegistryType _Registry;

        TimeType _TickRate;
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
        CK_PROPERTY(_TickRate);
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

        auto AdjustedTickRate = InDeltaT + _RemainingDeltaTFromLastFrame;

        if (_TickRate == TimeType::ZeroSecond())
        {
            ++_TotalTicks;
            This()->DoTick(InDeltaT);
            AdjustedTickRate = TimeType::ZeroSecond();
        }
        else
        {
            while(AdjustedTickRate >= _TickRate)
            {
                AdjustedTickRate -= _TickRate;
                ++_TotalTicks;

                This()->DoTick(_TickRate);
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
