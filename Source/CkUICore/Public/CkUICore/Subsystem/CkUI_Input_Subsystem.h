// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Subsystems/LocalPlayerSubsystem.h>

#include "CkUI_Input_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class ULocalPlayer;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Opaque handle representing an active input suspension.
 *
 * Created via UCk_Utils_UI_UE::SuspendInput() or UCk_UI_Input_Subsystem_UE::SuspendInput().
 * Must be explicitly resumed via Resume() or UCk_Utils_UI_UE::ResumeInput().
 *
 * The handle tracks:
 * - Unique ID for this suspension instance
 * - The underlying CommonInputSubsystem token
 * - Weak reference to the owning LocalPlayer
 *
 * Invalid handles (default constructed or already resumed) are safe to use -
 * Resume() will simply no-op.
 */
USTRUCT(BlueprintType)
struct CKUICORE_API FCk_Handle_InputSuspension
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_InputSuspension);

    FCk_Handle_InputSuspension() = default;

    auto IsValid() const -> bool;
    auto Resume() -> void;
    auto Get_LocalPlayer() const -> const ULocalPlayer*;

    auto operator==(const ThisType& Other) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    friend class UCk_UI_Input_Subsystem_UE;

    static auto Create(
        uint32 InId,
        FName InReason,
        FName InToken,
        const ULocalPlayer* InLocalPlayer) -> FCk_Handle_InputSuspension;


    auto DoMarkInvalid() -> void;

private:
    uint32 _Id = 0;

    UPROPERTY()
    FName _Reason = NAME_None;

    UPROPERTY()
    FName _Token = NAME_None;

    UPROPERTY()
    TWeakObjectPtr<const ULocalPlayer> _LocalPlayer;

public:
    CK_PROPERTY_GET(_Id);
    CK_PROPERTY_GET(_Reason);
    CK_PROPERTY_GET(_Token);
};

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_Handle_InputSuspension, [](const FCk_Handle_InputSuspension& InHandle)
{
    return ck::Format(TEXT("Id:[{}] | Reason:[{}]"), InHandle.Get_Id(), InHandle.Get_Reason());
});

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_Handle_InputSuspension, IsValid_Policy_Default,
[](const FCk_Handle_InputSuspension& InHandle)
{
    return InHandle.IsValid();
});

// --------------------------------------------------------------------------------------------------------------------

/**
 * Per-player input-policy subsystem.
 *
 * Responsibilities:
 * - Input suspension management with handle-based tracking
 * - Automatic input restoration during editor modal dialogs
 *
 * Note: Layout management is handled by UCk_UI_Layout_Subsystem_UE.
 *       Screen fade is handled by UCk_ScreenFade_Subsystem_UE.
 *       Context injection is handled by the ECS ContextReceiver system.
 */
UCLASS(DisplayName = "CkSubsystem_UI_Input")
class CKUICORE_API UCk_UI_Input_Subsystem_UE : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_Input_Subsystem_UE);

    // ----------------------------------------------------------------------------------------------------------------

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

    // ----------------------------------------------------------------------------------------------------------------

public:
    /**
     * Suspend all input for this player.
     * Returns a handle that must be used to resume input.
     * Multiple suspensions can be active simultaneously - input is only
     * fully restored when all handles have been resumed.
     *
     * @param InReason Category/reason for the suspension (for debugging)
     * @return Token to resume this specific suspension
     */
    auto SuspendInput(FName InReason) -> FCk_Handle_InputSuspension;

    /**
     * Resume input for a specific suspension.
     * Safe to call with invalid handles (will no-op).
     * The handle will be marked invalid after this call.
     *
     * @param InSuspensionHandle The handle returned from SuspendInput
     */
    auto ResumeInput(FCk_Handle_InputSuspension& InSuspensionHandle) -> void;

    /**
     * Resume all active input suspensions for this player.
     * Use sparingly - primarily for emergency cleanup.
     */
    auto ResumeAllInput() -> void;

    /**
     * Check if input is currently suspended for this player.
     */
    UFUNCTION(BlueprintPure, Category = "Ck|UI")
    bool IsInputSuspended() const;

    /**
     * Get the number of active input suspensions.
     */
    UFUNCTION(BlueprintPure, Category = "Ck|UI")
    int32 Get_ActiveSuspensionCount() const;

    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoApplyInputFilter(FName InToken, bool InShouldFilter) const -> void;
    auto DoGenerateSuspensionHandleName(FName InReason) const -> FName;

#if WITH_EDITOR
    auto DoHandleModalLoopTick(float InDeltaTime) -> void;
    auto DoCheckAndRestoreFiltersAfterModal() -> void;
    auto DoSuspendFiltersForModal() -> void;
    auto DoRestoreFiltersAfterModal() -> void;
#endif

    // ----------------------------------------------------------------------------------------------------------------

private:
    TMap<uint32, FCk_Handle_InputSuspension> _ActiveSuspensions;
    uint32 _SuspensionIdCounter = 0;

#if WITH_EDITOR
    FDelegateHandle _ModalDialogTickHandle;
    bool _IsInModalLoop = false;
    TArray<FName> _SuspendedTokensDuringModal;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
