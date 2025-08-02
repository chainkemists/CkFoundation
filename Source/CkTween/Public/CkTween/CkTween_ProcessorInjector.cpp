#include "CkTween_ProcessorInjector.h"

#include "CkTween/CkTween_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Tween_ProcessorInjector_Setup::DoInjectProcessors(EcsWorldType& InWorld) -> void
{
    InWorld.Add<ck::FProcessor_Tween_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Tween_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Tween_ProcessorInjector_Update::DoInjectProcessors(EcsWorldType& InWorld) -> void
{
    InWorld.Add<ck::FProcessor_Tween_HandleDelays>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Tween_Update>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Tween_ApplyToTransform>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Tween_ProcessorInjector_Teardown::DoInjectProcessors(EcsWorldType& InWorld) -> void
{
    InWorld.Add<ck::FProcessor_Tween_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------