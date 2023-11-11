#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkUI_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "UI"))
class CKUI_API UCk_UI_ProjectSettings_UE : public UCk_Engine_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_UI_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "Watermark",
              meta = (AllowPrivateAccess = true, AllowAbstract = false))
    TSoftClassPtr<class UCk_Watermark_UserWidget_UE> _WatermarkWidgetClass;

public:
    CK_PROPERTY_GET(_WatermarkWidgetClass);
};

// --------------------------------------------------------------------------------------------------------------------

class CKUI_API UCk_Utils_UI_ProjectSettings_UE
{
public:
    static auto Get_WatermarkWidgetClass() -> TSubclassOf<class UCk_Watermark_UserWidget_UE>;
};

// --------------------------------------------------------------------------------------------------------------------
