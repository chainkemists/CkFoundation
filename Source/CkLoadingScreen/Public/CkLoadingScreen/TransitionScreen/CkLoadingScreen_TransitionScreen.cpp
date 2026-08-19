#include "CkLoadingScreen_TransitionScreen.h"

#include "CkCore/Macros/CkMacros.h"

#include <Slate/DeferredCleanupSlateBrush.h>
#include <Styling/CoreStyle.h>
#include <Widgets/Images/SImage.h>
#include <Widgets/Layout/SBorder.h>
#include <Widgets/Layout/SScaleBox.h>
#include <Widgets/SOverlay.h>
#include <Widgets/Text/STextBlock.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_loading_screen_transition_screen
{
    constexpr auto TwoPi = 6.28318530718f;
    constexpr auto TipBottomPadding = 64.0f;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    SCk_LoadingScreen_Transition::
    Construct(
        const FArguments& InArgs)
    -> void
{
    _LogoBrush = InArgs._LogoBrush;
    _BackdropBrush = InArgs._BackdropBrush;
    _ThrobPeriod = InArgs._ThrobPeriod;
    _ThrobMinOpacity = FMath::Clamp(InArgs._ThrobMinOpacity, 0.0f, 1.0f);

    const auto Overlay = SNew(SOverlay);

    // The opaque plate. With no backdrop configured this IS the background, which is precisely
    // BusterBlock's shipped look (black + corner logo) - so the retired plugin's screen survives
    // its own deletion unchanged.
    Overlay->AddSlot()
    [
        SNew(SBorder)
        .HAlign(HAlign_Fill)
        .VAlign(VAlign_Fill)
        .BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
        .BorderBackgroundColor(InArgs._BackgroundTint)
    ];

    if (_BackdropBrush.IsValid())
    {
        Overlay->AddSlot()
        [
            SNew(SScaleBox)
            .Stretch(EStretch::ScaleToFill)
            [
                SNew(SImage)
                .Image(_BackdropBrush->GetSlateBrush())
            ]
        ];
    }

    if (_LogoBrush.IsValid())
    {
        Overlay->AddSlot()
        .HAlign(HAlign_Right)
        .VAlign(VAlign_Bottom)
        .Padding(FMargin{0.0f, 0.0f,
            static_cast<float>(-InArgs._LogoOffset.X),
            static_cast<float>(-InArgs._LogoOffset.Y)})
        [
            SAssignNew(_LogoImage, SImage)
            .Image(_LogoBrush->GetSlateBrush())
        ];
    }

    if (NOT InArgs._TipText.IsEmpty())
    {
        Overlay->AddSlot()
        .HAlign(HAlign_Center)
        .VAlign(VAlign_Bottom)
        .Padding(FMargin{0.0f, 0.0f, 0.0f, ck_loading_screen_transition_screen::TipBottomPadding})
        [
            SNew(STextBlock)
            .Text(InArgs._TipText)
            .Font(FCoreStyle::GetDefaultFontStyle("Regular", 20.0f))
            .ColorAndOpacity(FSlateColor{FLinearColor::White})
        ];
    }

    ChildSlot
    [
        Overlay
    ];
}

auto
    SCk_LoadingScreen_Transition::
    OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const
    -> int32
{
    // The paint pass is the clock. Nothing else ticks out here.
    _ElapsedSeconds += Args.GetDeltaTime();

    if (_LogoImage.IsValid() && _ThrobPeriod > 0.0f)
    {
        // Cosine so the cycle opens at full opacity: 1 -> min -> 1 over the period. Kept
        // numerically identical to the game-thread widget's throb so the handoff is invisible.
        const auto Phase = 0.5f * (1.0f + FMath::Cos(
            _ElapsedSeconds * ck_loading_screen_transition_screen::TwoPi / _ThrobPeriod));

        _LogoImage->SetColorAndOpacity(
            FLinearColor{1.0f, 1.0f, 1.0f, FMath::Lerp(_ThrobMinOpacity, 1.0f, Phase)});
    }

    return SCompoundWidget::OnPaint(
        Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);
}

// --------------------------------------------------------------------------------------------------------------------
