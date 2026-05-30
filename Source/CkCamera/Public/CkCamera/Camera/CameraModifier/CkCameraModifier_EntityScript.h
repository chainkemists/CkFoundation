#pragma once

#include "CkEcs/EntityScript/CkEntityScript.h"

#include "CkCore/Time/CkTime.h"

#include "CkCamera/Camera/CkCamera_Fragment_Data.h"
#include "CkCamera/Camera/CkCameraProfile.h"

#include "CkCameraModifier_EntityScript.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Camera modifier = entity script (the direct analog of UCk_SmState_EntityScript).
//
// A modifier is a child entity of the Camera director. Its lifecycle bridges the generic EntityScript
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

    // Called every tick by FProcessor_Camera_ComposeProfile. The modifier contributes into the running profile
    // entity (passed as a handle, not a by-ref struct), weighted by its own blend alpha — typically by calling
    // UCk_Utils_CameraProfile_UE::BlendInto(InOutProfile, <authored target>, InBlendAlpha).
    virtual auto
    ContributeToProfile(
        const FCk_Handle_CameraModifier& InHandle,
        FCk_Handle_CameraProfile& InOutProfile,
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
        Category = "Ck|Camera|Modifier",
        DisplayName = "Enter")
    void
    DoEnter(
        FCk_Handle_CameraModifier InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|Camera|Modifier",
        DisplayName = "Exit")
    void
    DoExit(
        FCk_Handle_CameraModifier InHandle);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|Camera|Modifier",
        DisplayName = "Contribute To Profile")
    void
    DoContributeToProfile(
        const FCk_Handle_CameraModifier& InHandle,
        UPARAM(ref) FCk_Handle_CameraProfile& InOutProfile,
        float InBlendAlpha);

    UFUNCTION(BlueprintImplementableEvent,
        Category = "Ck|Camera|Modifier",
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
        Category = "Ck|Camera|Modifier",
        DisplayName = "[Ck][Camera] Get Owning Camera",
        meta = (CompactNodeTitle = "OwningCamera", HideSelfPin = true))
    FCk_Handle_Camera
    Get_OwningCamera() const;

    // ================================================================================================================
    // MEMBERS
    // ================================================================================================================

protected:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Camera|Modifier",
        meta = (AllowPrivateAccess = true))
    ECk_Camera_TickMode _TickMode = ECk_Camera_TickMode::EnterExitOnly;

    // Mode (default) contributes a whole preset and runs before all Trims; Trim layers field-level adjustments on
    // top. See ECk_Camera_ModifierRole. Existing whole-camera modifiers stay Mode.
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        Category = "Camera|Modifier",
        meta = (AllowPrivateAccess = true))
    ECk_Camera_ModifierRole _Role = ECk_Camera_ModifierRole::Mode;

public:
    CK_PROPERTY_GET(_TickMode);
    CK_PROPERTY_GET(_Role);

private:
    FCk_Handle_Camera _OwningCamera;
};

// --------------------------------------------------------------------------------------------------------------------
