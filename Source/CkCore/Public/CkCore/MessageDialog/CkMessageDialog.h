#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CKMessageDialog.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MessageDialog_YesNo : uint8
{
    Yes,
    No
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MessageDialog_OkCancel : uint8
{
    Okay,
    Cancel
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MessageDialog_YesNoCancel : uint8
{
    Yes,
    No,
    Cancel
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MessageDialog_CancelRetryContinue : uint8
{
    Cancel,
    Retry,
    Continue
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MessageDialog_YesNoYesAllNoAll : uint8
{
    Yes,
    No,
    YesAll,
    NoAll
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MessageDialog_YesNoYesAllNoAllCancel : uint8
{
    Yes,
    No,
    YesAll,
    NoAll,
    Cancel
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MessageDialog_YesNoYesAll : uint8
{
    Yes,
    No,
    YesAll
};

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_MessageDialog_Types : uint8
{
    Ok,
    YesNo,
    OkCancel,
    YesNoCancel,
    CancelRetryContinue,
    YesNoYesAllNoAll,
    YesNoYesAllNoAllCancel,
    YesNoYesAll
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKCORE_API UCk_Utils_MessageDialog_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_MessageDialog_UE);

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
};

// --------------------------------------------------------------------------------------------------------------------
