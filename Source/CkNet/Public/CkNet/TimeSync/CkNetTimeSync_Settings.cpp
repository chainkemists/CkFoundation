#pragma once

#include "CkNetTimeSync_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NetTimeSync_Settings_UE::
    Get_EnableNetTimeSynchronization()
    -> bool
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_NetTimeSync_ProjectSettings_UE>()->Get_EnableNetTimeSynchronization();
}

// --------------------------------------------------------------------------------------------------------------------
