#include "CkGeometryCollectionOwner_ProcessorInjector.h"

#include "CkChaos/GeometryCollectionOwner/CkGeometryCollectionOwner_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GeometryCollectionOwner_ProcessorInjector_Setup_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_GeometryCollectionOwner_Setup>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GeometryCollectionOwner_ProcessorInjector_Replicate_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_GeometryCollectionOwner_Replicate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GeometryCollectionOwner_ProcessorInjector_Requests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_GeometryCollectionOwner_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
