#include "CkRenderProxy_ProcessorInjector.h"

#include "CkRenderProxy/Proxy/CkRenderProxy_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RenderProxy_ProcessorInjector_Setup::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_RenderProxy_Setup>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RenderProxy_ProcessorInjector_Requests::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_RenderProxy_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RenderProxy_ProcessorInjector_Transform::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
#if CK_BUILD_TEST
    InWorld.Add<ck::FProcessor_RenderProxy_EnsureStaticNotMoved_DEBUG>(InWorld.Get_Registry());
#endif

    InWorld.Add<ck::FProcessor_RenderProxy_UpdateTransform>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_RenderProxy_ProcessorInjector_Teardown::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_RenderProxy_EndPlay>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------