#pragma once

#include "CkNetTimeSync_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NetTimeSync_UserSettings_UE::
    Get_EnableNetTimeSynchronization()
    -> bool
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_NetTimeSync_UserSettings_UE>()->Get_EnableNetTimeSynchronization();
}

// --------------------------------------------------------------------------------------------------------------------
