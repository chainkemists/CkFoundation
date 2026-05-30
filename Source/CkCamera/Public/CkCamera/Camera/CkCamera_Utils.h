#pragma once

#include "CkCamera/Camera/CkCamera_Fragment.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkCamera_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_Camera"))
class CKCAMERA_API UCk_Utils_Camera_UE : public UCk_Utils_Ecs_Base_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Camera_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_Camera);

public:
    friend class UCk_Utils_Ecs_Base_UE;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Camera] Add")
    static FCk_Handle_Camera
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Camera_ParamsData& InParams);

public:
    // Has Feature
    static bool
    Has(
        const FCk_Handle& InHandle);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Has")
    static bool
    Has_Any(
        const FCk_Handle& InHandle);

public:
    // Number of live modifier entities currently on the camera (active or blending).
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Get Modifier Count")
    static int32
    Get_ModifierCount(
        const FCk_Handle_Camera& InCamera);

    // True if a live modifier of the given class is present on the camera.
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Has Modifier")
    static bool
    Has_Modifier(
        const FCk_Handle_Camera& InCamera,
        TSubclassOf<UCk_CameraModifier_EntityScript> InModifierClass);

    // Class of the dominant active modifier (highest blend alpha), or null if none.
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Get Dominant Modifier Class")
    static TSubclassOf<UCk_CameraModifier_EntityScript>
    Get_DominantModifierClass(
        const FCk_Handle_Camera& InCamera);

    // The resolved composed profile (this frame's blended Modes + layered Trims). Handy for gameplay that needs
    // the live FOV / boom / framing, and for tests asserting composition. Refreshed each frame by ComposeProfile;
    // returns a default profile if the camera has no Current fragment yet.
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Get Composed Profile")
    static FCk_CameraProfile
    Get_ComposedProfile(
        const FCk_Handle_Camera& InCamera);

private:
    UFUNCTION(BlueprintCallable,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Cast",
        meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_Camera
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Handle -> Camera Handle",
        meta = (CompactNodeTitle = "<AsCamera>", BlueprintAutocast))
    static FCk_Handle_Camera
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
        DisplayName = "[Ck] Get Invalid Camera Handle",
        Category = "Ck|Utils|Camera",
        meta = (CompactNodeTitle = "INVALID_CameraHandle", Keywords = "make"))
    static FCk_Handle_Camera
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Camera] Request Add Modifier")
    static FCk_Handle_Camera
    Request_AddModifier(
        UPARAM(ref) FCk_Handle_Camera& InCamera,
        const FCk_Request_Camera_AddModifier& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|BLUEPRINT_INTERNAL_USE_ONLY",
              DisplayName = "[Ck][Camera] Request Remove Modifier")
    static FCk_Handle_Camera
    Request_RemoveModifier(
        UPARAM(ref) FCk_Handle_Camera& InCamera,
        const FCk_Request_Camera_RemoveModifier& InRequest);

public:
    // Feeds the camera's abstract orbit intention (X = yaw, Y = pitch), already scaled/inverted by the caller's
    // input/user-preference handling. The camera module never reads input devices — a fixed/follow/lock-on camera
    // simply never calls this (intention stays zero). Set immediately each frame; consumed later the same frame
    // by the camera processors (which run after the Transform groups).
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Request Set Orientation Intention")
    static FCk_Handle_Camera
    Request_SetOrientationIntention(
        UPARAM(ref) FCk_Handle_Camera& InCamera,
        FVector InOrientationIntention);

public:
    // The resolved camera view rotation (this frame's composed POV). Handy for camera-relative movement: take the
    // yaw to derive a horizontal forward/right. Returns zero rotation if the camera has no composed view yet.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Get View Rotation")
    static FRotator
    Get_ViewRotation(
        const FCk_Handle_Camera& InCamera);
};

// --------------------------------------------------------------------------------------------------------------------
