#pragma once

#include "CkTransform_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Transform_Settings_UE::
    Get_EnableTransformSmoothing()
    -> bool
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Transform_ProjectSettings_UE>()->Get_EnableTransformSmoothing();
}

// --------------------------------------------------------------------------------------------------------------------
