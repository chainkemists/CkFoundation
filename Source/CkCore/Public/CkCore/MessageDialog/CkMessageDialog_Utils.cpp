#include "CkMessageDialog_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <Dialog/SCustomDialog.h>
#include <Misc/MessageDialog.h>

// --------------------------------------------------------------------------------------------------------------------



struct DialogButton_TEMP
{
    FText _Name;
    FSimpleDelegate _OnClicked;
    bool _IsPrimary = false;
    bool _ShouldFocus = false;
    FLinearColor _Color = FLinearColor::Gray;
};








#define CK_ENSURE_2(InExpression, InString, ...)                                                                                           \
[&]() -> bool                                                                                                                              \
{                                                                                                                                          \
    const auto ExpressionResult = InExpression;                                                                                            \
                                                                                                                                           \
    if (LIKELY(ExpressionResult))                                                                                                          \
    { return true; }                                                                                                                       \
                                                                                                                                           \
    UCk_Utils_Ensure_UE::Request_IncrementEnsureCountAtFileAndLine(__FILE__, __LINE__);                                                    \
                                                                                                                                           \
    if (UCk_Utils_Ensure_UE::Get_IsEnsureIgnored(__FILE__, __LINE__))                                                                      \
    { return false; }                                                                                                                      \
                                                                                                                                           \
    const auto IsMessageOnly = UCk_Utils_Core_UserSettings_UE::Get_EnsureDetailsPolicy() == ECk_EnsureDetails_Policy::MessageOnly;         \
                                                                                                                                           \
    const auto& Message = ck::Format_UE(InString, ##__VA_ARGS__);                                                                          \
    const auto& Title = ck::Format_UE(TEXT("Frame#[{}] PIE-ID[{}]"), GFrameCounter, GPlayInEditorID - 1);                                  \
    const auto& StackTraceWith2Skips = IsMessageOnly ?                                                                                     \
        TEXT("[StackTrace DISABLED]") :                                                                                                    \
        UCk_Utils_Debug_StackTrace_UE::Get_StackTrace(2);                                                                                  \
    const auto& BpStackTrace = IsMessageOnly ?                                                                                             \
        TEXT("[BP StackTrace DISABLED]") :                                                                                                 \
        UCk_Utils_Debug_StackTrace_UE::Get_StackTrace_Blueprint(ck::type_traits::AsString{});                                              \
    const auto& MessagePlusBpCallStack = ck::Format_UE(                                                                                    \
        TEXT("[{}] {}\n{}\n\n == BP CallStack ==\n{}"),                                                                                    \
        GPlayInEditorID - 1 < 0 ? TEXT("Server") : TEXT("Client"),                                                                         \
        TEXT(#InExpression),                                                                                                               \
        Message,                                                                                                                           \
        BpStackTrace);                                                                                                                     \
                                                                                                                                           \
    const auto& MessagePlusBpCallStackStr = FText::FromString(MessagePlusBpCallStack);                                                     \
                                                                                                                                           \
    if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::StreamerMode)                               \
    {                                                                                                                                      \
        ck::core::Error(TEXT("{}"), MessagePlusBpCallStack);                                                                               \
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(__FILE__, MessagePlusBpCallStackStr, __LINE__);                  \
        return false;                                                                                                                      \
    }                                                                                                                                      \
                                                                                                                                           \
    _DETAILS_CK_ENSURE_LOG_OR_PUSHMESSAGE("CkEnsures", MessagePlusBpCallStack, nullptr);                                                   \
                                                                                                                                           \
    if (UCk_Utils_Core_UserSettings_UE::Get_EnsureDisplayPolicy() == ECk_EnsureDisplay_Policy::MessageLog)                                 \
    {                                                                                                                                      \
        UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr, MessagePlusBpCallStackStr);                                              \
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(__FILE__, MessagePlusBpCallStackStr, __LINE__);                  \
        return false;                                                                                                                      \
    }                                                                                                                                      \
                                                                                                                                           \
    const auto& CallstackPlusMessage = ck::Format_UE(                                                                                      \
        TEXT("{}\nExpression: {}\nMessage: {}\n\n == BP CallStack ==\n{}\n == CallStack ==\n{}\n"),                                        \
        Title,                                                                                                                             \
        TEXT(#InExpression),                                                                                                               \
        Message,                                                                                                                           \
        BpStackTrace,                                                                                                                      \
        StackTraceWith2Skips);                                                                                                             \
    const auto& DialogMessage = FText::FromString(CallstackPlusMessage);                                                                   \
                                                                                                                                           \
    auto ReturnResult = false;                                                                                                             \
    using DialogButton = UCk_Utils_MessageDialog_UE::DialogButton;                                                                         \
    auto Buttons = TArray<DialogButton>{};                                                                                                 \
                                                                                                                                           \
    Buttons.Add(DialogButton{FText::FromString(TEXT("Ignore Once")), FSimpleDelegate::CreateLambda([&]()                                   \
    {                                                                                                                                      \
        ReturnResult = false;                                                                                                              \
    })}.Set_Color(FLinearColor{0.22f, 0.22f, 0.22f, 1.0f}));                                                                               \
                                                                                                                                           \
    Buttons.Add(DialogButton{FText::FromString(TEXT("Ignore All")), FSimpleDelegate::CreateLambda([&]()                                    \
    {                                                                                                                                      \
        UCk_Utils_Ensure_UE::Request_IgnoreEnsureAtFileAndLineWithMessage(__FILE__, DialogMessage, __LINE__);                              \
        ReturnResult = false;                                                                                                              \
    })}.Set_Color(FLinearColor{1.0f, 0.62f, 0.27f, 1.0f}).Set_IsPrimary(true).Set_ShouldFocus(true));                                      \
                                                                                                                                           \
                                                                                                                                           \
    Buttons.Add(DialogButton{FText::FromString(TEXT("Break")), {}} \
    .Set_Color(FLinearColor{0.22f, 0.22f, 0.22f, 1.0f})                    \
    .Set_EnableDisable(StackTraceWith2Skips.IsEmpty() ? ECk_EnableDisable::Disable : ECk_EnableDisable::Enable));                                  \
                                                                                                                                           \
    if (GIsEditor)                                                                                                                         \
    {                                                                                                                                      \
    Buttons.Add(DialogButton{FText::FromString(TEXT("Break in BP")), FSimpleDelegate::CreateLambda([&]()                                   \
    {                                                                                                                                      \
        UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr);                                                                         \
    })}.Set_Color(FLinearColor{0.34f, 0.34f, 0.59f, 1.0f})                                                                                 \
    .Set_EnableDisable(BpStackTrace.IsEmpty() ? ECk_EnableDisable::Disable : ECk_EnableDisable::Enable));                                  \
    }                                                                                                                                      \
                                                                                                                                           \
    if (GIsEditor)                                                                                                                         \
    {                                                                                                                                      \
        Buttons.Add(DialogButton{FText::FromString(TEXT("Abort PIE")), FSimpleDelegate::CreateLambda([&]()                                 \
        {                                                                                                                                  \
            UCk_Utils_Ensure_UE::Request_IgnoreAllEnsures();                                                                               \
            UCk_Utils_EditorOnly_UE::Request_AbortPIE();                                                                                   \
        })}.Set_Color(FLinearColor{1.0f, 0.1f, 0.1f, 1.0f}));                                                                              \
    }                                                                                                                                      \
                                                                                                                                           \
    auto ButtonIndex = UCk_Utils_MessageDialog_UE::CustomDialog(DialogMessage, FText::FromString(Title), Buttons);                         \
    if (ButtonIndex == 2)                                                                                                                  \
    {                                                                                                                                      \
        ReturnResult = ensureAlwaysMsgf(false, TEXT("[DEBUG BREAK HIT]"));                                                                 \
        if (NOT BpStackTrace.IsEmpty())                                                                                                    \
        {                                                                                                                                  \
            UCk_Utils_Debug_StackTrace_UE::Try_BreakInScript(nullptr);                                                                     \
        }                                                                                                                                  \
    }                                                                                                                                      \
    return ReturnResult;                                                                                                                   \
}()


















