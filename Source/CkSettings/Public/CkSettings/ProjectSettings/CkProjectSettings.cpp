#include "CkProjectSettings.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_Engine_ProjectSettings_UE::
    UCk_Engine_ProjectSettings_UE(
        const FObjectInitializer& ObjectInitializer)
{
    CategoryName = FName{TEXT("CkFoundation")};
}

// --------------------------------------------------------------------------------------------------------------------

UCk_Editor_ProjectSettings_UE::
    UCk_Editor_ProjectSettings_UE(
        const FObjectInitializer& ObjectInitializer)
{
    CategoryName = FName{TEXT("CkFoundation")};
}

// --------------------------------------------------------------------------------------------------------------------

UCk_Game_ProjectSettings_UE::
    UCk_Game_ProjectSettings_UE(
        const FObjectInitializer& ObjectInitializer)
{
    CategoryName = FName{TEXT("CkFoundation")};
}

// --------------------------------------------------------------------------------------------------------------------
