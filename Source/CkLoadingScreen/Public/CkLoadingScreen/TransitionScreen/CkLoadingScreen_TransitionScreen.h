#pragma once

#include <Framework/Text/TextLayout.h>
#include <Internationalization/Text.h>
#include <Layout/Margin.h>
#include <Math/Color.h>
#include <Math/Vector2D.h>
#include <Styling/SlateTypes.h>
#include <Templates/SharedPointer.h>
#include <Types/SlateEnums.h>
#include <Widgets/DeclarativeSyntaxSupport.h>
#include <Widgets/Layout/SScaleBox.h>
#include <Widgets/SCompoundWidget.h>

// --------------------------------------------------------------------------------------------------------------------

class FDeferredCleanupSlateBrush;
class SImage;

// --------------------------------------------------------------------------------------------------------------------

/**
 * The Slate widget the MoviePlayer draws while the game thread is blocked in LoadMap.
 *
 * Three constraints make this widget look the way it does, and all three are load-bearing:
 *
 * 1. It runs on the Slate loading thread while the game thread is stalled, so it must NEVER touch
 *    a UObject after construction. Every brush arrives pre-resolved as FDeferredCleanupSlateBrush
 *    (the engine's MoviePlayer-safe wrapper); the constructor takes brushes, literals and FTexts,
 *    never soft paths and never UObjects.
 * 2. There is no Tick and no active timer available to it - the game thread that would drive them
 *    is the thread that is blocked. Animation is therefore PAINT-driven: OnPaint accumulates
 *    Args.GetDeltaTime(). This is the one mechanism the retired AsyncLoadingScreen plugin got
 *    right and the reason its screen animated through a blocking load.
 * 3. Texture lifetime is owned elsewhere (the subsystem's FStreamableHandle). This widget never
 *    loads anything, sync or async.
 */
class CKLOADINGSCREEN_API SCk_LoadingScreen_Transition : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCk_LoadingScreen_Transition)
        : _BackgroundTint(FLinearColor::Black)
        , _BackdropStretch(EStretch::ScaleToFill)
        , _LogoOffset(FVector2D{-40.0, -40.0})
        , _LogoHAlign(HAlign_Right)
        , _LogoVAlign(VAlign_Bottom)
        , _ThrobPeriod(1.5f)
        , _ThrobMinOpacity(0.35f)
        , _TipHAlign(HAlign_Center)
        , _TipVAlign(VAlign_Bottom)
        , _TipPadding(FMargin{0.0f, 0.0f, 0.0f, 64.0f})
        , _TipWrapTextAt(0.0f)
        , _TipJustification(ETextJustify::Center)
        , _TipTextMargin(FMargin{})
        , _TipLineHeightPercentage(1.0f)
    {}
        /** Pre-resolved, pre-sized. Null = no logo. */
        SLATE_ARGUMENT(TSharedPtr<FDeferredCleanupSlateBrush>, LogoBrush)

        /** Pre-resolved full-bleed backdrop. Null = the tint IS the background. */
        SLATE_ARGUMENT(TSharedPtr<FDeferredCleanupSlateBrush>, BackdropBrush)

        SLATE_ARGUMENT(FLinearColor, BackgroundTint)

        SLATE_ARGUMENT(EStretch::Type, BackdropStretch)

        /** Inset from the anchored corner/edge. Negative values move the logo inward. */
        SLATE_ARGUMENT(FVector2D, LogoOffset)

        SLATE_ARGUMENT(EHorizontalAlignment, LogoHAlign)

        SLATE_ARGUMENT(EVerticalAlignment, LogoVAlign)

        /** Full 1 -> min -> 1 cycle, in seconds. Zero or negative = a static logo. */
        SLATE_ARGUMENT(float, ThrobPeriod)

        SLATE_ARGUMENT(float, ThrobMinOpacity)

        /** Already picked by the caller - the widget does no selection of its own. */
        SLATE_ARGUMENT(FText, TipText)

        /**
         * Pre-resolved plain-struct style (constraint 1: the CommonTextStyle CDO it came from is
         * a UObject, so the CALLER flattens it on the game thread). Null = engine default look.
         */
        SLATE_ARGUMENT(TSharedPtr<FTextBlockStyle>, TipStyle)

        SLATE_ARGUMENT(EHorizontalAlignment, TipHAlign)

        SLATE_ARGUMENT(EVerticalAlignment, TipVAlign)

        /** Inset of the tip block from the screen edges its alignment anchors it to. */
        SLATE_ARGUMENT(FMargin, TipPadding)

        /** Wrap width in Slate units. Zero = never wrap. */
        SLATE_ARGUMENT(float, TipWrapTextAt)

        SLATE_ARGUMENT(ETextJustify::Type, TipJustification)

        /** The style asset's Margin/LineHeightPercentage - FTextBlockStyle cannot carry them. */
        SLATE_ARGUMENT(FMargin, TipTextMargin)

        SLATE_ARGUMENT(float, TipLineHeightPercentage)
    SLATE_END_ARGS()

    auto
    Construct(
        const FArguments& InArgs) -> void;

public:
    auto
    OnPaint(
        const FPaintArgs& Args,
        const FGeometry& AllottedGeometry,
        const FSlateRect& MyCullingRect,
        FSlateWindowElementList& OutDrawElements,
        int32 LayerId,
        const FWidgetStyle& InWidgetStyle,
        bool bParentEnabled) const -> int32 override;

private:
    TSharedPtr<FDeferredCleanupSlateBrush> _LogoBrush;
    TSharedPtr<FDeferredCleanupSlateBrush> _BackdropBrush;
    TSharedPtr<SImage> _LogoImage;

    /** STextBlock stores a raw pointer to its style - this keeps the pointee alive. */
    TSharedPtr<FTextBlockStyle> _TipStyle;

    float _ThrobPeriod = 1.5f;
    float _ThrobMinOpacity = 0.35f;

    /** Mutable because the only clock this widget has is the const OnPaint it is driven by. */
    mutable float _ElapsedSeconds = 0.0f;
};

// --------------------------------------------------------------------------------------------------------------------