// --------------------------------------------------------------------------------------------------------------------























#include "Framework/Application/SlateApplication.h"
#include "Framework/Docking/TabManager.h"
#include "Styling/AppStyle.h"
#include "Styling/SlateBrush.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SNullWidget.h"

SCk_MessageDialog::FArguments& SCk_MessageDialog::FArguments::IconBrush(FName InIconBrush)
{
    const FSlateBrush* ImageBrush = FAppStyle::Get().GetBrush(InIconBrush);
    if (ensureMsgf(ImageBrush != nullptr, TEXT("Brush %s is unknown"), *InIconBrush.ToString()))
    {
        _Icon = ImageBrush;
    }
    return *this;
}

auto
    SCk_MessageDialog::Construct(
        const FArguments& InArgs)
        -> void
{
    check(InArgs._Buttons.Num() > 0);

    OnClosed = InArgs._OnClosed;
    bAutoCloseOnButtonPress = InArgs._AutoCloseOnButtonPress;

    SWindow::Construct( SWindow::FArguments(InArgs._WindowArguments)
        .Title(InArgs._Title)
        .SizingRule(ESizingRule::Autosized)
        .SupportsMaximize(false)
        .SupportsMinimize(false)
        [
            SNew(SBorder)
            .Padding(InArgs._RootPadding)
            .BorderImage(FAppStyle::Get().GetBrush( "ToolPanel.GroupBorder" ))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    CreateContentBox(InArgs)
                ]

                + SVerticalBox::Slot()
                .VAlign(InArgs._VAlignButtonBox)
                .HAlign(InArgs._HAlignButtonBox)
                .AutoHeight()
                .Padding(InArgs._ButtonAreaPadding)
                [
                    CreateButtonBox(InArgs)
                ]
            ]
        ] );
}

