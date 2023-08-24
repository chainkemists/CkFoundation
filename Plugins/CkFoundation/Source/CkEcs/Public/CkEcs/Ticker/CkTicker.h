#pragma once

#include "CkCore/Concepts/CkConcepts.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"

#include "CkMacros/CkMacros.h"

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    class CKECS_API FTicker
    {
        CK_GENERATED_BODY(FTicker);

    public:
        using FRegistryType = FCk_Registry;
        using FTimeType = FCk_Time;
        using FHandleType = FCk_Handle;
        using FTickableType = concept::FTickableType;
        using FEntityType = FCk_Entity;

    public:
        template <typename T_Tickable>
        auto Add(T_Tickable&& InTickable) -> FHandleType;

        template <typename T_Tickable, typename... T_Args>
        auto Add(T_Args&&... InArgs) -> FHandleType;

    public:
        auto Tick(FTimeType InDeltaTime) -> void;

    private:
        template <typename T_Tickable>
        auto DoSortTickable() -> void;

    private:
        FRegistryType _ProcessorsRegistry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_Tickable>
    auto FTicker::Add(T_Tickable&& InTickable) -> FHandleType
    {
        auto Handle = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_ProcessorsRegistry);

        Handle.Add<FTickableType>(std::forward<T_Tickable>(InTickable));
        DoSortTickable<T_Tickable>();

        return Handle;
    }

    template <typename T_Tickable, typename ... T_Args>
    auto FTicker::Add(T_Args&&... InArgs) -> FHandleType
    {
        auto Handle = UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_ProcessorsRegistry);

        Handle.Add<FTickableType>(T_Tickable{std::forward<T_Args>(InArgs)...});
        DoSortTickable<FTickableType>();

        return Handle;
    }

    template <typename T_Tickable>
    auto FTicker::
    DoSortTickable() -> void
    {
        _ProcessorsRegistry.Sort<T_Tickable>(
        [](const FEntityType InA, const FEntityType InB)
        {
            return InA < InB;
        });
    }

    // --------------------------------------------------------------------------------------------------------------------
}
