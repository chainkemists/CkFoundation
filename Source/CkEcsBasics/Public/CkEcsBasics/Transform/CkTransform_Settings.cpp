#pragma once

#include "CkTransform_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_Transform_UserSettings_UE::
    UCk_Transform_UserSettings_UE(
        const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    CategoryName = FName{TEXT("ChainKemists")};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transform_UserSettings_UE::
    Get_EnableTransformSmoothing()
    -> bool
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Transform_UserSettings_UE>()->Get_EnableTransformSmoothing();
}

// --------------------------------------------------------------------------------------------------------------------