int32 SCk_MessageDialog::ShowModal()
{
    FSlateApplication::Get().AddModalWindow(StaticCastSharedRef<SWindow>(this->AsShared()), FGlobalTabmanager::Get()->GetRootWindow());
    return LastPressedButton;
}

void SCk_MessageDialog::Show()
{
    const TSharedRef<SWindow> Window = FSlateApplication::Get().AddWindow(StaticCastSharedRef<SWindow>(this->AsShared()), true);
    if (OnClosed.IsBound())
    {
        Window->GetOnWindowClosedEvent().AddLambda([this](const TSharedRef<SWindow>&) { OnClosed.Execute(); });
    }
}

TSharedRef<SWidget> SCk_MessageDialog::CreateContentBox(const FArguments& InArgs)
{
    TSharedRef<SHorizontalBox> ContentBox = SNew(SHorizontalBox);

    ContentBox->AddSlot()
        .AutoWidth()
        .VAlign(InArgs._VAlignIcon)
        .HAlign(InArgs._HAlignIcon)
        [
            SNew(SBox)
            .Padding(0, 0, 8, 0)
            .Visibility_Lambda([IconAttr = InArgs._Icon]() { return IconAttr.Get(nullptr) ? EVisibility::Visible : EVisibility::Collapsed; })
            [
                SNew(SImage)
                .DesiredSizeOverride(InArgs._IconDesiredSizeOverride)
                .Image(InArgs._Icon)
            ]
        ];

    if (InArgs._UseScrollBox)
    {
        ContentBox->AddSlot()
        .VAlign(InArgs._VAlignContent)
        .HAlign(InArgs._HAlignContent)
        .Padding(InArgs._ContentAreaPadding)
        [
            SNew(SBox)
            .MaxDesiredHeight(InArgs._ScrollBoxMaxHeight)
            [
                SNew(SScrollBox)
                + SScrollBox::Slot()
                [
                    InArgs._Content.Widget
                ]
            ]
        ];
    }
    else
    {
        ContentBox->AddSlot()
            .FillWidth(1.0f)
            .VAlign(InArgs._VAlignContent)
            .HAlign(InArgs._HAlignContent)
            .Padding(InArgs._ContentAreaPadding)
            [
                InArgs._Content.Widget
            ];
    }

    return ContentBox;
}

