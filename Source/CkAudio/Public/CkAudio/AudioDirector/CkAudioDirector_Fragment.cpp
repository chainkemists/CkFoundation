#include "CkAudioDirector_Fragment.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::FFragment_AudioDirector_Current::
    Request_CreateTrackEntity()
    -> FCk_Handle
{
    return UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_TrackRegistry);
}

auto
    ck::FFragment_AudioDirector_Current::
    Request_CreateTimerEntity()
    -> FCk_Handle
{
    return UCk_Utils_EntityLifetime_UE::Request_CreateEntity(_TimerRegistry);
}

// --------------------------------------------------------------------------------------------------------------------
