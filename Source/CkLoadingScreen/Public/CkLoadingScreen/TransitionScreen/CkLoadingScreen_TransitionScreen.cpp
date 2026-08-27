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

    // The opaque plate. With no backdrop configured this IS the background - a solid tint behind
    // the optional corner logo - so a project that configures nothing still gets a complete screen
    // rather than an empty one.
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
            .Stretch(InArgs._BackdropStretch)
            [
                SNew(SImage)
                .Image(_BackdropBrush->GetSlateBrush())
            ]
        ];
    }

    if (_LogoBrush.IsValid())
    {
        // The offset insets from whichever corner/edge the alignment anchors to; a centered axis
        // has no anchored edge to inset from, so the offset is inert there by construction.
        const auto LogoPadding = [&]
        {
            auto Padding = FMargin{};

            switch (InArgs._LogoHAlign)
            {
                case HAlign_Left:  Padding.Left  = static_cast<float>(-InArgs._LogoOffset.X); break;
                case HAlign_Right: Padding.Right = static_cast<float>(-InArgs._LogoOffset.X); break;
                default: break;
            }

            switch (InArgs._LogoVAlign)
            {
                case VAlign_Top:    Padding.Top    = static_cast<float>(-InArgs._LogoOffset.Y); break;
                case VAlign_Bottom: Padding.Bottom = static_cast<float>(-InArgs._LogoOffset.Y); break;
                default: break;
            }

            return Padding;
        }();

        Overlay->AddSlot()
        .HAlign(InArgs._LogoHAlign)
        .VAlign(InArgs._LogoVAlign)
        .Padding(LogoPadding)
        [
            SAssignNew(_LogoImage, SImage)
            .Image(_LogoBrush->GetSlateBrush())
        ];
    }

    if (NOT InArgs._TipText.IsEmpty())
    {
        _TipStyle = InArgs._TipStyle;

        if (NOT _TipStyle.IsValid())
        {
            // The pre-TipStyle shipping look, preserved verbatim for callers that configure nothing.
            _TipStyle = MakeShared<FTextBlockStyle>(FTextBlockStyle::GetDefault());
            _TipStyle->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", 20.0f));
            _TipStyle->SetColorAndOpacity(FSlateColor{FLinearColor::White});
        }

        Overlay->AddSlot()
        .HAlign(InArgs._TipHAlign)
        .VAlign(InArgs._TipVAlign)
        .Padding(InArgs._TipPadding)
        [
            SNew(STextBlock)
            .Text(InArgs._TipText)
            .TextStyle(_TipStyle.Get())
            .Justification(InArgs._TipJustification)
            .WrapTextAt(InArgs._TipWrapTextAt)
            .Margin(InArgs._TipTextMargin)
            .LineHeightPercentage(InArgs._TipLineHeightPercentage)
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
