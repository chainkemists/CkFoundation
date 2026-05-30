#pragma once

#include "CkCamera/Camera/CkCamera_Fragment_Data.h"
#include "CkCamera/Camera/Profile/CkCameraProfile.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkCameraProfile_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck { class FProcessor_Camera_ComposeProfile; }

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
    // The director owns the profile entity and is the only thing allowed to create it (via Create, below).
    friend class UCk_Utils_Camera_UE;
    // The compose processor resets the profile each frame.
    friend class ck::FProcessor_Camera_ComposeProfile;

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|CameraProfile",
        DisplayName = "[Ck][CameraProfile] Has")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    // Assembles the full profile from this entity's per-section fragments (copy).
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CameraProfile",
              DisplayName = "[Ck][CameraProfile] Get Profile")
    static FCk_CameraProfile
    Get_Profile(
        const FCk_Handle_CameraProfile& InProfile);

    // ---- Per-section accessors (the Trim contribution surface; each maps to one section fragment) ----

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Get Rig")
    static FCk_CameraProfile_Rig
    Get_Rig(const FCk_Handle_CameraProfile& InProfile);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Set Rig")
    static FCk_Handle_CameraProfile
    Set_Rig(UPARAM(ref) FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_Rig& InRig);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Get Springs")
    static FCk_CameraProfile_Springs
    Get_Springs(const FCk_Handle_CameraProfile& InProfile);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Set Springs")
    static FCk_Handle_CameraProfile
    Set_Springs(UPARAM(ref) FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_Springs& InSprings);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Get Sensor")
    static FCk_CameraProfile_Sensor
    Get_Sensor(const FCk_Handle_CameraProfile& InProfile);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Set Sensor")
    static FCk_Handle_CameraProfile
    Set_Sensor(UPARAM(ref) FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_Sensor& InSensor);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Get Noise")
    static FCk_CameraProfile_Noise
    Get_Noise(const FCk_Handle_CameraProfile& InProfile);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Set Noise")
    static FCk_Handle_CameraProfile
    Set_Noise(UPARAM(ref) FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_Noise& InNoise);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Get Orientation Control")
    static FCk_CameraProfile_OrientationControl
    Get_OrientationControl(const FCk_Handle_CameraProfile& InProfile);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Set Orientation Control")
    static FCk_Handle_CameraProfile
    Set_OrientationControl(UPARAM(ref) FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_OrientationControl& InOrientationControl);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Get Has Orientation Control")
    static bool
    Get_HasOrientationControl(const FCk_Handle_CameraProfile& InProfile);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Set Has Orientation Control")
    static FCk_Handle_CameraProfile
    Set_HasOrientationControl(UPARAM(ref) FCk_Handle_CameraProfile& InProfile, bool InEnabled);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Get Auto Reorient")
    static FCk_CameraProfile_AutoReorient
    Get_AutoReorient(const FCk_Handle_CameraProfile& InProfile);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Set Auto Reorient")
    static FCk_Handle_CameraProfile
    Set_AutoReorient(UPARAM(ref) FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_AutoReorient& InAutoReorient);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Get Has Auto Reorient")
    static bool
    Get_HasAutoReorient(const FCk_Handle_CameraProfile& InProfile);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Set Has Auto Reorient")
    static FCk_Handle_CameraProfile
    Set_HasAutoReorient(UPARAM(ref) FCk_Handle_CameraProfile& InProfile, bool InEnabled);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Get Collision")
    static FCk_CameraProfile_Collision
    Get_Collision(const FCk_Handle_CameraProfile& InProfile);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Set Collision")
    static FCk_Handle_CameraProfile
    Set_Collision(UPARAM(ref) FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_Collision& InCollision);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Get Has Collision")
    static bool
    Get_HasCollision(const FCk_Handle_CameraProfile& InProfile);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Set Has Collision")
    static FCk_Handle_CameraProfile
    Set_HasCollision(UPARAM(ref) FCk_Handle_CameraProfile& InProfile, bool InEnabled);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Get Depth Of Field")
    static FCk_CameraProfile_DepthOfField
    Get_DepthOfField(const FCk_Handle_CameraProfile& InProfile);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Set Depth Of Field")
    static FCk_Handle_CameraProfile
    Set_DepthOfField(UPARAM(ref) FCk_Handle_CameraProfile& InProfile, const FCk_CameraProfile_DepthOfField& InDepthOfField);

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Get Use Post Process")
    static bool
    Get_UsePostProcess(const FCk_Handle_CameraProfile& InProfile);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|CameraProfile", DisplayName = "[Ck][CameraProfile] Set Use Post Process")
    static FCk_Handle_CameraProfile
    Set_UsePostProcess(UPARAM(ref) FCk_Handle_CameraProfile& InProfile, bool InEnabled);

public:
    // Blends InTarget into the profile entity by InAlpha [0,1]. Continuous parameters (FOV, aspect, boom length,
    // pivot/framing offsets, springs, DoF) are linearly interpolated; discrete feature blocks (orientation control,
    // auto-reorient, collision, noise, post-process toggle) are adopted from the target once it dominates the
    // blend (alpha >= 0.5). The canonical helper a Mode modifier's DoContributeToProfile calls to layer a
    // designer-authored preset onto the running composite.
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
    // Creates a new profile entity owned by the camera, holding a default profile (all section fragments). The
    // director creates exactly one and stores its handle on FFragment_Camera_Current; it lives for the director's
    // lifetime. Internal — not exposed to BP/AS.
    static auto
    Create(
        FCk_Handle_Camera& InOwner) -> FCk_Handle_CameraProfile;

    // Writes a full profile into the section fragments. Internal — backs Reset and the handle BlendInto write-back.
    static auto
    DoWrite(
        FCk_Handle_CameraProfile& InProfile,
        const FCk_CameraProfile& InProfileValue) -> void;

    // Resets the entity's sections to a fresh default. Called once per frame by ComposeProfile before modifiers
    // contribute on top. Internal — not a public mutator.
    static auto
    Reset(
        FCk_Handle_CameraProfile& InProfile) -> void;

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
