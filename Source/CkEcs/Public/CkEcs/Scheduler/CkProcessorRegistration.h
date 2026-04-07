#pragma once

#include "CkProcessorRegistry.h"
#include "CkProcessorTraits.inl.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    template <typename T_Processor>
    struct FAutoProcessorRegistrar
    {
        FAutoProcessorRegistrar()
        {
            _RegisteredName = Get_ProcessorCanonicalName<T_Processor>();

            FProcessorRegistry::Get().Register(
                BuildDescriptor<T_Processor>(
                    [](const FCk_Registry& InRegistry) -> concepts::FTickableType
                    {
                        return T_Processor{InRegistry};
                    }));
        }

        explicit FAutoProcessorRegistrar(FProcessorFactory InFactory)
        {
            _RegisteredName = Get_ProcessorCanonicalName<T_Processor>();

            FProcessorRegistry::Get().Register(
                BuildDescriptor<T_Processor>(MoveTemp(InFactory)));
        }

        ~FAutoProcessorRegistrar()
        {
            if (IsEngineExitRequested())
            { return; }

            FProcessorRegistry::Get().Deregister(_RegisteredName);
        }

        FAutoProcessorRegistrar(const FAutoProcessorRegistrar&) = delete;
        auto operator=(const FAutoProcessorRegistrar&) -> FAutoProcessorRegistrar& = delete;
        FAutoProcessorRegistrar(FAutoProcessorRegistrar&&) = delete;
        auto operator=(FAutoProcessorRegistrar&&) -> FAutoProcessorRegistrar& = delete;

    private:
        FName _RegisteredName;
    };
    template <typename T_Group>
    struct FAutoGroupRegistrar
    {
        FAutoGroupRegistrar()
        {
            _RegisteredName = Get_ProcessorCanonicalName<T_Group>();

            FProcessorRegistry::Get().Register(
                BuildGroupDescriptor<T_Group>());
        }

        ~FAutoGroupRegistrar()
        {
            if (IsEngineExitRequested())
            { return; }

            FProcessorRegistry::Get().Deregister(_RegisteredName);
        }

        FAutoGroupRegistrar(const FAutoGroupRegistrar&) = delete;
        auto operator=(const FAutoGroupRegistrar&) -> FAutoGroupRegistrar& = delete;
        FAutoGroupRegistrar(FAutoGroupRegistrar&&) = delete;
        auto operator=(FAutoGroupRegistrar&&) -> FAutoGroupRegistrar& = delete;

    private:
        FName _RegisteredName;
    };
}

// --------------------------------------------------------------------------------------------------------------------

#define CK_DETAIL_CONCAT_INNER(a, b) a ## b
#define CK_DETAIL_CONCAT(a, b) CK_DETAIL_CONCAT_INNER(a, b)

#define CK_REGISTER_GROUP(GroupType) \
    static ::ck::FAutoGroupRegistrar<GroupType> \
        CK_DETAIL_CONCAT(GCkAutoRegisterGroup_, __COUNTER__){}

#define CK_REGISTER_PROCESSOR(ProcessorType) \
    static ::ck::FAutoProcessorRegistrar<ProcessorType> \
        CK_DETAIL_CONCAT(GCkAutoRegister_, __COUNTER__){}

#define CK_REGISTER_PROCESSOR_WITH_FACTORY(ProcessorType, ...) \
    static ::ck::FAutoProcessorRegistrar<ProcessorType> \
        CK_DETAIL_CONCAT(GCkAutoRegister_, __COUNTER__){__VA_ARGS__}

// --------------------------------------------------------------------------------------------------------------------
