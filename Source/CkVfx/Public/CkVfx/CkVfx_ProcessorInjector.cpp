#include "CkVfx_ProcessorInjector.h"

#include "CkVfx/Cue/CkVfxCue_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VfxCue_ProcessorInjector_Setup_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_VfxCue_Setup>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VfxCue_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_VfxCue_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_VfxCue_EffectLifetimeMonitor>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_VfxCue_ProcessorInjector_EndPlay_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_VfxCue_EndPlay>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
