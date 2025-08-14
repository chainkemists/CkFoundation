#include "CkAudio_ProcessorInjector.h"

#include "CkAudio/AudioDirector/CkAudioDirector_Processor.h"
#include "CkAudio/AudioTrack/CkAudioTrack_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioTrack_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_AudioTrack_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioTrack_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioTrack_SpatialUpdate>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioTrack_Playback>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioTrack_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioDirector_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_AudioDirector_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioDirector_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioDirector_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
