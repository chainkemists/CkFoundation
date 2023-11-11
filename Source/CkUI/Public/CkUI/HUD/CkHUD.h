#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <GameFramework/HUD.h>

#include "CkHUD.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType)
class CKUI_API ACk_HUD_UE : public AHUD
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_HUD_UE);

public:
    ACk_HUD_UE();

private:
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
              Category = "ACk_HUD_UE",
              meta = (AllowPrivateAccess = true))
    TSoftClassPtr<class UCk_HUD_UserWidget_UE> _HUD_WidgetClass;

    UPROPERTY(Transient, BlueprintReadWrite,
              Category = "ACk_HUD_UE",
              meta = (AllowPrivateAccess = true))
    TObjectPtr<class UCk_HUD_UserWidget_UE> _HUD_WidgetInstance;

public:
    CK_PROPERTY_GET(_HUD_WidgetClass);
    CK_PROPERTY_GET(_HUD_WidgetInstance);
};

// --------------------------------------------------------------------------------------------------------------------
