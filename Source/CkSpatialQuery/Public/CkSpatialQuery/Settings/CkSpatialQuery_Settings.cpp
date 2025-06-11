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

// --------------------------------------------------------------------------------------------------------------------
