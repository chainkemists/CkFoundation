#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkWatermark/CkWatermark_Panel_Widget.h"

#include <Subsystems/LocalPlayerSubsystem.h>

#include "CkWatermark_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_Watermark")
class CKWATERMARK_API UCk_Watermark_Subsystem_UE : public ULocalPlayerSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Watermark_Subsystem_UE);

public:
    auto Deinitialize() -> void override;

private:
    auto PlayerControllerChanged(APlayerController* InNewPlayerController) -> void override;

public:
    auto Request_UpdateWatermarkDisplayPolicy(ECk_Watermark_DisplayPolicy InDisplayPolicy) const -> void;

private:
    auto DoCreateAndSetWatermarkWidget(APlayerController* InPlayerController) -> void;

private:
    UPROPERTY(Transient)
    TObjectPtr<UCkWatermark_Panel_UWidget_UE> _WatermarkWidget;
};

// --------------------------------------------------------------------------------------------------------------------
