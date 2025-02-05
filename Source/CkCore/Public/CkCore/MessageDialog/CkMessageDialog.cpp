#include "CkMessageDialog.h"

#include "CkCore/Ensure/CkEnsure.h"

#include <Framework/Application/SlateApplication.h>
#include <Framework/Docking/TabManager.h>

#include <Misc/MessageDialog.h>

#include <Styling/AppStyle.h>
#include <Styling/SlateBrush.h>

#include <Widgets/SBoxPanel.h>
#include <Widgets/SNullWidget.h>
#include <Widgets/Images/SImage.h>
#include <Widgets/Input/SButton.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/Layout/SScrollBox.h>
#include <Widgets/Text/STextBlock.h>

// --------------------------------------------------------------------------------------------------------------------

SCk_MessageDialog::FButton::
FButton(
    const FText& InButtonText,
    const FSimpleDelegate& InOnClicked)
    : _ButtonText(InButtonText)
    , _OnClicked(InOnClicked) {}

auto
    SCk_MessageDialog::FButton::
    SetOnClicked(
        FSimpleDelegate InOnClicked)
        -> FButton&
{
    _OnClicked = MoveTemp(InOnClicked);
    return *this;
}

auto
    SCk_MessageDialog::FArguments::
    IconBrush(
        FName InIconBrush)
        -> FArguments&
{
    auto ImageBrush = FAppStyle::Get().GetBrush(InIconBrush);

    CK_ENSURE_IF_NOT(ck::IsValid(ImageBrush, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Brush {} is unknown"), InIconBrush.ToString())
    {
        return *this;
    }

    _Icon = ImageBrush;
    return *this;
}

auto
    SCk_MessageDialog::
    Construct(
        const FArguments& InArgs)
        -> void
{
    CK_ENSURE_IF_NOT(InArgs._Buttons.Num() > 0,
        TEXT("Expected at least one button to be added to the dialog.{}"), ck::Context(this))
    {
        return;
    }

    _OnClosed = InArgs._OnClosed;
    _AutoCloseOnButtonPress = InArgs._AutoCloseOnButtonPress;

    SWindow::Construct(SWindow::FArguments(InArgs._WindowArguments)
                       .Title(InArgs._Title)
                       .SizingRule(ESizingRule::Autosized)
                       .SupportsMaximize(false)
                       .SupportsMinimize(false)
        [
            SNew(SBorder)
            .Padding(InArgs._RootPadding)
            .BorderImage(FAppStyle::Get().GetBrush("ToolPanel.GroupBorder"))
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
        ]);
}

auto
    SCk_MessageDialog::
    ShowModal()
        -> int32
{
    FSlateApplication::Get().AddModalWindow(StaticCastSharedRef<SWindow>(this->AsShared()),
        FGlobalTabmanager::Get()->GetRootWindow());
    return _LastPressedButton;
}

auto
    SCk_MessageDialog::
    Show()
        -> void
{
    const auto Window = FSlateApplication::Get().AddWindow(
        StaticCastSharedRef<SWindow>(this->AsShared()), true);
    if (NOT _OnClosed.IsBound())
    {
        return;
    }

    Window->GetOnWindowClosedEvent().AddLambda([this](
        const TSharedRef<SWindow>&)
        {
            _OnClosed.Execute();
        });
}

auto
    SCk_MessageDialog::
    CreateContentBox(
        const FArguments& InArgs)
        -> TSharedRef<SWidget>
{
    auto ContentBox = SNew(SHorizontalBox);

    ContentBox->AddSlot()
              .AutoWidth()
              .VAlign(InArgs._VAlignIcon)
              .HAlign(InArgs._HAlignIcon)
    [
        SNew(SBox)
        .Padding(0, 0, 8, 0)
        .Visibility_Lambda([IconAttr = InArgs._Icon]()
        {
            return IconAttr.Get(nullptr) ? EVisibility::Visible : EVisibility::Collapsed;
        })
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
                    InArgs
                    ._Content
                    .Widget
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

auto
    SCk_MessageDialog::
    CreateButtonBox(
        const FArguments& InArgs)
        -> TSharedRef<SWidget>
{
    auto ButtonPanel = TSharedPtr<SHorizontalBox>{};
    auto ButtonBox =
        SNew(SHorizontalBox)

        // Before buttons
        + SHorizontalBox::Slot()
        .AutoWidth()
        .HAlign(HAlign_Left)
        .VAlign(VAlign_Center)
        [
            InArgs
            ._BeforeButtons
            .Widget
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
        const auto& Button = InArgs._Buttons[ButtonIndex];

        auto ButtonWidget = [&]()
        {
            if (Button.Get_IsPrimary())
            {
                const auto ButtonStyle = &FAppStyle::Get().GetWidgetStyle<FButtonStyle>("PrimaryButton");

                auto Widget = SNew(SButton)
                    .OnClicked(FOnClicked::CreateSP(this, &SCk_MessageDialog::OnButtonClicked, Button.Get_OnClicked(),
                        ButtonIndex))
                    .ButtonStyle(ButtonStyle)
                    [
                        SNew(STextBlock)
                        .Text(Button.Get_ButtonText())
                    ];
                return Widget;
            }
            else
            {
                const FButtonStyle* ButtonStyle = &FAppStyle::Get().GetWidgetStyle<FButtonStyle>("Button");

                const auto CustomStyle = new FButtonStyle(*ButtonStyle);
                CustomStyle->Normal.TintColor = Button.Get_Color();
                CustomStyle->Hovered.TintColor = Button.Get_Color() * 1.2f;
                CustomStyle->Pressed.TintColor = Button.Get_Color() * 0.8f;

                auto Widget = SNew(SButton)
                    .OnClicked(FOnClicked::CreateSP(this, &SCk_MessageDialog::OnButtonClicked, Button.Get_OnClicked(),
                        ButtonIndex))
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
auto
    SCk_MessageDialog::
    OnButtonClicked(
        // ReSharper disable once CppPassValueParameterByConstReference
        FSimpleDelegate OnClicked,
        const int32 ButtonIndex)
        -> FReply
{
    _LastPressedButton = ButtonIndex;

    if (_AutoCloseOnButtonPress)
    {
        RequestDestroyWindow();
    }

    std::ignore = OnClicked.ExecuteIfBound();
    return FReply::Handled();
}

// --------------------------------------------------------------------------------------------------------------------
