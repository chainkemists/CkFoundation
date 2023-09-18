#pragma once

#include "CkOverlapBody_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_OverlapBody_UserSettings_UE::
    UCk_OverlapBody_UserSettings_UE(
        const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    CategoryName = FName{TEXT("ChainKemists")};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_OverlapBody_UserSettings_UE::
    Get_DebugPreviewAllSensors()
    -> bool
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_OverlapBody_UserSettings_UE>()->Get_DebugPreviewAllSensors();
}

auto
    UCk_Utils_OverlapBody_UserSettings_UE::
    Get_DebugPreviewAllMarkers()
    -> bool
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_OverlapBody_UserSettings_UE>()->Get_DebugPreviewAllMarkers();
}

// --------------------------------------------------------------------------------------------------------------------
