#include "CkResolverDataBundle_ProcessorInjector.h"

#include "ResolverDataBundle/CkResolverDataBundle_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ResolverDataBundle_ProcessorInjector_StartNewPhase_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ResolverDataBundle_StartNewPhase>(InWorld.Get_Registry());
}

auto
    UCk_ResolverDataBundle_ProcessorInjector_HandleRequests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_ResolverDataBundle_HandleRequests>(InWorld.Get_Registry());
}

auto
    UCk_ResolverDataBundle_ProcessorInjector_ResolveOperations_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ResolverDataBundle_ResolveOperations>(InWorld.Get_Registry());
}

auto
    UCk_ResolverDataBundle_ProcessorInjector_Calculate_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_ResolverDataBundle_Calculate>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
