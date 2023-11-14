#pragma once

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkProjectSettings_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_RendererSettings_CustomDepth : uint8
{
    Disabled,
    Enabled,
    EnabledOnDemand,
    EnabledWithStencil
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class CKSETTINGS_API UCk_Utils_ProjectSettings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|ProjectSettings")
    static ECk_RendererSettings_CustomDepth
    Get_RendererSettings_CustomDepthValue();
};

// --------------------------------------------------------------------------------------------------------------------
