#pragma once

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Discovers every UCk_Processor_Script_Base_UE subclass at world-begin-play and injects a matching
// FProcessorDescriptor into CkEcs's FProcessorRegistry.
// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKDYNAMIC_API FScriptProcessor_Host
    {
    public:
        static auto Startup() -> void;
        static auto Shutdown() -> void;
    };
}
