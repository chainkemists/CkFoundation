#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/MessageDialog/CkMessageDialog.h"

#include <CoreMinimal.h>

#include "CKMessageDialog_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCORE_API UCk_Utils_MessageDialog_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_MessageDialog_UE);

public:
    using DialogButton = SCk_MessageDialog::FButton;

public:
    UFUNCTION(BlueprintCallable,
        DisplayName = "[Ck] Open Message Dialog (Ok)",
        Category = "Ck|Utils|MessageDialog")
    static void
    Ok(
        FText InMessage,
        FText InTitle = FText::GetEmpty());

    UFUNCTION(BlueprintCallable,
        DisplayName = "[Ck] Open Message Dialog (Yes/No)",
        Category = "Ck|Utils|MessageDialog")
    static ECk_MessageDialog_YesNo
    YesNo(
        FText InMessage,
        FText InTitle = FText::GetEmpty());

    UFUNCTION(BlueprintCallable,
        DisplayName = "[Ck] Open Message Dialog (Ok/Cancel)",
        Category = "Ck|Utils|MessageDialog")
    static ECk_MessageDialog_OkCancel
    OkCancel(
        FText InMessage,
        FText InTitle = FText::GetEmpty());

    UFUNCTION(BlueprintCallable,
        DisplayName = "[Ck] Open Message Dialog (Yes/No/Cancel)",
        Category = "Ck|Utils|MessageDialog")
    static ECk_MessageDialog_YesNoCancel
    YesNoCancel(
        FText InMessage,
        FText InTitle = FText::GetEmpty());

    UFUNCTION(BlueprintCallable,
        DisplayName = "[Ck] Open Message Dialog (Cancel/Retry/Continue)",
        Category = "Ck|Utils|MessageDialog")
    static ECk_MessageDialog_CancelRetryContinue
    CancelRetryContinue(
        FText InMessage,
        FText InTitle = FText::GetEmpty());

    UFUNCTION(BlueprintCallable,
        DisplayName = "[Ck] Open Message Dialog (Yes/No/YesAll/NoAll)",
        Category = "Ck|Utils|MessageDialog")
    static ECk_MessageDialog_YesNoYesAllNoAll
    YesNoYesAllNoAll(
        FText InMessage,
        FText InTitle = FText::GetEmpty());

    UFUNCTION(BlueprintCallable,
        DisplayName = "[Ck] Open Message Dialog (Yes/No/YesAll/NoAll/Cancel)",
        Category = "Ck|Utils|MessageDialog")
    static ECk_MessageDialog_YesNoYesAllNoAllCancel
    YesNoYesAllNoAllCancel(
        FText InMessage,
        FText InTitle = FText::GetEmpty());

    UFUNCTION(BlueprintCallable,
        DisplayName = "[Ck] Open Message Dialog (Yes/No/YesAll)",
        Category = "Ck|Utils|MessageDialog")
    static ECk_MessageDialog_YesNoYesAll
    YesNoYesAll(
        FText InMessage,
        FText InTitle = FText::GetEmpty());

public:
    static int32
    CustomDialog(
        const FText& InMessage,
        const FText& InTitle,
        const TArray<DialogButton>& InButtons);

private:
    static auto Get_EnsureRichTextStyleSet() -> TSharedPtr<FSlateStyleSet>;
    static TSharedPtr<FSlateStyleSet> EnsureRichTextStyleSet;
};

// --------------------------------------------------------------------------------------------------------------------
