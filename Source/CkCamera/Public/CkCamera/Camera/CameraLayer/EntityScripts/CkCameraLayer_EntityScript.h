#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkCore/Time/CkTime.h"

#include "CkCamera/Camera/CkCamera_Fragment_Data.h"

#include "CkCameraLayer_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Camera layer = entity script (the camera analog of UCk_SmState_EntityScript / UCk_SmTask_EntityScript).
//
// A layer is a child entity of the Camera director. It acquires attribute modifiers on the director's tuner
// attributes (via UCk_Utils_CameraLayer_UE::Acquire_CameraModifier_<Tuner>), naming each by this layer's
// auto-generated gameplay tag (Get_LayerTag). The framework auto-blends those modifiers in/out — the layer author
// never touches blend weights. Lifecycle: BeginPlay() -> EnterLayer(); Exit on removal (deduped by
// FTag_CameraLayer_Active). Camera is client-local only — Get_EffectiveReplication forced to DoesNotReplicate.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKCAMERA_API UCk_CameraLayer_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CameraLayer_EntityScript);

    // ================================================================================================================
    // LIFECYCLE (EntityScript overrides)
    // ================================================================================================================

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

    // ================================================================================================================
    // LAYER LIFECYCLE (Enter/Exit) — invoked by the base + the camera processors
    // ================================================================================================================

public:
    virtual auto
    EnterLayer(
        FCk_Handle_CameraLayer InHandle) -> void;

    virtual auto
    ExitLayer(
        FCk_Handle_CameraLayer InHandle) -> void;

    // Optional per-frame stateful logic, gated by _TickMode. Dispatched by the lifecycle processor.
    virtual auto
    Tick(
        FCk_Handle_CameraLayer InHandle,
        FCk_Time InDeltaT) -> void;

    // Called each frame by the blend processor with the layer's current blend alpha [0,1]. Standard tuner blending
    // (the modifiers acquired via Acquire_CameraModifier_*) is automatic; override DoBlend only for custom per-frame
    // logic that needs the alpha (e.g. a non-attribute effect that should fade with the layer).
    virtual auto
    Blend(
        FCk_Handle_CameraLayer InHandle,
        float InAlpha) -> void;

    // The layer's unique gameplay tag, derived from its class name (e.g. CameraLayer_Zoom -> "CameraLayer.Zoom").
    // Used as the modifier NAME when acquiring attribute modifiers, so each layer's modifiers are uniquely identified.
    UFUNCTION(BlueprintPure,
        Category = "Ck|Camera|Layer",
        DisplayName = "[Ck][Camera] Get Layer Tag")
    FGameplayTag
    Get_LayerTag() const;

    // ================================================================================================================
    // BLUEPRINT / ANGELSCRIPT IMPLEMENTABLE EVENTS
    // ================================================================================================================

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

    // ================================================================================================================
    // HELPERS
    // ================================================================================================================

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Camera|Layer",
        DisplayName = "[Ck][Camera] Get Owning Camera",
        meta = (CompactNodeTitle = "OwningCamera", HideSelfPin = true))
    FCk_Handle_Camera
    Get_OwningCamera() const;

    // ================================================================================================================
    // MEMBERS
    // ================================================================================================================

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
// The persistent base layer auto-created by UCk_Utils_Camera_UE::Add (one per director). It acquires no modifiers —
// the resting profile lives in the director's tuner-attribute base values; this layer exists only as the always-on,
// never-evicted floor (so feature layers blend back to the base rather than to a stale/empty stack) and as the
// dominant fallback when no real layer is active. Not meant to be added/removed by gameplay.
// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCAMERA_API UCk_CameraLayer_Default_EntityScript : public UCk_CameraLayer_EntityScript
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CameraLayer_Default_EntityScript);
};

// --------------------------------------------------------------------------------------------------------------------
