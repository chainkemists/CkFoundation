#include "CkInput_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Input_Settings_UE::
    Get_MappingContextScanPaths()
    -> const TArray<FDirectoryPath>&
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Input_ProjectSettings_UE>()->Get_MappingContextScanPaths();
}

// --------------------------------------------------------------------------------------------------------------------
