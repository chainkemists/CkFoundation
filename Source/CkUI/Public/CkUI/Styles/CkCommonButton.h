#pragma once

#include <CoreMinimal.h>
#include <CommonButtonBase.h>
#include <CommonTextBlock.h>

#include "CkCommonButton.generated.h"

UCLASS(Abstract, BlueprintType, Blueprintable)
class CKUI_API UCkCommonButton : public UCommonButtonBase
{
    GENERATED_BODY()

public:
    UCkCommonButton();

    UFUNCTION(BlueprintCallable)
    void Request_SetText(const FText& InText);

    UFUNCTION(BlueprintCallable)
    FText Get_ButtonText() const;

    UFUNCTION(BlueprintCallable)
    void Request_SetTextStyle(TSubclassOf<UCommonTextStyle> InTextStyle);

    UFUNCTION(BlueprintCallable)
    UCommonTextBlock* Get_TextBlock() const;

protected:
    void NativePreConstruct() override;
    void NativeOnInitialized() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Text")
    FText ButtonText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Button Text")
    TSubclassOf<UCommonTextStyle> TextStyleClass;

    UPROPERTY(BlueprintReadOnly, Category = "Button Text")
    TObjectPtr<UCommonTextBlock> TextBlock;

    virtual TSubclassOf<UCommonTextStyle> Get_DefaultTextStyle() const;
    virtual void Request_ApplyDefaultTextStyle();
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="CK Primary Button"))
class CKUI_API UCkCommonButton_Primary : public UCkCommonButton
{
    GENERATED_BODY()

public:
    UCkCommonButton_Primary();

protected:
    TSubclassOf<UCommonTextStyle> Get_DefaultTextStyle() const override;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="CK Secondary Button"))
class CKUI_API UCkCommonButton_Secondary : public UCkCommonButton
{
    GENERATED_BODY()

public:
    UCkCommonButton_Secondary();

protected:
    TSubclassOf<UCommonTextStyle> Get_DefaultTextStyle() const override;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="CK Danger Button"))
class CKUI_API UCkCommonButton_Danger : public UCkCommonButton
{
    GENERATED_BODY()

public:
    UCkCommonButton_Danger();

protected:
    TSubclassOf<UCommonTextStyle> Get_DefaultTextStyle() const override;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="CK Tab Button"))
class CKUI_API UCkCommonButton_Tab : public UCkCommonButton
{
    GENERATED_BODY()

public:
    UCkCommonButton_Tab();

protected:
    TSubclassOf<UCommonTextStyle> Get_DefaultTextStyle() const override;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType, Blueprintable, meta=(DisplayName="CK Dropdown Button"))
class CKUI_API UCkCommonButton_Dropdown : public UCkCommonButton
{
    GENERATED_BODY()

public:
    UCkCommonButton_Dropdown();

protected:
    TSubclassOf<UCommonTextStyle> Get_DefaultTextStyle() const override;
};

// --------------------------------------------------------------------------------------------------------------------

