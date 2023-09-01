#include "CkMarkerAndSensor_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MarkerAndSensor_UE::
    Draw_Marker_DebugLines(
        UObject* InOuter,
        const ck::FFragment_Marker_Current& InMarkerCurrent,
        const FCk_Fragment_Marker_ParamsData& InMarkerParams)
    -> void
{
    DoDraw_MarkerOrSensor_DebugLines(InOuter, InMarkerCurrent, InMarkerParams);
}

auto
    UCk_Utils_MarkerAndSensor_UE::
    Draw_Sensor_DebugLines(
        UObject* InOuter,
        const ck::FFragment_Sensor_Current& InSensorCurrent,
        const FCk_Fragment_Sensor_ParamsData& InSensorParams)
    -> void
{
    DoDraw_MarkerOrSensor_DebugLines(InOuter, InSensorCurrent, InSensorParams);
}

// --------------------------------------------------------------------------------------------------------------------
