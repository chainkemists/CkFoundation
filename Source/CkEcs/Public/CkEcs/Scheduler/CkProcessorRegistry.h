#pragma once

#include "CkProcessorDescriptor.h"

#include "CkCore/Macros/CkMacros.h"

#include <HAL/CriticalSection.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKECS_API FProcessorRegistry
    {
    public:
        CK_GENERATED_BODY(FProcessorRegistry);

    public:
        static auto Get() -> FProcessorRegistry&;

        auto Register(FProcessorDescriptor&& InDescriptor) -> void;
        auto Deregister(FName InProcessorName) -> void;
        auto DeregisterAllFromModule(FName InModuleName) -> void;

        auto Get_AllDescriptors() const -> const TArray<FProcessorDescriptor>&;

    private:
        TArray<FProcessorDescriptor> _Descriptors;
        FCriticalSection _Mutex;
    };
}

// --------------------------------------------------------------------------------------------------------------------
