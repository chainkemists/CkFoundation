#pragma once

#include <CoreMinimal.h>

#include "CkCore/Macros/CkMacros.h"
#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkEcsBasics_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "ECS Basics"))
class CKECSBASICS_API UCk_EcsBasics_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

private:
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Transform", meta = (AllowPrivateAccess = true))
    bool _EnableTransformSmoothing = true;

public:
    CK_PROPERTY_GET(_EnableTransformSmoothing);
};

// --------------------------------------------------------------------------------------------------------------------

class CKECSBASICS_API UCk_Utils_EcsBasics_Settings_UE
{
public:
    static auto Get_EnableTransformSmoothing() -> bool;
};

// --------------------------------------------------------------------------------------------------------------------
