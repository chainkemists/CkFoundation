// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUICore/ScreenFade/CkScreenFade_Utils.h"

#include <Subsystems/LocalPlayerSubsystem.h>

#include "CkScreenFade_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class APlayerController;
class SWidget;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Per-player screen fade subsystem.
 *
 * Owns the fade widget for each local player and adds/removes it from the game viewport.
 * A fade that ends fully transparent removes its own widget on completion.
 */
UCLASS(DisplayName = "CkSubsystem_ScreenFade")
class CKUICORE_API UCk_ScreenFade_Subsystem_UE : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_ScreenFade_Subsystem_UE);

    // ----------------------------------------------------------------------------------------------------------------

public:
    auto Request_AddScreenFadeWidget(
        const FCk_ScreenFade_Params& InFadeParams,
        const APlayerController* InOwningPlayer = nullptr,
        int32 InZOrder = 100) -> void;

    // ----------------------------------------------------------------------------------------------------------------

private:
    auto DoRemoveScreenFadeWidget(const APlayerController* InOwningPlayer, int32 InControllerID) -> void;
    auto DoRemoveScreenFadeWidget(int32 InControllerID) -> void;

    auto DoGet_PlayerControllerID(const APlayerController* PlayerController) const -> int32;
    auto DoGet_PlayerControllerFromID(int32 ControllerID) const -> APlayerController*;

    // ----------------------------------------------------------------------------------------------------------------

private:
    TMap<int32, TWeakPtr<SWidget>> _FadeWidgetsForID;
    static constexpr int32 _InvalidPlayerControllerID = INT_MIN;
};

// --------------------------------------------------------------------------------------------------------------------
