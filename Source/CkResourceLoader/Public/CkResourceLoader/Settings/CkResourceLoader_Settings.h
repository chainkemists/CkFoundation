#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkResourceLoader_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Resource Loader"))
class CKRESOURCELOADER_API UCk_ResourceLoader_ProjectSettings_UE : public UCk_Game_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ResourceLoader_ProjectSettings_UE);

private:
    UPROPERTY(Config, VisibleDefaultsOnly, BlueprintReadOnly, Category = "Caching",
              meta = (AllowPrivateAccess = true, UIMin = 100, ClampMin = 100))
    int32 _MaxNumberOfCachedResourcesPerType = 100;

public:
    CK_PROPERTY_GET(_MaxNumberOfCachedResourcesPerType);
};

// --------------------------------------------------------------------------------------------------------------------

class CKRESOURCELOADER_API UCk_Utils_ResourceLoader_ProjectSettings_UE
{
public:
    static auto Get_MaxNumberOfCachedResourcesPerType() -> int32;
};

// --------------------------------------------------------------------------------------------------------------------
