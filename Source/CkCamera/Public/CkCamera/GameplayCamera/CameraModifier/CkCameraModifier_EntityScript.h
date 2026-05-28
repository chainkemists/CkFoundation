#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkCore/Time/CkTime.h"

#include "CkCamera/GameplayCamera/CkGameplayCamera_Fragment_Data.h"
#include "CkCamera/GameplayCamera/CkGameplayCamera_Profile.h"

#include "CkCameraModifier_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Camera modifier = entity script (the direct analog of UCk_SmState_EntityScript).
//
// A modifier is a child entity of the GameplayCamera director. Its lifecycle bridges the generic EntityScript
// lifecycle to camera hooks: BeginPlay() -> EnterModifier(); the camera processors drive ContributeToProfile/Tick,
// and Exit is driven on removal (deduped by FTag_CameraModifier_Active, independent of EndPlay).
//
// Camera is client-local only (decision 7) — Get_EffectiveReplication is forced to DoesNotReplicate.
// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, BlueprintType)
class CKCAMERA_API UCk_CameraModifier_EntityScript : public UCk_EntityScript_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_CameraModifier_EntityScript);

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
    // MODIFIER LIFECYCLE (Enter/Exit) — invoked by the base + the camera processors
    // ================================================================================================================

public:
    virtual auto
    EnterModifier(
        FCk_Handle_CameraModifier InHandle) -> void;

    virtual auto
    ExitModifier(
        FCk_Handle_CameraModifier InHandle) -> void;

    // Called every tick by FProcessor_GameplayCamera_ComposeProfile. The modifier writes/accumulates
    // its contribution into the running profile, weighted by its own blend alpha. [dispatch wired in M1]
    virtual auto
    ContributeToProfile(
        FCk_Handle_CameraModifier InHandle,
        FCk_GameplayCamera_Profile& InOutProfile,
        float InBlendAlpha) -> void;

    // Optional per-frame stateful logic, gated by _TickMode. [dispatch wired in M1]
    virtual auto
    Tick(
        FCk_Handle_CameraModifier InHandle,
        FCk_Time InDeltaT) -> void;

    // ================================================================================================================
    // BLUEPRINT / ANGELSCRIPT IMPLEMENTABLE EVENTS
    // ================================================================================================================

protected:
    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|GameplayCamera|Modifier",
        DisplayName = "Enter")
    void
    DoEnter(
        FCk_Handle_CameraModifier InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|GameplayCamera|Modifier",
        DisplayName = "Exit")
    void
    DoExit(
        FCk_Handle_CameraModifier InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|GameplayCamera|Modifier",
        DisplayName = "Contribute To Profile")
    void
    DoContributeToProfile(
        FCk_Handle_CameraModifier InHandle,
        UPARAM(ref) FCk_GameplayCamera_Profile& InOutProfile,
        float InBlendAlpha);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|GameplayCamera|Modifier",
        DisplayName = "Tick")
    void
    DoTick(
        FCk_Handle_CameraModifier InHandle,
        FCk_Time InDeltaT);

    // ================================================================================================================
    // HELPERS
    // ================================================================================================================

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|GameplayCamera|Modifier",
        DisplayName = "[Ck][GameplayCamera] Get Owning Camera",
        meta = (CompactNodeTitle = "OwningCamera", HideSelfPin = true))
    FCk_Handle_GameplayCamera
    Get_OwningCamera() const;

    // ================================================================================================================
    // MEMBERS
    // ================================================================================================================

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "GameplayCamera|Modifier",
        meta = (AllowPrivateAccess = true))
    ECk_GameplayCamera_TickMode _TickMode = ECk_GameplayCamera_TickMode::EnterExitOnly;

public:
    CK_PROPERTY_GET(_TickMode);

private:
    FCk_Handle_GameplayCamera _OwningCamera;
};

// --------------------------------------------------------------------------------------------------------------------
