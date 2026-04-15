#include "CkDynamic_Module.h"

#include "CkDynamic/CkDynamic_ScriptProcessor_Host.h"

#define LOCTEXT_NAMESPACE "FCkDynamicModule"

void FCkDynamicModule::StartupModule()
{
    ck::FScriptProcessor_Host::Startup();
}

void FCkDynamicModule::ShutdownModule()
{
    ck::FScriptProcessor_Host::Shutdown();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkDynamicModule, CkDynamic)
