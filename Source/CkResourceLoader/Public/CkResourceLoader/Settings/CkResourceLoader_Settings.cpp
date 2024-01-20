#include "CkResourceLoader_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ResourceLoader_Settings_UE::
    Get_MaxNumberOfCachedResourcesPerType()
    -> int32
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_ResourceLoader_ProjectSettings_UE>()->Get_MaxNumberOfCachedResourcesPerType();
}

// --------------------------------------------------------------------------------------------------------------------
