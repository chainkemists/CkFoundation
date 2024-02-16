#pragma once

#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkProfile/Stats/CkStats.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_STATS_GROUP(TEXT("ForEachEntity"), STATGROUP_CkProcessors_Details, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("Tick"), STATGROUP_CkProcessors, STATCAT_Advanced);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_DerivedProcessor, typename... T_Fragments>
    class TProcessor
    {
        CK_GENERATED_BODY(TProcessor<T_DerivedProcessor COMMA T_Fragments...>);

    public:
        CK_DEFINE_STAT(STAT_ForEachEntity, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors_Details);
        CK_DEFINE_STAT(STAT_Tick, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors);

    public:
        using EntityType = FCk_Entity;
        using TimeType = FCk_Time;
        using HandleType = FCk_Handle;
        using RegistryType = FCk_Registry;
        using DerivedType = T_DerivedProcessor;

    public:
        explicit TProcessor(RegistryType InRegistry);

    public:
        auto Tick(TimeType InDeltaT) -> void;

    private:
        template <typename... T_ComponentsOnly>
        auto DoTick(TimeType InDeltaT, entt::type_list<T_ComponentsOnly...>) -> void;

    private:
        CK_ENABLE_SFINAE_THIS(DerivedType);

    protected:
        RegistryType _Registry;
    };
}

// --------------------------------------------------------------------------------------------------------------------
// Definitions

namespace ck
{
    template <typename T_DerivedProcessor, typename ... T_Fragments>
    TProcessor<T_DerivedProcessor, T_Fragments...>::
        TProcessor(
            RegistryType InRegistry)
        : _Registry(InRegistry)
    {
    }

    template <typename T_DerivedProcessor, typename ... T_Fragments>
    auto
        TProcessor<T_DerivedProcessor, T_Fragments...>::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        using ViewType = decltype(_Registry.View<T_Fragments...>());
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

        _Registry.View<T_Fragments...>().ForEach([&](EntityType InEntity, T_ComponentsOnly&... InComponents)
        {
            CK_STAT(STAT_ForEachEntity);

            auto Handle = HandleType{InEntity, _Registry};
            This()->ForEachEntity(InDeltaT, Handle, InComponents...);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_exp
{
    template <typename T_DerivedProcessor, typename T_HandleType, typename... T_Fragments>
    class TProcessor
    {
        CK_GENERATED_BODY(TProcessor<T_DerivedProcessor COMMA T_Fragments...>);

    public:
        CK_DEFINE_STAT(STAT_ForEachEntity, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors_Details);
        CK_DEFINE_STAT(STAT_Tick, T_DerivedProcessor, FStatGroup_STATGROUP_CkProcessors);

    public:
        using EntityType = FCk_Entity;
        using TimeType = FCk_Time;
        using HandleType = T_HandleType;
        using RegistryType = FCk_Registry;
        using DerivedType = T_DerivedProcessor;

    public:
        explicit TProcessor(RegistryType InRegistry);

    public:
        auto Tick(TimeType InDeltaT) -> void;

    private:
        template <typename... T_ComponentsOnly>
        auto DoTick(TimeType InDeltaT, entt::type_list<T_ComponentsOnly...>) -> void;

    private:
        CK_ENABLE_SFINAE_THIS(DerivedType);

    protected:
        RegistryType _Registry;
    };
}

// --------------------------------------------------------------------------------------------------------------------
// Definitions

namespace ck_exp
{
    template <typename T_DerivedProcessor, typename T_HandleType, typename ... T_Fragments>
    TProcessor<T_DerivedProcessor, T_HandleType, T_Fragments...>::
        TProcessor(
            RegistryType InRegistry)
        : _Registry(InRegistry)
    {
    }

    template <typename T_DerivedProcessor, typename T_HandleType, typename ... T_Fragments>
    auto
        TProcessor<T_DerivedProcessor, T_HandleType, T_Fragments...>::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        using ViewType = decltype(_Registry.View<T_Fragments...>());
        using ComponentsOnly = typename ViewType::template FragmentsOnly<T_Fragments...>;

        DoTick(InDeltaT, ComponentsOnly{});
    }

    template <typename T_DerivedProcessor, typename T_HandleType, typename ... T_Fragments>
    template <typename ... T_ComponentsOnly>
    auto
        TProcessor<T_DerivedProcessor, T_HandleType, T_Fragments...>::
        DoTick(
            TimeType InDeltaT,
            entt::type_list<T_ComponentsOnly...>)
        -> void
    {
        CK_STAT(STAT_Tick);

        _Registry.View<T_Fragments...>().ForEach([&](EntityType InEntity, T_ComponentsOnly&... InComponents)
        {
            CK_STAT(STAT_ForEachEntity);

            auto TypeSafeHandle = ck::StaticCast<HandleType>(FCk_Handle{InEntity, _Registry});
            This()->ForEachEntity(InDeltaT, TypeSafeHandle, InComponents...);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
