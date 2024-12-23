#include "CkIsmRenderer_ProcessorInjector.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_Processor.h"
#include "CkIsmRenderer/Proxy/CkIsmProxy_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_ProcessorInjector_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_IsmRenderer_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_IsmRenderer_ClearInstances>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_IsmProxy_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_IsmProxy_AddInstance>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_IsmRenderer_ProcessorInjector_Requests_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_IsmProxy_HandleRequests>(InWorld.Get_Registry());
}

auto
    UCk_IsmRenderer_ProcessorInjector_Teardown_UE::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_IsmProxy_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
