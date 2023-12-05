#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkGameSession/Subsystem/CkGameSession_Subsystem.h"

#include "CkUI/CustomWidgets/Watermark/CkWatermark_Widget.h"

#include <Subsystems/GameInstanceSubsystem.h>

#include "CkUI_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "CkSubsystem_UI")
class CKUI_API UCk_UI_Subsystem_UE : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_Subsystem_UE);

public:
    auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    auto Deinitialize() -> void override;
    auto ShouldCreateSubsystem(UObject* InOuter) const -> bool override;

private:
    auto OnPlayerControllerReady(
        TWeakObjectPtr<APlayerController> InNewPlayerController,
        TArray<TWeakObjectPtr<APlayerController>> InAllPlayerControllers) -> void;

public:
    auto Request_UpdateWatermarkDisplayPolicy(ECk_Watermark_DisplayPolicy InDisplayPolicy) const -> void;

private:
    auto DoCreateAndSetWatermarkWidget() -> void;

private:
    UPROPERTY(Transient)
    TObjectPtr<UCk_Watermark_UserWidget_UE> _WatermarkWidget;

private:
    ck::UUtils_Signal_OnLoginEvent_PostFireUnbind::ConnectionType _PostFireUnbind_Connection;
};

// --------------------------------------------------------------------------------------------------------------------
