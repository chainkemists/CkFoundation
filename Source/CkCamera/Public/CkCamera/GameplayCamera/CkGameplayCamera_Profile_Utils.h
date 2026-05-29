#pragma once

#include "CkCamera/GameplayCamera/CkGameplayCamera_Profile.h"

#include "CkCore/Macros/CkMacros.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkGameplayCamera_Profile_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCAMERA_API UCk_Utils_GameplayCamera_Profile_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_GameplayCamera_Profile_UE);

public:
    // Blends InTarget into InOutProfile by InAlpha [0,1]. Continuous parameters (FOV, aspect, boom length,
    // pivot/framing offsets, DoF) are linearly interpolated; discrete feature blocks (orientation control,
    // auto-reorient, collision, noise, post-process toggle) are adopted from the target once it dominates the
    // blend (alpha >= 0.5). The canonical helper a modifier's DoContributeToProfile calls to layer a
    // designer-authored profile onto the running composite.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|GameplayCamera|Profile",
              DisplayName = "[Ck][GameplayCamera] Blend Profile Into")
    static void
    BlendInto(
        UPARAM(ref) FCk_GameplayCamera_Profile& InOutProfile,
        const FCk_GameplayCamera_Profile& InTarget,
        float InAlpha);
};

// --------------------------------------------------------------------------------------------------------------------
