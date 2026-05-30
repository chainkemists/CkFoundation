#pragma once

#include "CkCamera/Camera/CkCamera_Fragment_Data.h"
#include "CkCamera/Camera/CkCameraProfile.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkCameraProfile_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_CameraProfile"))
class CKCAMERA_API UCk_Utils_CameraProfile_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CameraProfile_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_CameraProfile);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    // Creates a profile entity (owned by InOwner) holding a default FCk_CameraProfile. The director creates exactly
    // one and stores its handle on FFragment_Camera_Current; it lives for the director's lifetime.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][CameraProfile] Add")
    static FCk_Handle_CameraProfile
    Add(
        UPARAM(ref) FCk_Handle& InOwner);

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|CameraProfile",
        DisplayName = "[Ck][CameraProfile] Has")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

public:
    // The current composed profile carried by this entity (copy).
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CameraProfile",
              DisplayName = "[Ck][CameraProfile] Get Profile")
    static FCk_CameraProfile
    Get_Profile(
        const FCk_Handle_CameraProfile& InProfile);

    // Replaces the entity's whole profile.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CameraProfile",
              DisplayName = "[Ck][CameraProfile] Set Profile")
    static FCk_Handle_CameraProfile
    Set_Profile(
        UPARAM(ref) FCk_Handle_CameraProfile& InProfile,
        const FCk_CameraProfile& InNewProfile);

    // Resets the entity's profile to a fresh default. Called once per frame by ComposeProfile before modifiers
    // contribute on top.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CameraProfile",
              DisplayName = "[Ck][CameraProfile] Reset")
    static FCk_Handle_CameraProfile
    Reset(
        UPARAM(ref) FCk_Handle_CameraProfile& InProfile);

public:
    // Blends InTarget into the profile entity by InAlpha [0,1]. Continuous parameters (FOV, aspect, boom length,
    // pivot/framing offsets, DoF) are linearly interpolated; discrete feature blocks (orientation control,
    // auto-reorient, collision, noise, post-process toggle) are adopted from the target once it dominates the
    // blend (alpha >= 0.5). The canonical helper a modifier's DoContributeToProfile calls to layer a
    // designer-authored profile onto the running composite.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CameraProfile",
              DisplayName = "[Ck][CameraProfile] Blend Profile Into")
    static FCk_Handle_CameraProfile
    BlendInto(
        UPARAM(ref) FCk_Handle_CameraProfile& InOutProfile,
        const FCk_CameraProfile& InTarget,
        float InAlpha);

    // Struct-level blend core (world-free, pure). Shared by the handle overload above and by unit tests.
    static auto
    BlendInto(
        FCk_CameraProfile& InOutProfile,
        const FCk_CameraProfile& InTarget,
        float InAlpha) -> void;

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|CameraProfile",
        DisplayName = "[Ck][CameraProfile] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_CameraProfile
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|CameraProfile",
        DisplayName = "[Ck][CameraProfile] Handle -> CameraProfile Handle",
        meta = (CompactNodeTitle = "<AsCameraProfile>", BlueprintAutocast))
    static FCk_Handle_CameraProfile
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid CameraProfile Handle",
        Category = "Ck|Utils|CameraProfile",
        meta = (CompactNodeTitle = "INVALID_CameraProfileHandle", Keywords = "make"))
    static FCk_Handle_CameraProfile
    Get_InvalidHandle() { return {}; };
};

// --------------------------------------------------------------------------------------------------------------------
