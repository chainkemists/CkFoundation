#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUI/CustomWidgets/Watermark/CkWatermark_Widget.h"

#include <Subsystems/GameInstanceSubsystem.h>

#include "CkUI_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(DisplayName = "Ck UI Subsystem")
class CKUI_API UCk_UI_Subsystem_UE : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_Subsystem_UE);

public:
    virtual auto Initialize(FSubsystemCollectionBase& InCollection) -> void override;
    virtual auto Deinitialize() -> void override;

public:
    auto Request_UpdateWatermarkDisplayPolicy(ECk_Watermark_DisplayPolicy InDisplayPolicy) const -> void;

private:
    UPROPERTY(Transient)
    TObjectPtr<UCk_Watermark_UserWidget_UE> _WatermarkWidget;
};

// --------------------------------------------------------------------------------------------------------------------
