#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkWatermark_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Watermark_DisplayPolicy : uint8
{
    Hidden,
    Regular,
    Detailed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Watermark_DisplayPolicy);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_Watermark_UserWidget_UE : public UCk_UserWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Watermark_UserWidget_UE);

public:
    UCk_Watermark_UserWidget_UE();

public:
    auto Request_SetDisplayPolicy(ECk_Watermark_DisplayPolicy InNewDisplayPolicy) -> void;

protected:
    UFUNCTION(BlueprintImplementableEvent,
              Category = "Ck|UI|Watermark")
    void OnDisplayPolicyChanged(
        ECk_Watermark_DisplayPolicy InPreviousDisplayPolicy,
        ECk_Watermark_DisplayPolicy InNewDisplayPolicy);

private:
    ECk_Watermark_DisplayPolicy _CurrentDisplayPolicy = ECk_Watermark_DisplayPolicy::Regular;
};

// --------------------------------------------------------------------------------------------------------------------
