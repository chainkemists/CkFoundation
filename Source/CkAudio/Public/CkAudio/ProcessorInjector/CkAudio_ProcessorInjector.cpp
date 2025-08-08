#include "CkAudio_ProcessorInjector.h"

#include "CkAudio/CkAudio_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Audio_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_Audio_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Audio_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Audio_Teardown>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Audio_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------