#include "CkCamera_ProcessorInjector.h"

#include "CkCamera/CameraShake/CkCameraShake_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Camera_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_CameraShake_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