TSharedRef<SWidget> SCk_MessageDialog::CreateButtonBox(const FArguments& InArgs)
{
    TSharedPtr<SHorizontalBox> ButtonPanel;
    TSharedRef<SHorizontalBox> ButtonBox =
        SNew(SHorizontalBox)

        // Before buttons
        + SHorizontalBox::Slot()
            .AutoWidth()
            .HAlign(HAlign_Left)
            .VAlign(VAlign_Center)
            [
                InArgs._BeforeButtons.Widget
            ]

        // Buttons
        + SHorizontalBox::Slot()
            .FillWidth(1.0f)
            .VAlign(VAlign_Center)
            .HAlign(HAlign_Right)
            [
                SAssignNew(ButtonPanel, SHorizontalBox)
            ];

    auto CanFocusLastPrimary = true;
    auto FocusedAnyButtonYet = false;

    for (int32 ButtonIndex = 0; ButtonIndex < InArgs._Buttons.Num(); ++ButtonIndex)
    {
        const FButton& Button = InArgs._Buttons[ButtonIndex];

        auto ButtonWidget = [&]()
        {
            if (Button.Get_IsPrimary())
            {
                const auto ButtonStyle = &FAppStyle::Get().GetWidgetStyle< FButtonStyle >("PrimaryButton");

                auto Widget = SNew(SButton)
                    .OnClicked(FOnClicked::CreateSP(this, &SCk_MessageDialog::OnButtonClicked, Button.Get_OnClicked(), ButtonIndex))
                    .ButtonStyle(ButtonStyle)
                    [
                        SNew(STextBlock)
                        .Text(Button.Get_ButtonText())
                    ];
                return Widget;
            }
            else
            {
                const FButtonStyle* ButtonStyle = &FAppStyle::Get().GetWidgetStyle< FButtonStyle >("Button");

                const auto CustomStyle = new FButtonStyle(*ButtonStyle);
                CustomStyle->Normal.TintColor = Button.Get_Color();
                CustomStyle->Hovered.TintColor = Button.Get_Color() * 1.2f;
                CustomStyle->Pressed.TintColor = Button.Get_Color() * 0.8f;

                auto Widget = SNew(SButton)
                    .OnClicked(FOnClicked::CreateSP(this, &SCk_MessageDialog::OnButtonClicked, Button.Get_OnClicked(), ButtonIndex))
                    .ButtonStyle(CustomStyle)
                    .ButtonColorAndOpacity(Button.Get_Color())
                    .IsEnabled(Button.Get_EnableDisable() == ECk_EnableDisable::Enable)
                    [
                        SNew(STextBlock)
                        .Text(Button.Get_ButtonText())
                    ];
                return Widget;
            }
        }();

        ButtonPanel->AddSlot()
            .Padding(FAppStyle::Get().GetMargin("StandardDialog.SlotPadding"))
            .AutoWidth()
        [
            ButtonWidget
        ];

        if (Button.Get_ShouldFocus())
        {
            CanFocusLastPrimary = false;
        }

        const bool IsLastButton = ButtonIndex == InArgs._Buttons.Num() - 1;
        if (Button.Get_ShouldFocus() ||
             (CanFocusLastPrimary && Button.Get_IsPrimary()) ||
             (IsLastButton && NOT FocusedAnyButtonYet))
        {
            FocusedAnyButtonYet = true;
            SetWidgetToFocusOnActivate(ButtonWidget);
        }

    }
    return ButtonBox;
}

/** Handle the button being clicked */
FReply SCk_MessageDialog::OnButtonClicked(FSimpleDelegate OnClicked, int32 ButtonIndex)
{
    LastPressedButton = ButtonIndex;

    if (bAutoCloseOnButtonPress)
    {
        RequestDestroyWindow();
    }

    OnClicked.ExecuteIfBound();
    return FReply::Handled();
}

































// --------------------------------------------------------------------------------------------------------------------

namespace ck_message_dialog
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 3
    auto
        DoGet_TitleOrNullptr(
            const FText& InTitle)
        -> const FText*
    {
        return InTitle.IsEmpty() ? nullptr : &InTitle;
    }
#else
    auto
        DoGet_TitleOrNullptr(
            const FText& InTitle)
        -> FText
    {
        return InTitle;
    }
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MessageDialog_UE::
    Ok(
        FText InMessage,
        FText InTitle)
    -> void
{
    FMessageDialog::Open(
        EAppMsgType::Ok,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle));
}

