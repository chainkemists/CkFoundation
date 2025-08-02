// Inspired by https://github.com/suramaru517/ScreenFade

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkScreenFade_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DELEGATE(FCk_Delegate_OnScreenFadeFinished);
DECLARE_DYNAMIC_DELEGATE(FCk_Delegate_OnScreenFadeFinished_Dynamic);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FCk_ScreenFade_Params
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ScreenFade_Params);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
    float _FadeTime = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
    FLinearColor _FromColor = FLinearColor::Transparent;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
    FLinearColor _ToColor = FLinearColor::Transparent;

    FCk_Delegate_OnScreenFadeFinished _OnFinished;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "On Finished", meta = (AllowPrivateAccess))
    FCk_Delegate_OnScreenFadeFinished_Dynamic _OnFinishedDynamic;

    UPROPERTY(AdvancedDisplay, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
    bool _FadeAudio = false;

    UPROPERTY(AdvancedDisplay, EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess))
    bool _FadeWhenPaused = false;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ScreenFade_Params, _FadeTime);

    CK_PROPERTY_GET(_FadeTime);
    CK_PROPERTY(_FromColor);
    CK_PROPERTY(_ToColor);
    CK_PROPERTY(_OnFinished);
    CK_PROPERTY(_OnFinishedDynamic);
    CK_PROPERTY(_FadeAudio);
    CK_PROPERTY(_FadeWhenPaused);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKUI_API UCk_ScreenFade_Utils : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
        Category = "Ck|Utils|ScreenFade",
        DisplayName = "[Ck] Request Screen Fade",
        meta = (AdvancedDisplay = 2))
    static void
    Request_ScreenFade(
        const APlayerController* InOwningPlayer,
        const FCk_ScreenFade_Params& InFadeParams,
        int32 InZOrder = 100);

    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
        Category = "Ck|Utils|ScreenFade",
        DisplayName = "[Ck] Request Screen Fade In",
        meta = (InFadeTime = "1.0", InFromColor = "(R=0.0,G=0.0,B=0.0,A=1.0)", AdvancedDisplay = 3))
    static void
    Request_ScreenFadeIn(
        const APlayerController* InOwningPlayer,
        float InFadeTime,
        FLinearColor InFromColor,
        FCk_Delegate_OnScreenFadeFinished_Dynamic OnFinished,
        bool InFadeAudio,
        bool InFadeWhenPaused,
        int32 InZOrder = 100);

    UFUNCTION(BlueprintCallable, BlueprintCosmetic,
        Category = "Ck|Utils|ScreenFade",
        DisplayName = "[Ck] Request Screen Fade Out",
        meta = (InFadeTime = "1.0", InToColor = "(R=0.0,G=0.0,B=0.0,A=1.0)", AdvancedDisplay = 3))
    static void
    Request_ScreenFadeOut(
        const APlayerController* InOwningPlayer,
        float InFadeTime,
        FLinearColor InToColor,
        FCk_Delegate_OnScreenFadeFinished_Dynamic OnFinished,
        bool InFadeAudio,
        bool InFadeWhenPaused,
        int32 InZOrder = 100);
};

// --------------------------------------------------------------------------------------------------------------------
