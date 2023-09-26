#pragma once

#include <CoreMinimal.h>

#include "CkCore/Macros/CkMacros.h"
#include "CkSettings/Public/CkSettings/UserSettings/CkUserSettings.h"
#include "CkTransform_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Transform"))
class CKECSBASICS_API UCk_Transform_UserSettings_UE : public UCk_EditorPerProject_UserSettings_UE
{
    GENERATED_BODY()

private:
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General", meta = (AllowPrivateAccess = true))
    bool _EnableTransformSmoothing = true;

public:
    CK_PROPERTY_GET(_EnableTransformSmoothing);
};

// --------------------------------------------------------------------------------------------------------------------

class CKECSBASICS_API UCk_Utils_Transform_UserSettings_UE
{
public:
    static auto Get_EnableTransformSmoothing() -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
