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


// --------------------------------------------------------------------------------------------------------------------
