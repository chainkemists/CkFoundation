#pragma once

#include "CkCamera/Camera/CkCameraProfile.h"

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCameraProfile_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCAMERA_API UCk_Utils_CameraProfile_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CameraProfile_UE);

public:
    // Blends InTarget into InOutProfile by InAlpha [0,1]. Continuous parameters (FOV, aspect, boom length,
    // pivot/framing offsets, DoF) are linearly interpolated; discrete feature blocks (orientation control,
    // auto-reorient, collision, noise, post-process toggle) are adopted from the target once it dominates the
    // blend (alpha >= 0.5). The canonical helper a modifier's DoContributeToProfile calls to layer a
    // designer-authored profile onto the running composite.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Camera|Profile",
              DisplayName = "[Ck][Camera] Blend Profile Into")
    static void
    BlendInto(
        UPARAM(ref) FCk_CameraProfile& InOutProfile,
        const FCk_CameraProfile& InTarget,
        float InAlpha);
};

// --------------------------------------------------------------------------------------------------------------------
