#pragma once

#include "CkSpatialQuery_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_spatial_query_settings
{
    namespace cvar
    {
        static bool PreviewAllProbes = false;
        static FAutoConsoleVariableRef CVar_PreviewAllProbes(TEXT("ck.SpatialQuery.PreviewAllProbes"),
            PreviewAllProbes,
            TEXT("Draw the debug information of all existing Probe Fragments"));

        static bool PreviewAllLineTraces = false;
        static FAutoConsoleVariableRef CVar_PreviewAllLineTraces(TEXT("ck.SpatialQuery.PreviewAllLineTraces"),
            PreviewAllLineTraces,
            TEXT("Draw the debug information of all existing Probe Traces"));

        static bool PreviewAllProbesUsingJolt = false;
        static FAutoConsoleVariableRef CVar_PreviewAllProbesUsingJolt(TEXT("ck.SpatialQuery.PreviewAllProbesUsingJolt"),
            PreviewAllProbesUsingJolt,
            TEXT("Draw the debug information of all existing Probe Fragments using Jolt's debugger"));

        static float ProbeLineTraceDebugDuration = 0.0f;
        static FAutoConsoleVariableRef CVar_ProbeLineTraceDebugDuration(TEXT("ck.SpatialQuery.ProbeLineTraceDebugDuration"),
            ProbeLineTraceDebugDuration,
            TEXT("Duration in seconds for line trace debug display (0.0 = single frame)"));

        static bool DebugPreviewServerLineTraces = true;
        static FAutoConsoleVariableRef CVar_DebugPreviewServerLineTraces(TEXT("ck.SpatialQuery.DebugPreviewServerLineTraces"),
            DebugPreviewServerLineTraces,
            TEXT("Enable debug preview for server line traces"));

        static bool DebugPreviewClientLineTraces = true;
        static FAutoConsoleVariableRef CVar_DebugPreviewClientLineTraces(TEXT("ck.SpatialQuery.DebugPreviewClientLineTraces"),
            DebugPreviewClientLineTraces,
            TEXT("Enable debug preview for client line traces"));

        static float ProbeLineTraceDebugThickness = 0.5f;
        static FAutoConsoleVariableRef CVar_ProbeLineTraceDebugThickness(TEXT("ck.SpatialQuery.ProbeLineTraceDebugThickness"),
            ProbeLineTraceDebugThickness,
            TEXT("Line thickness for probe line trace debug drawing"));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SpatialQuery_Settings::
    Get_DebugPreviewAllProbes()
    -> bool
{
    return ck_spatial_query_settings::cvar::PreviewAllProbes;
}

auto
    UCk_Utils_SpatialQuery_Settings::
    Get_DebugPreviewAllLineTraces()
    -> bool
{
    return ck_spatial_query_settings::cvar::PreviewAllLineTraces;
}

auto
    UCk_Utils_SpatialQuery_Settings::
    Get_DebugPreviewAllProbesUsingJolt()
    -> bool
{
    return ck_spatial_query_settings::cvar::PreviewAllProbesUsingJolt;
}

auto
    UCk_Utils_SpatialQuery_Settings::
    Get_ProbeLineTraceDebugDuration()
    -> float
{
    return ck_spatial_query_settings::cvar::ProbeLineTraceDebugDuration;
}

auto
    UCk_Utils_SpatialQuery_Settings::
    Get_DebugPreviewServerLineTraces()
    -> bool
{
    return ck_spatial_query_settings::cvar::DebugPreviewServerLineTraces;
}

auto
    UCk_Utils_SpatialQuery_Settings::
    Get_DebugPreviewClientLineTraces()
    -> bool
{
    return ck_spatial_query_settings::cvar::DebugPreviewClientLineTraces;
}

auto
    UCk_Utils_SpatialQuery_Settings::
    Get_ProbeLineTraceDebugThickness()
    -> float
{
    return ck_spatial_query_settings::cvar::ProbeLineTraceDebugThickness;
}

// --------------------------------------------------------------------------------------------------------------------
