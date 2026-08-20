#pragma once

#include "CkCamera/Camera/CkCamera_Fragment.h"

#include "CkEcsExt/CkEcsExt_Utils.h"

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Request/CkRequest_Completion.h"

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
        UPARAM(ref) FCk_Handle_Transform& InHandle,
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
              DisplayName = "[Ck][Camera] Request Add Layer",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_AddLayer(
        UPARAM(ref) FCk_Handle_Camera& InCamera,
        const FCk_Request_Camera_AddLayer& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Request Remove Layer",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_RemoveLayer(
        UPARAM(ref) FCk_Handle_Camera& InCamera,
        const FCk_Request_Camera_RemoveLayer& InRequest,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    // Feeds the camera's abstract orbit intention (X = yaw, Y = pitch), already scaled/inverted by the caller's
    // input/user-preference handling. The camera module never reads input devices.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Request Set Orientation Intention",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_SetOrientationIntention(
        UPARAM(ref) FCk_Handle_Camera& InCamera,
        FVector InOrientationIntention,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // One-shot absolute seed of the persistent boom (view) rotation — the orbit equivalent of a teleport. Unlike
    // SetOrientationIntention (a per-frame delta) this snaps the view to a world rotation.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Request Snap Boom Rotation",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_SnapBoomRotation(
        UPARAM(ref) FCk_Handle_Camera& InCamera,
        FRotator InWorldRotation,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    // Override the orientation-control yaw clamp window (absolute world yaws). The POV clamp is wrap-safe, so a cone
    // straddling +/-180 is fine; pass [-180, 180] to restore unrestricted look. Min/Max are the window edges, NOT a
    // +/- half-angle — callers wanting a cone around a facing pass [Facing - Half, Facing + Half].
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Request Set Orientation Yaw Limits",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_Set_OrientationYawLimits(
        UPARAM(ref) FCk_Handle_Camera& InCamera,
        float InMinYaw,
        float InMaxYaw,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    // ---- Toggles for the non-blending bool leaves stored on FFragment_Camera_Current. Local-only, take effect the
    //      same frame, and are reflected by Get_ComposedProfile / the POV pipeline. ----

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Use Fixed Boom Rotation",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_Set_UseFixedBoomRotation(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Constrain Aspect Ratio",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_Set_ConstrainAspectRatio(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate);

    /**
     * Switches the camera between perspective and orthographic, optionally setting the orthographic depth range
     * in the same call. Takes effect the same frame, like the toggles above.
     *
     * The orthographic WIDTH is not here: it blends, so it is a tuner attribute driven through
     * Acquire_CameraModifier_OrthoWidth like FOV, and setting it from two places would let a layer and a direct
     * write fight over it.
     */
    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Projection Mode",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_SetProjectionMode(UPARAM(ref) FCk_Handle_Camera& InCamera, const FCk_Request_Camera_SetProjectionMode& InRequest, const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Has Orientation Control",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_Set_HasOrientationControl(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Has Auto Reorient",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_Set_HasAutoReorient(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Has Collision",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_Set_HasCollision(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Use Async Trace",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_Set_UseAsyncTrace(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable, Category = "Ck|Utils|Camera", DisplayName = "[Ck][Camera] Request Set Use Post Process",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static FCk_Handle_Camera
    Request_Set_UsePostProcess(UPARAM(ref) FCk_Handle_Camera& InCamera, bool bInEnabled, const FCk_Delegate_Request_OnCompleted& InDelegate);

public:
    // The resolved POV for this frame (the composed view), as written by UpdatePOV. The camera component
    // (UCk_CameraComponent::GetCameraView) reads this; gameplay can too.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Get View Info")
    static FMinimalViewInfo
    Get_ViewInfo(
        const FCk_Handle_Camera& InCamera);

    // The composed view's attachable transform presence. Scene-node-attach here to have content follow
    // the rendered view; children compose in the frame's late-resolve pass, after the POV publish.
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Camera",
              DisplayName = "[Ck][Camera] Get View Anchor")
    static FCk_Handle_Transform
    Get_ViewAnchor(
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
