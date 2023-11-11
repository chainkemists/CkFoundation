#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkHUD_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_HUD_UserWidget_UE : public UCk_UserWidget_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_HUD_UserWidget_UE);
};

// --------------------------------------------------------------------------------------------------------------------
