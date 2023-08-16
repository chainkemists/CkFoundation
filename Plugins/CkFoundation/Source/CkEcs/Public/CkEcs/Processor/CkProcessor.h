#pragma once
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"


namespace ck
{
    template <typename T_DerivedProcessor, typename... T_Fragments>
    class TProcessor
    {
        CK_GENERATED_BODY(TProcessor<T_DerivedProcessor COMMA T_Fragments...>);

    public:
        using FEntitType = FCk_Entity;
        using FTimeType = FCk_Time;
        using FHandleType = FCk_Handle;
        using FRegistryType = FCk_Registry;
        using FDerivedType = T_DerivedProcessor;

    public:
        explicit TProcessor(FRegistryType InRegistry);

    public:
        auto Tick(FTimeType InDeltaT) -> void;

    private:
        template <typename... T_ComponentsOnly>
        auto DoTick(FTimeType InDeltaT, entt::type_list<T_ComponentsOnly...>) -> void;

    private:
        CK_ENABLE_SFINAE_THIS(FDerivedType);

    protected:
        FRegistryType _Registry;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_DerivedProcessor, typename ... T_Fragments>
    TProcessor<T_DerivedProcessor, T_Fragments...>::TProcessor(FRegistryType InRegistry)
        : _Registry(InRegistry)
    {
    }

    template <typename T_DerivedProcessor, typename ... T_Fragments>
    auto TProcessor<T_DerivedProcessor, T_Fragments...>::Tick(FTimeType InDeltaT) -> void
    {
        using FViewType = decltype(_Registry.View<T_Fragments...>());
        using FComponentsOnly = typename FViewType::template TFragmentsOnly<T_Fragments...>;

        DoTick(InDeltaT, FComponentsOnly{});
    }

    template <typename T_DerivedProcessor, typename ... T_Fragments>
    template <typename ... T_ComponentsOnly>
    auto TProcessor<T_DerivedProcessor, T_Fragments...>::DoTick(FTimeType InDeltaT,
        entt::type_list<T_ComponentsOnly...>) -> void
    {
        _Registry.View<T_Fragments...>().Each([&](FEntitType InEntity, T_ComponentsOnly&... InComponents)
        {
            auto Handle = FHandleType{InEntity, _Registry};
            This()->ForEachEntity(InDeltaT, Handle, InComponents...);
        });
    }

    // --------------------------------------------------------------------------------------------------------------------
}

