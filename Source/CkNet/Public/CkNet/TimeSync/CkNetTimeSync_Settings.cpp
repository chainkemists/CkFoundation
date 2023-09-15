#pragma once

#include "CkNetTimeSync_Settings.h"

#include "CkObject/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_NetTimeSync_UserSettings_UE::
    UCk_NetTimeSync_UserSettings_UE(
        const FObjectInitializer& InObjectInitializer)
    : Super(InObjectInitializer)
{
    CategoryName = FName{TEXT("ChainKemists")};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_NetTimeSync_UserSettings_UE::
    Get_EnableNetTimeSynchronization()
    -> bool
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_NetTimeSync_UserSettings_UE>()->Get_EnableNetTimeSynchronization();
}

// --------------------------------------------------------------------------------------------------------------------
