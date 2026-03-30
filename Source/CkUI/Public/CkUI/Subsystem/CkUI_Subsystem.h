// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/ScreenFade/CkScreenFade_Utils.h"
#include "CkUI/Types/CkUI_Types.h"

#include <Subsystems/LocalPlayerSubsystem.h>

#include "CkUI_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class APlayerController;
class SWidget;

// --------------------------------------------------------------------------------------------------------------------

/**
 * General per-player UI subsystem.
 *
 * Responsibilities:
 * - Input suspension management with handle-based tracking
 * - Automatic input restoration during editor modal dialogs
 * - Screen fade effects
 *
 * Note: Layout management is handled by UCk_UI_Layout_Subsystem_UE.
 *       Context injection is handled by the ECS ContextReceiver system.
 */
UCLASS(DisplayName = "CkSubsystem_UI")
class CKUI_API UCk_UI_Subsystem_UE : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_Subsystem_UE);

    // ----------------------------------------------------------------------------------------------------------------
    // Lifecycle
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

    // ----------------------------------------------------------------------------------------------------------------
    // Input Suspension
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
    auto SuspendInput(FName InReason) -> FCk_UI_InputSuspensionToken;

    /**
     * Resume input for a specific suspension.
     * Safe to call with invalid handles (will no-op).
     * The handle will be marked invalid after this call.
     *
     * @param InSuspendToken The token returned from SuspendInput
     */
    auto ResumeInput(FCk_UI_InputSuspensionToken& InSuspendToken) -> void;

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
    // Screen Fade
    // ----------------------------------------------------------------------------------------------------------------

public:
    auto Request_AddScreenFadeWidget(
        const FCk_ScreenFade_Params& InFadeParams,
        const APlayerController* InOwningPlayer = nullptr,
        int32 InZOrder = 100) -> void;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Screen Fade
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoRemoveScreenFadeWidget(const APlayerController* InOwningPlayer, int32 InControllerID) -> void;
    auto DoRemoveScreenFadeWidget(int32 InControllerID) -> void;

    auto DoGet_PlayerControllerID(const APlayerController* PlayerController) const -> int32;
    auto DoGet_PlayerControllerFromID(int32 ControllerID) const -> APlayerController*;

    // ----------------------------------------------------------------------------------------------------------------
    // Internal - Input Suspension
    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoApplyInputFilter(FName InToken, bool InShouldFilter) const -> void;
    auto DoGenerateSuspendTokenName(FName InReason) const -> FName;

#if WITH_EDITOR
    auto DoHandleModalLoopTick(float InDeltaTime) -> void;
    auto DoCheckAndRestoreFiltersAfterModal() -> void;
    auto DoSuspendFiltersForModal() -> void;
    auto DoRestoreFiltersAfterModal() -> void;
#endif

    // ----------------------------------------------------------------------------------------------------------------
    // Properties - Screen Fade
    // ----------------------------------------------------------------------------------------------------------------

private:
    TMap<int32, TWeakPtr<SWidget>> _FadeWidgetsForID;
    static constexpr int32 _InvalidPlayerControllerID = INT_MIN;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties - Input Suspension
    // ----------------------------------------------------------------------------------------------------------------

private:
    TMap<uint32, FCk_UI_InputSuspensionToken> _ActiveSuspensions;
    uint32 _SuspensionIdCounter = 0;

#if WITH_EDITOR
    FDelegateHandle _ModalDialogTickHandle;
    bool _IsInModalLoop = false;
    TArray<FName> _SuspendedTokensDuringModal;
#endif
};

// --------------------------------------------------------------------------------------------------------------------