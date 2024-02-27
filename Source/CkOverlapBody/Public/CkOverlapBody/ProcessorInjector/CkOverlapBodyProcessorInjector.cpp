#include "CkOverlapBodyProcessorInjector.h"

#include "CkOverlapBody/Marker/CkMarker_Processor.h"
#include "CkOverlapBody/Sensor/CkSensor_Processor.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_OverlapBody_ProcessorInjector::
    DoInjectProcessors(
        EcsWorldType& InWorld)
        -> void
{
    InWorld.Add<ck::FProcessor_Marker_Setup>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Sensor_Setup>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Marker_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Marker_Teardown>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Sensor_HandleRequests>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Sensor_Teardown>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_OverlapBody_ProcessorInjector_UpdateTransformAndDebug::
    DoInjectProcessors(
        EcsWorldType& InWorld)
    -> void
{
    InWorld.Add<ck::FProcessor_Marker_UpdateTransform>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Sensor_UpdateTransform>(InWorld.Get_Registry());

    InWorld.Add<ck::FProcessor_Marker_DebugPreviewAll>(InWorld.Get_Registry());
    InWorld.Add<ck::FProcessor_Sensor_DebugPreviewAll>(InWorld.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------
