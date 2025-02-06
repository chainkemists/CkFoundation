#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include <CoreMinimal.h>

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

class CKCORE_API SCk_MessageDialog : public SWindow
{
public:
    struct CKCORE_API FButton
    {
        CK_GENERATED_BODY(FButton);

    public:
        explicit
            FButton(
            const FText& InButtonText,
            const FSimpleDelegate& InOnClicked = FSimpleDelegate());

        auto
            SetOnClicked(
                FSimpleDelegate InOnClicked)
                -> FButton&;

    private:
        FText _ButtonText;
        FSimpleDelegate _OnClicked;
        ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;
        FLinearColor _Color = FLinearColor::Gray;
        bool _IsPrimary = false;
        bool _ShouldFocus = false;

    public:
        CK_PROPERTY_GET(_ButtonText);
        CK_PROPERTY_GET(_OnClicked);

        CK_PROPERTY(_EnableDisable);
        CK_PROPERTY(_Color);
        CK_PROPERTY(_IsPrimary);
        CK_PROPERTY(_ShouldFocus);
    };

    SLATE_BEGIN_ARGS(SCk_MessageDialog)
            : _AutoCloseOnButtonPress(true)
            , _Icon(nullptr)
            , _HAlignIcon(HAlign_Left)
            , _VAlignIcon(VAlign_Center)
            , _RootPadding(FMargin(4.f))
            , _ButtonAreaPadding(FMargin(20.f, 16.f, 0.f, 0.f))
            , _UseScrollBox(true)
            , _ScrollBoxMaxHeight(300)
            , _HAlignContent(HAlign_Left)
            , _VAlignContent(VAlign_Center)
        {
            _AccessibleParams = FAccessibleWidgetData(EAccessibleBehavior::Auto);
        }

        SLATE_ARGUMENT(FText, Title)
        SLATE_NAMED_SLOT(FArguments, Content)
        SLATE_ARGUMENT(TArray<FButton>, Buttons)
        SLATE_EVENT(FSimpleDelegate, OnClosed)
        SLATE_ARGUMENT(SWindow::FArguments, WindowArguments)
        SLATE_ARGUMENT(bool, AutoCloseOnButtonPress)

        SLATE_ATTRIBUTE(const FSlateBrush*, Icon)
        SLATE_ATTRIBUTE(TOptional<FVector2D>, IconDesiredSizeOverride)
        SLATE_ARGUMENT(EHorizontalAlignment, HAlignIcon)
        SLATE_ARGUMENT(EVerticalAlignment, VAlignIcon)
        SLATE_NAMED_SLOT(FArguments, BeforeButtons)
        SLATE_ARGUMENT(EHorizontalAlignment, HAlignButtonBox)
        SLATE_ARGUMENT(EVerticalAlignment, VAlignButtonBox)
        SLATE_ATTRIBUTE(FMargin, RootPadding)
        SLATE_ATTRIBUTE(FMargin, ButtonAreaPadding)
        SLATE_ATTRIBUTE(FMargin, ContentAreaPadding)
        SLATE_ARGUMENT(bool, UseScrollBox)
        SLATE_ARGUMENT(int32, ScrollBoxMaxHeight)
        SLATE_ARGUMENT(EHorizontalAlignment, HAlignContent)
        SLATE_ARGUMENT(EVerticalAlignment, VAlignContent)

    SLATE_END_ARGS()

    auto
        Construct(
            const FArguments& InArgs)
            -> void;

    auto
        Show()
            -> void;

    auto
        ShowModal()
            -> int32;

private:
    static auto
        CreateContentBox(
            const FArguments& InArgs)
            -> TSharedRef<SWidget>;

    auto
        CreateButtonBox(
            const FArguments& InArgs)
            -> TSharedRef<SWidget>;

    auto
        OnButtonClicked(
            FSimpleDelegate OnClicked,
            int32 ButtonIndex)
            -> FReply;
private:
    int32 _LastPressedButton = -1;
    FSimpleDelegate _OnClosed;
    bool _AutoCloseOnButtonPress = false;
};

// --------------------------------------------------------------------------------------------------------------------
