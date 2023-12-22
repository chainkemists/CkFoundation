#pragma once

#include "CkCore/Concepts/CkConcepts.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "CkCore/Macros/CkMacros.h"

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FTicker
    {
        CK_GENERATED_BODY(FTicker);

    public:
        using RegistryType = FCk_Registry;
        using TimeType     = FCk_Time;
        using HandleType   = FCk_Handle;
        using TickableType = concepts::FTickableType;
        using EntityType   = FCk_Entity;

    public:
        template <typename T_Tickable>
        auto Add(T_Tickable&& InTickable) -> HandleType;

        template <typename T_Tickable, typename... T_Args>
        auto Add(T_Args&&... InArgs) -> HandleType;

    public:
        auto Tick(TimeType InDeltaTime) -> void;

    private:
        template <typename T_Tickable>
        auto DoSortTickable() -> void;

    private:
        RegistryType _ProcessorsRegistry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_Tickable>
    auto
        FTicker::
        Add(
            T_Tickable&& InTickable)
        -> HandleType
    {
        const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_ProcessorsRegistry, [&](FCk_Handle InEntity)
        {
            InEntity.Add<TickableType>(std::forward<T_Tickable>(InTickable));
        });

        DoSortTickable<T_Tickable>();

        return NewEntity;
    }

    template <typename T_Tickable, typename ... T_Args>
    auto
        FTicker::
        Add(
            T_Args&&... InArgs)
        -> HandleType
    {
        const auto NewEntity = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_ProcessorsRegistry, [&](FCk_Handle InEntity)
        {
            InEntity.Add<TickableType>(T_Tickable{std::forward<T_Args>(InArgs)...});
        });

        DoSortTickable<TickableType>();

        return NewEntity;
    }

    template <typename T_Tickable>
    auto
        FTicker::
        DoSortTickable()
        -> void
    {
        _ProcessorsRegistry.Sort<T_Tickable>(
        [](const EntityType InA, const EntityType InB)
        {
            return InA < InB;
        });
    }

    // --------------------------------------------------------------------------------------------------------------------
}
