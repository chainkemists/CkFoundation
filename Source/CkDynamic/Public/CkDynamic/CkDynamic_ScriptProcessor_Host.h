#pragma once

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Bridge that discovers every subclass of UCk_Processor_Script_Base_UE at world-begin-play time and injects a
// corresponding FProcessorDescriptor into CkEcs's FProcessorRegistry so the scheduler can drive them.
//
// Owned by the CkDynamic module; its StartupModule/ShutdownModule wires subscribe/unsubscribe.
// Lives in CkDynamic (not CkEcs) because descriptor construction needs CkDynamic's UScriptStruct → storage
// bridge for the MarkedDirtyBy gate, and CkEcs must not depend on CkDynamic (cycle).
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
