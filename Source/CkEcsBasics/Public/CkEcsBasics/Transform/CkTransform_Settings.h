#pragma once

#include <CoreMinimal.h>
#include <Engine/DeveloperSettings.h>

#include "CkCore/Macros/CkMacros.h"
#include "CkTransform_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(config = EditorPerProjectUserSettings, meta = (DisplayName = "Transform"))
class CKECSBASICS_API UCk_Transform_UserSettings_UE : public UDeveloperSettings
{
    GENERATED_BODY()

public:
    explicit UCk_Transform_UserSettings_UE(const FObjectInitializer& ObjectInitializer);

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
