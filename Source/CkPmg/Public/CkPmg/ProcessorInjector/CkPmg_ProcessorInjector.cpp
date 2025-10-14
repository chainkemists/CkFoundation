#include "CkPmg_ProcessorInjector.h"

#include "CkPmg/CkPmg_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Pmg_ProcessorInjector_Setup_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_Pmg_Donut_Setup>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Pmg_ProcessorInjector_HandleRequests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Pmg_Donut_HandleRequests>(InWorld.Get_Registry());
}

auto
    UCk_Pmg_ProcessorInjector_Transform_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld) 
    -> void
{
    InWorld.Add<ck::FProcessor_Pmg_Donut_UpdateTransform>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Pmg_ProcessorInjector_EndPlay_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Pmg_Donut_EndPlay>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

