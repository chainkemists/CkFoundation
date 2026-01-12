// Copyright 2025 CkFoundation. All Rights Reserved.

#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkUI/CustomWidgets/Watermark/CkWatermark_Widget.h"
#include "CkUI/ScreenFade/CkScreenFade_Utils.h"

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
 * - Watermark widget management
 * - Screen fade effects
 *
 * Note: Layout management is handled by UCk_UI_Layout_Subsystem_UE.
 *       Context injection is handled by UCk_UI_Context_Subsystem_UE.
 */
UCLASS(DisplayName = "CkSubsystem_UI")
class CKUI_API UCk_UI_Subsystem_UE : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_Subsystem_UE);

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;

private:
    auto PlayerControllerChanged(APlayerController* InNewPlayerController) -> void override;

public:
    auto Request_UpdateWatermarkDisplayPolicy(ECk_Watermark_DisplayPolicy InDisplayPolicy) const -> void;
    auto Request_AddScreenFadeWidget(const FCk_ScreenFade_Params& InFadeParams, const APlayerController* InOwningPlayer = nullptr, int32 InZOrder = 100) -> void;

private:
    auto DoCreateAndSetWatermarkWidget(APlayerController* InPlayerController) -> void;
    auto DoRemoveScreenFadeWidget(const APlayerController* InOwningPlayer, int32 InControllerID) -> void;
    auto DoRemoveScreenFadeWidget(int32 InControllerID) -> void;

    auto DoGet_PlayerControllerID(const APlayerController* PlayerController) const -> int32;
    auto DoGet_PlayerControllerFromID(const int32 ControllerID) const -> APlayerController*;

    // ----------------------------------------------------------------------------------------------------------------
    // Properties
    // ----------------------------------------------------------------------------------------------------------------

private:
    UPROPERTY(Transient)
    TObjectPtr<UCk_Watermark_UserWidget_UE> _WatermarkWidget;

    TMap<int32, TWeakPtr<SWidget>> _FadeWidgetsForID;

    static constexpr int32 _InvalidPlayerControllerID = INT_MIN;
};

// --------------------------------------------------------------------------------------------------------------------