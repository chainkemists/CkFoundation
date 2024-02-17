#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkProfile/Stats/CkStats.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("ForEachEntity"), STATGROUP_CkProcessors_Details, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("Tick"), STATGROUP_CkProcessors, STATCAT_Advanced);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FProcessor
    {
    public:
        using EntityType = FCk_Entity;
        using TimeType = FCk_Time;
        using HandleType = FCk_Handle;
        using RegistryType = FCk_Registry;

    public:
        explicit FProcessor(
            const RegistryType& InRegistry);

    protected:
        ~FProcessor() = default;

    private:
        RegistryType _Registry;

    protected:
        HandleType _TransientEntity;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename... T_Fragments>
    class TProcessor : public FProcessor
    {
        CK_GENERATED_BODY(TProcessor<T_DerivedProcessor COMMA T_Fragments...>);

    public:
        CK_DEFINE_STAT(STAT_ForEachEntity, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors_Details);
        CK_DEFINE_STAT(STAT_Tick, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors);

    public:
        using Super = FProcessor;
        using EntityType = FCk_Entity;
        using TimeType = FCk_Time;
        using HandleType = FCk_Handle;
        using RegistryType = FCk_Registry;
        using DerivedType = T_DerivedProcessor;

    public:
        explicit TProcessor(
            const RegistryType& InRegistry);

    public:
        auto Tick(TimeType InDeltaT) -> void;

    private:
        template <typename... T_ComponentsOnly>
        auto DoTick(TimeType InDeltaT, entt::type_list<T_ComponentsOnly...>) -> void;

    private:
        CK_ENABLE_SFINAE_THIS(DerivedType);
    };
}

// --------------------------------------------------------------------------------------------------------------------
// Definitions

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
        Tick(
            TimeType InDeltaT)
        -> void
    {
        using ViewType = decltype(_TransientEntity.View<T_Fragments...>());
        using ComponentsOnly = typename ViewType::template FragmentsOnly<T_Fragments...>;

        DoTick(InDeltaT, ComponentsOnly{});
    }

    template <typename T_DerivedProcessor, typename ... T_Fragments>
    template <typename ... T_ComponentsOnly>
    auto
        TProcessor<T_DerivedProcessor, T_Fragments...>::
        DoTick(
            TimeType InDeltaT,
            entt::type_list<T_ComponentsOnly...>)
        -> void
    {
        CK_STAT(STAT_Tick);

        _TransientEntity.View<T_Fragments...>().ForEach([&](EntityType InEntity, T_ComponentsOnly&... InComponents)
        {
            CK_STAT(STAT_ForEachEntity);

            auto Handle = ck::MakeHandle(InEntity, _TransientEntity);
            This()->ForEachEntity(InDeltaT, Handle, InComponents...);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_exp
{
    template <typename T_DerivedProcessor, typename T_HandleType, typename... T_Fragments>
    requires(std::is_base_of_v<FCk_Handle, T_HandleType>)
    class TProcessor : public ck::FProcessor
    {
        CK_GENERATED_BODY(TProcessor<T_DerivedProcessor COMMA T_HandleType COMMA T_Fragments...>);

    public:
        CK_DEFINE_STAT(STAT_ForEachEntity, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors_Details);
        CK_DEFINE_STAT(STAT_Tick, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors);

    public:
        using Super = ck::FProcessor;
        using EntityType = FCk_Entity;
        using TimeType = FCk_Time;
        using HandleType = T_HandleType;
        using RegistryType = FCk_Registry;
        using DerivedType = T_DerivedProcessor;

    public:
        explicit TProcessor(
            const RegistryType& InRegistry);

    public:
        auto Tick(TimeType InDeltaT) -> void;

    private:
        template <typename... T_ComponentsOnly>
        auto DoTick(TimeType InDeltaT, entt::type_list<T_ComponentsOnly...>) -> void;

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
        Tick(
            TimeType InDeltaT)
        -> void
    {
        using ViewType = decltype(_TransientEntity.View<T_Fragments...>());
        using ComponentsOnly = typename ViewType::template FragmentsOnly<T_Fragments...>;

        DoTick(InDeltaT, ComponentsOnly{});
    }

    template <typename T_DerivedProcessor, typename T_HandleType, typename ... T_Fragments>
    requires(std::is_base_of_v<FCk_Handle, T_HandleType>)
    template <typename ... T_ComponentsOnly>
    auto
        TProcessor<T_DerivedProcessor, T_HandleType, T_Fragments...>::
        DoTick(
            TimeType InDeltaT,
            entt::type_list<T_ComponentsOnly...>)
        -> void
    {
        CK_STAT(STAT_Tick);

        _TransientEntity.View<T_Fragments...>().ForEach([&](EntityType InEntity, T_ComponentsOnly&... InComponents)
        {
            CK_STAT(STAT_ForEachEntity);

            auto TypeSafeHandle = ck::StaticCast<HandleType>(ck::MakeHandle(InEntity, _TransientEntity));
            This()->ForEachEntity(InDeltaT, TypeSafeHandle, InComponents...);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
