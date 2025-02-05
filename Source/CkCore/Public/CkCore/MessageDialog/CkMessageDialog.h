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

        /*********** Functional ***********/

        /** Title to display for the dialog. */
        SLATE_ARGUMENT(FText, Title)

        /** The content to display above the button; icon is optionally created to the left of it.  */
        SLATE_NAMED_SLOT(FArguments, Content)

        /** The buttons that this dialog should have. One or more buttons must be added.*/
        SLATE_ARGUMENT(TArray<FButton>, Buttons)

        /** Event triggered when the dialog is closed, either because one of the buttons is pressed, or the windows is closed. */
        SLATE_EVENT(FSimpleDelegate, OnClosed)

        /** Provides default values for SWindow::FArguments not overriden by SCustomDialog. */
        SLATE_ARGUMENT(SWindow::FArguments, WindowArguments)

        /** Whether to automatically close this window when any button is pressed (default: true) */
        SLATE_ARGUMENT(bool, AutoCloseOnButtonPress)

        /*********** Cosmetic ***********/

        /** Optional icon to display (default: empty, translucent)*/
        SLATE_ATTRIBUTE(const FSlateBrush*, Icon)

        /** When specified, ignore the brushes size and report the DesiredSizeOverride as the desired image size (default: use icon size) */
        SLATE_ATTRIBUTE(TOptional<FVector2D>, IconDesiredSizeOverride)

        /** Alignment of icon (default: HAlign_Left)*/
        SLATE_ARGUMENT(EHorizontalAlignment, HAlignIcon)

        /** Alignment of icon (default: VAlign_Center) */
        SLATE_ARGUMENT(EVerticalAlignment, VAlignIcon)

        /** Custom widget placed before the buttons */
        SLATE_NAMED_SLOT(FArguments, BeforeButtons)

        /** HAlign to use for Button Box slot (default: HAlign_Left) */
        SLATE_ARGUMENT(EHorizontalAlignment, HAlignButtonBox)

        /** VAlign to use for Button Box  slot (default: VAlign_Center) */
        SLATE_ARGUMENT(EVerticalAlignment, VAlignButtonBox)

        /** Padding to apply to the widget embedded in the window, i.e. to all widgets contained in the window (default: {4,4,4,4} )*/
        SLATE_ATTRIBUTE(FMargin, RootPadding)

        /** Padding to apply around the layout holding the buttons (default: {20,16,0,0}) */
        SLATE_ATTRIBUTE(FMargin, ButtonAreaPadding)

        /** Padding to apply to DialogContent - you can use it to move away from the icon (default: {0,0,0,0}) */
        SLATE_ATTRIBUTE(FMargin, ContentAreaPadding)

        /** Should this dialog use a scroll box for over-sized content? (default: true) */
        SLATE_ARGUMENT(bool, UseScrollBox)

        /** Max height for the scroll box (default: 300) */
        SLATE_ARGUMENT(int32, ScrollBoxMaxHeight)

        /** HAlign to use for Content slot (default: HAlign_Left) */
        SLATE_ARGUMENT(EHorizontalAlignment, HAlignContent)

        /** VAlign to use for Content slot (default: VAlign_Center) */
        SLATE_ARGUMENT(EVerticalAlignment, VAlignContent)

        /********** Legacy - do not use **********/

        /** Optional icon to display in the dialog (default: none) */
        UE_DEPRECATED(5.1, "Use Icon() instead")
        CKCORE_API FArguments&
        IconBrush(
            FName InIconBrush);

        /** Content for the dialog (deprecated - use Content instead)*/
        SLATE_ARGUMENT_DEPRECATED(TSharedPtr<SWidget>, DialogContent, 5.1, "Use Content() instead.")

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
