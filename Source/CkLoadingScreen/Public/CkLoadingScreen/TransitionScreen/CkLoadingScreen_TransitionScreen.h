#pragma once

#include <Internationalization/Text.h>
#include <Math/Color.h>
#include <Math/Vector2D.h>
#include <Templates/SharedPointer.h>
#include <Widgets/DeclarativeSyntaxSupport.h>
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
        , _LogoOffset(FVector2D{-40.0, -40.0})
        , _ThrobPeriod(1.5f)
        , _ThrobMinOpacity(0.35f)
    {}
        /** Pre-resolved, pre-sized. Null = no logo. */
        SLATE_ARGUMENT(TSharedPtr<FDeferredCleanupSlateBrush>, LogoBrush)

        /** Pre-resolved full-bleed backdrop. Null = the tint IS the background. */
        SLATE_ARGUMENT(TSharedPtr<FDeferredCleanupSlateBrush>, BackdropBrush)

        SLATE_ARGUMENT(FLinearColor, BackgroundTint)

        /** Inset from the bottom-right corner. Negative values move the logo inward. */
        SLATE_ARGUMENT(FVector2D, LogoOffset)

        /** Full 1 -> min -> 1 cycle, in seconds. Zero or negative = a static logo. */
        SLATE_ARGUMENT(float, ThrobPeriod)

        SLATE_ARGUMENT(float, ThrobMinOpacity)

        /** Already picked by the caller - the widget does no selection of its own. */
        SLATE_ARGUMENT(FText, TipText)
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

    float _ThrobPeriod = 1.5f;
    float _ThrobMinOpacity = 0.35f;

    /** Mutable because the only clock this widget has is the const OnPaint it is driven by. */
    mutable float _ElapsedSeconds = 0.0f;
};

// --------------------------------------------------------------------------------------------------------------------
