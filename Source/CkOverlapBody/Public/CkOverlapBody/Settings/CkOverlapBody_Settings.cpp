#pragma once

#include "CkOverlapBody_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_overlap_body_settings
{
    namespace cvar
    {
        static bool ShouldPreviewAllSensors = false;
        static FAutoConsoleVariableRef CVar_ShouldPreviewAllSensors(TEXT("ck.OverlapBody.ShouldPreviewAllSensors"),
            ShouldPreviewAllSensors,
            TEXT("Should draw the debug information of all existing Sensor Fragments"));

        static bool ShouldPreviewAllMarkers = false;
        static FAutoConsoleVariableRef CVar_ShouldPreviewAllMarkers(TEXT("ck.OverlapBody.ShouldPreviewAllMarkers"),
            ShouldPreviewAllMarkers,
            TEXT("Should draw the debug information of all existing Marker Fragments"));
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_OverlapBody_Settings_UE::
    Get_DebugPreviewAllSensors()
    -> bool
{
    return ck_overlap_body_settings::cvar::ShouldPreviewAllSensors;
}

auto
    UCk_Utils_OverlapBody_Settings_UE::
    Get_DebugPreviewAllMarkers()
    -> bool
{
    return ck_overlap_body_settings::cvar::ShouldPreviewAllMarkers;
}

// --------------------------------------------------------------------------------------------------------------------
