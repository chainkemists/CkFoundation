#include "CkResourceLoaderProcessorInjector.h"

#include "CkResourceLoader/CkResourceLoader_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_ResourceLoader_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_ResourceLoader_HandleRequests>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
