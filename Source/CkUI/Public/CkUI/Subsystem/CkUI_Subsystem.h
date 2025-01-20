#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Registry/CkRegistry.h"
#include "CkEcs/Handle/CkHandle.h"

#include "CkUI/CustomWidgets/Watermark/CkWatermark_Widget.h"
#include "CkUI/WidgetLayerHandler/CkWidgetLayerHandler_Fragment_Data.h"

#include <Subsystems/LocalPlayerSubsystem.h>

#include "CkUI_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

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
    auto Get_WidgetLayerHandler() const -> FCk_Handle_WidgetLayerHandler;

private:
    auto DoCreateAndSetWatermarkWidget(
        APlayerController* InPlayerController) -> void;

private:
    UPROPERTY(Transient)
    TObjectPtr<UCk_Watermark_UserWidget_UE> _WatermarkWidget;

private:
    FCk_Registry _Registry;
    FCk_Handle _SubsystemEntity;
};

// --------------------------------------------------------------------------------------------------------------------
