#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkCore/Time/CkTime.h"

#include "CkCamera/Camera/CkCamera_Fragment_Data.h"

#include "CkCameraLayer_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKCAMERA_API UCk_CameraLayer_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CameraLayer_EntityScript);

    // LIFECYCLE (EntityScript overrides)

protected:
    auto
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams) -> ECk_EntityScript_ConstructionFlow override;

    auto
    BeginPlay() -> void override;

    auto
    EndPlay() -> void override;

    // Camera is never replicated — always DoesNotReplicate regardless of the CkEntityScript CDO default.
    auto
    Get_EffectiveReplication() const -> ECk_Replication override;

    // LAYER LIFECYCLE (Enter/Exit) — invoked by the base + the camera processors

public:
    virtual auto
    EnterLayer(
        FCk_Handle_CameraLayer InHandle) -> void;

    virtual auto
    ExitLayer(
        FCk_Handle_CameraLayer InHandle) -> void;

    virtual auto
    Tick(
        FCk_Handle_CameraLayer InHandle,
        FCk_Time InDeltaT) -> void;

    // Tuner blending is already automatic; override only for custom per-frame logic that needs the alpha.
    virtual auto
    Blend(
        FCk_Handle_CameraLayer InHandle,
        float InAlpha) -> void;

    // Opt-in boundary hooks, fired once per alpha crossing. FullyBlendedOut is best-effort — if the layer is pruned
    // before the blend processor observes alpha 0, only ExitLayer/DoExit runs.
    virtual auto
    FullyBlendedIn(
        FCk_Handle_CameraLayer InHandle) -> void;

    virtual auto
    FullyBlendedOut(
        FCk_Handle_CameraLayer InHandle) -> void;

    // Derived from the class name (CameraLayer_Zoom -> "CameraLayer.Zoom"); names this layer's acquired modifiers.
    UFUNCTION(BlueprintPure,
        Category = "Ck|Camera|Layer",
        DisplayName = "[Ck][Camera] Get Layer Tag")
    FGameplayTag
    Get_LayerTag() const;

    // BLUEPRINT / ANGELSCRIPT IMPLEMENTABLE EVENTS

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|Camera|Layer",
        DisplayName = "Enter")
    void
    DoEnter(
        FCk_Handle_CameraLayer InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|Camera|Layer",
        DisplayName = "Exit")
    void
    DoExit(
        FCk_Handle_CameraLayer InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|Camera|Layer",
        DisplayName = "Tick")
    void
    DoTick(
        FCk_Handle_CameraLayer InHandle,
        FCk_Time InDeltaT);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|Camera|Layer",
        DisplayName = "Blend")
    void
    DoBlend(
        FCk_Handle_CameraLayer InHandle,
        float InAlpha);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|Camera|Layer",
        DisplayName = "Fully Blended In")
    void
    DoFullyBlendedIn(
        FCk_Handle_CameraLayer InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|Camera|Layer",
        DisplayName = "Fully Blended Out")
    void
    DoFullyBlendedOut(
        FCk_Handle_CameraLayer InHandle);

    // HELPERS

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Camera|Layer",
        DisplayName = "[Ck][Camera] Get Owning Camera",
        meta = (CompactNodeTitle = "OwningCamera", HideSelfPin = true))
    FCk_Handle_Camera
    Get_OwningCamera() const;

    // MEMBERS

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Camera|Layer",
        meta = (AllowPrivateAccess = true))
    ECk_Camera_TickMode _TickMode = ECk_Camera_TickMode::EnterExitOnly;

public:
    CK_PROPERTY_GET(_TickMode);

private:
    FCk_Handle_Camera _OwningCamera;
};

// --------------------------------------------------------------------------------------------------------------------
// The persistent base layer auto-created by UCk_Utils_Camera_UE::Add. Never added or removed by gameplay.
// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCAMERA_API UCk_CameraLayer_Default_EntityScript : public UCk_CameraLayer_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CameraLayer_Default_EntityScript);
};

// --------------------------------------------------------------------------------------------------------------------
