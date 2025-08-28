#include "CkAudio_ProcessorInjector.h"

#include "CkAudio/AudioDirector/CkAudioDirector_Processor.h"
#include "CkAudio/AudioTrack/CkAudioTrack_Processor.h"
#include "CkAudio/Cue/CkAudioCue_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

void
    UCk_AudioTrack_ProcessorInjector_Setup_UE::DoInjectProcessors(
        EcsWorldType& InWorld)
{
    InWorld.Add<ck::FProcessor_AudioDirector_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioTrack_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioCue_Setup>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioTrack_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_AudioTrack_SpatialUpdate>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioTrack_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioCue_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioTrack_Playback>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_AudioTrack_DebugDraw_Individual_Spatial>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioTrack_DebugDraw_Individual_NonSpatial>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_AudioTrack_DebugDraw_All_Spatial>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioTrack_DebugDraw_All_NonSpatial>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_AudioTrack_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_AudioDirector_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_AudioDirector_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioDirector_Teardown>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_AudioCue_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