auto
    UCk_Utils_MessageDialog_UE::
    YesNo(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_YesNo
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::YesNo,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Cancel:
        case EAppReturnType::No:
        {
            return ECk_MessageDialog_YesNo::No;
        }
        case EAppReturnType::Yes:
        {
            return ECk_MessageDialog_YesNo::Yes;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_YesNo::No;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    OkCancel(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_OkCancel
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::OkCancel,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Ok:
        {
            return ECk_MessageDialog_OkCancel::Okay;
        }
        case EAppReturnType::Cancel:
        {
            return ECk_MessageDialog_OkCancel::Cancel;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_OkCancel::Cancel;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    YesNoCancel(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_YesNoCancel
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::YesNoCancel,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Yes:
        {
            return ECk_MessageDialog_YesNoCancel::Yes;
        }
        case EAppReturnType::No:
        {
            return ECk_MessageDialog_YesNoCancel::No;
        }
        case EAppReturnType::Cancel:
        {
            return ECk_MessageDialog_YesNoCancel::Cancel;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_YesNoCancel::Cancel;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    CancelRetryContinue(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_CancelRetryContinue
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::CancelRetryContinue,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Cancel:
        {
            return ECk_MessageDialog_CancelRetryContinue::Cancel;
        }
        case EAppReturnType::Retry:
        {
            return ECk_MessageDialog_CancelRetryContinue::Retry;
        }
        case EAppReturnType::Continue:
        {
            return ECk_MessageDialog_CancelRetryContinue::Continue;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_CancelRetryContinue::Cancel;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    YesNoYesAllNoAll(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_YesNoYesAllNoAll
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::YesNoYesAllNoAll,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Yes:
        {
            return ECk_MessageDialog_YesNoYesAllNoAll::Yes;
        }
        case EAppReturnType::Cancel:
        case EAppReturnType::No:
        {
            return ECk_MessageDialog_YesNoYesAllNoAll::No;
        }
        case EAppReturnType::YesAll:
        {
            return ECk_MessageDialog_YesNoYesAllNoAll::YesAll;
        }
        case EAppReturnType::NoAll:
        {
            return ECk_MessageDialog_YesNoYesAllNoAll::NoAll;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_YesNoYesAllNoAll::NoAll;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    YesNoYesAllNoAllCancel(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_YesNoYesAllNoAllCancel
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::YesNoYesAllNoAllCancel,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Yes:
        {
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::Yes;
        }
        case EAppReturnType::No:
        {
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::No;
        }
        case EAppReturnType::YesAll:
        {
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::YesAll;
        }
        case EAppReturnType::NoAll:
        {
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::NoAll;
        }
        case EAppReturnType::Cancel:
        {
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::Cancel;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::NoAll;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    YesNoYesAll(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_YesNoYesAll
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::YesNoYesAll,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Yes:
        {
            return ECk_MessageDialog_YesNoYesAll::Yes;
        }
        case EAppReturnType::Cancel:
        case EAppReturnType::No:
        {
            return ECk_MessageDialog_YesNoYesAll::No;
        }
        case EAppReturnType::YesAll:
        {
            return ECk_MessageDialog_YesNoYesAll::YesAll;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_YesNoYesAll::No;
        }
    }
}

void
    UCk_Utils_MessageDialog_UE::FORCE_FIRE_ENSURE()
{
    CK_ENSURE(false, TEXT("FORCE_FIRE_ENSURE"));
}

auto
    UCk_Utils_MessageDialog_UE::
    CustomDialog(
        const FText& InMessage,
        const FText& InTitle,
        const TArray<DialogButton> InButtons)
    -> int32
{
    CK_ENSURE_IF_NOT(NOT InButtons.IsEmpty(), TEXT("Cannot create Custom Dialog with no buttons"))
    { return INDEX_NONE; }

    const auto Dialog = SNew(SCk_MessageDialog)
        .Title(InTitle)
        .Content()
        [
            SNew(SBox)
            .WidthOverride(1000.0f)
            .HeightOverride(1000.0f)
            [
                SNew(STextBlock).Text(InMessage).AutoWrapText(true)
            ]
        ]
        .BeforeButtons()
        [
            SNew(SButton)
            .ButtonStyle(FAppStyle::Get(), "SimpleButton")
            .OnClicked_Lambda([InMessage]()
            {
                FPlatformApplicationMisc::ClipboardCopy(*InMessage.ToString());
                return FReply::Handled();
            })
            .ToolTipText(NSLOCTEXT("SChoiceDialog", "CopyMessageTooltip", "Copy the text in this message to the clipboard (CTRL+C)"))
            .Content()
            [
                SNew(SImage)
                .Image(FAppStyle::Get().GetBrush("Icons.Clipboard"))
                .ColorAndOpacity(FSlateColor::UseForeground())
            ]
        ]
        .Buttons(InButtons);

    return Dialog->ShowModal();
}

// --------------------------------------------------------------------------------------------------------------------
