#pragma once

#include "CkCamera/Camera/CkCamera_Fragment.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkCamera_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_CameraLayer_EntityScript;

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
        Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Add")
    static FCk_Handle_Camera
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_Fragment_Camera_ParamsData& InParams);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Has Feature")
    static bool
    Has(
        const FCk_Handle& InHandle);

public:
    // Number of live layer entities currently on the camera (active or blending).
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Get Layer Count")
    static int32
    Get_LayerCount(
        const FCk_Handle_Camera& InCamera);

    // True if a live layer of the given class is present on the camera.
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Has Layer")
    static bool
    Has_Layer(
        const FCk_Handle_Camera& InCamera,
        TSubclassOf<UCk_CameraLayer_EntityScript> InLayerClass);

    // Class of the dominant active layer (highest blend alpha), or null if none.
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Get Dominant Layer Class")
    static TSubclassOf<UCk_CameraLayer_EntityScript>
    Get_DominantLayerClass(
        const FCk_Handle_Camera& InCamera);

    // The resolved composed profile (this frame's tuner-attribute finals + bool/curve leaves). Handy for gameplay that
    // needs the live FOV / boom / framing, and for tests asserting composition. Refreshed each frame by the lifecycle
    // processor; returns a default profile if the camera has no Current fragment yet.
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Get Composed Profile")
    static FCk_CameraProfile
    Get_ComposedProfile(
        const FCk_Handle_Camera& InCamera);

    // Assembles a fresh FCk_CameraProfile from the camera's tuner attributes' final values + the Current bool/curve
    // leaves. Get_ComposedProfile returns the cached snapshot refreshed each frame by the lifecycle processor; this
    // recomputes it on demand (and is what the lifecycle processor calls to refresh the cache).
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Camera",
        DisplayName = "[Ck][Camera] Get Profile (Assemble)")
    static FCk_CameraProfile
    Get_Profile(
        const FCk_Handle_Camera& InCamera);

private:
    // Materializes every FCk_CameraProfile leaf into a (non-replicated) tuner attribute on the camera (Float / Vector
    // / Rotator / Integer; FloatRange → Float-with-MinMax), labels each with its Camera.* native tag, fills the
    // per-section fragments, and writes the bool/curve leaves onto FFragment_Camera_Current. Called from Add.
    static auto
    DoMaterializeAttributes(
        FCk_Handle_Camera& InCamera,
        const FCk_CameraProfile& InDefaults) -> void;

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
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Request Add Layer")
    static FCk_Handle_Camera
    Request_AddLayer(
        UPARAM(ref) FCk_Handle_Camera& InCamera,
        const FCk_Request_Camera_AddLayer& InRequest);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Request Remove Layer")
    static FCk_Handle_Camera
    Request_RemoveLayer(
        UPARAM(ref) FCk_Handle_Camera& InCamera,
        const FCk_Request_Camera_RemoveLayer& InRequest);

public:
    // Feeds the camera's abstract orbit intention (X = yaw, Y = pitch), already scaled/inverted by the caller's
    // input/user-preference handling. The camera module never reads input devices.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Request Set Orientation Intention")
    static FCk_Handle_Camera
    Request_SetOrientationIntention(
        UPARAM(ref) FCk_Handle_Camera& InCamera,
        FVector InOrientationIntention);

public:
    // ---- Toggles for the non-blending bool leaves stored on FFragment_Camera_Current. Local-only, take effect the
    //      same frame, and are reflected by Get_ComposedProfile / the POV pipeline. ----

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Use Fixed Boom Rotation")
    static FCk_Handle_Camera
    Request_Set_UseFixedBoomRotation(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Constrain Aspect Ratio")
    static FCk_Handle_Camera
    Request_Set_ConstrainAspectRatio(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Has Orientation Control")
    static FCk_Handle_Camera
    Request_Set_HasOrientationControl(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Has Auto Reorient")
    static FCk_Handle_Camera
    Request_Set_HasAutoReorient(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Has Collision")
    static FCk_Handle_Camera
    Request_Set_HasCollision(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Use Async Trace")
    static FCk_Handle_Camera
    Request_Set_UseAsyncTrace(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Use Post Process")
    static FCk_Handle_Camera
    Request_Set_UsePostProcess(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled);

public:
    // The resolved POV for this frame (the composed view), as written by UpdatePOV. The camera component
    // (UCk_CameraComponent::GetCameraView) reads this; gameplay can too.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Get View Info")
    static FMinimalViewInfo
    Get_ViewInfo(
        const FCk_Handle_Camera& InCamera);

    // The resolved camera view rotation (this frame's composed POV).
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Get View Rotation")
    static FRotator
    Get_ViewRotation(
        const FCk_Handle_Camera& InCamera);
};

// --------------------------------------------------------------------------------------------------------------------
