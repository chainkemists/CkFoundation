#include "CkEntityVisualizer_Module.h"

#define LOCTEXT_NAMESPACE "FCkEntityVisualizerModule"

auto
    FCkEntityVisualizerModule::
    StartupModule()
    -> void
{
}

auto
    FCkEntityVisualizerModule::
    ShutdownModule()
    -> void
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FCkEntityVisualizerModule, CkEntityVisualizer)
