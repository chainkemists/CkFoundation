#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include <Widgets/CommonActivatableWidgetContainer.h>

#include "CkCore/Time/CkTime.h"
#include "CkUI/UserWidget/CkUserWidget.h"

#include "CkStack_Widget.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable, meta = (DisableNativeTick))
class CKUI_API UCk_Stack_UserWidget_UE : public UCommonActivatableWidgetStack
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Stack_UserWidget_UE);

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|UI|StackWidget")
    UCk_UserWidget_UE* PushWidgetInstance(
            UCk_UserWidget_UE* InWidgetInstance);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|StackWidget")
    bool ContainsWidgetInstance(
            UCk_UserWidget_UE* InWidgetInstance);

    UFUNCTION(BlueprintCallable, Category = "Ck|UI|StackWidget")
    void SetTransitionDetails(
        ECommonSwitcherTransition InTransitionType,
        ETransitionCurve InTransitionCurveType,
        FCk_Time InTransitionDuration,
        ECommonSwitcherTransitionFallbackStrategy InTransitionFallbackStrategy = ECommonSwitcherTransitionFallbackStrategy::None);

protected:
#if WITH_EDITOR
    auto GetPaletteCategory() -> const FText override;
#endif
};

// --------------------------------------------------------------------------------------------------------------------
