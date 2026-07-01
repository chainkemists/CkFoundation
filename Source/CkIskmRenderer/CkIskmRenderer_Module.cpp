#include "CkIskmRenderer_Module.h"

#define LOCTEXT_NAMESPACE "FCkIskmRendererModule"

void FCkIskmRendererModule::StartupModule()
{
    // Plan-2 shader-dir mapping + the batched vertex-factory type now live in the CkIskmRendererVF module
    // (PostConfigInit), so the VF registers before the engine seals its vertex-factory list.
}

void FCkIskmRendererModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkIskmRendererModule, CkIskmRenderer)
