#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkUI_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "UI"))
class CKUI_API UCk_UI_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Watermark",
              meta = (AllowPrivateAccess = true, AllowAbstract = false))
    TSoftClassPtr<class UCk_Watermark_UserWidget_UE> _WatermarkWidgetClass;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Watermark",
              meta = (AllowPrivateAccess = true, UIMin = 0, ClampMin = 0))
    int32 _WatermarkWidget_ZOrder = 999999;

public:
    CK_PROPERTY_GET(_WatermarkWidgetClass);
    CK_PROPERTY_GET(_WatermarkWidget_ZOrder);
};

// --------------------------------------------------------------------------------------------------------------------

class CKUI_API UCk_Utils_UI_Settings_UE
{
public:
    static auto Get_WatermarkWidgetClass() -> TSubclassOf<class UCk_Watermark_UserWidget_UE>;
    static auto Get_WatermarkWidget_ZOrder() -> int32;
};

// --------------------------------------------------------------------------------------------------------------------
