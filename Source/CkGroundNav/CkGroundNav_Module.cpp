#include "CkGroundNav_Module.h"

#include "CkGroundNav/Facade/CkGroundNav_NavSurfaceAdapter.h"

#define LOCTEXT_NAMESPACE "FCkGroundNavModule"

void FCkGroundNavModule::StartupModule()
{
    // At module startup rather than lazily: CkNavigation's provider registry is read without a lock,
    // and that is only sound while every registration has landed before the first world exists.
    ck::groundnav::nav_surface_adapter::Register();
}

void FCkGroundNavModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkGroundNavModule, CkGroundNav)
