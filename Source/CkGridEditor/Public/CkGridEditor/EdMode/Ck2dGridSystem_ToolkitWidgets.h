#pragma once

#include "CoreMinimal.h"

#include "Misc/Attribute.h"
#include "Misc/Optional.h"
#include "Styling/SlateColor.h"
#include "Fonts/SlateFontInfo.h"

// --------------------------------------------------------------------------------------------------------------------

class SWidget;
struct FSlateBrush;

// --------------------------------------------------------------------------------------------------------------------

// Lightweight, local mirror of the CkDebuggerCommon visual language (section headers, labeled-group cards,
// key/value rows, count badges, color swatches) for the Grid Paint editor-mode toolkit. We CANNOT depend on
// CkDebuggerCommon directly: it lives in the CkDebugger plugin which depends on CkFoundation, so a dependency
// the other way would be a circular plugin dependency. These helpers reproduce the same look using only engine
// FAppStyle / FCoreStyle brushes and the hardcoded palette below (copied from UCkDebuggerStyleSettings defaults
// — FLinearColor(FColor(hex)) so the sRGB->linear conversion matches the debugger exactly).
namespace ck::grid_paint_style
{
    // ----- Palette (sRGB hex -> linear, matching the debugger) ------------------------------------------------
    CKGRIDEDITOR_API auto BgRoot()   -> FLinearColor; // #0B0E13 — outermost panel fill
    CKGRIDEDITOR_API auto Bg1()      -> FLinearColor; // #11151C — group header strip
    CKGRIDEDITOR_API auto Bg2()      -> FLinearColor; // #161B24 — group body
    CKGRIDEDITOR_API auto Bg3()      -> FLinearColor; // #1B2230 — badge / inset fill
    CKGRIDEDITOR_API auto Border()   -> FLinearColor; // #232A38 — 1px card border
    CKGRIDEDITOR_API auto Text()     -> FLinearColor; // #E5E7ED — primary text
    CKGRIDEDITOR_API auto TextDim()  -> FLinearColor; // #8A92A4 — header / label text
    CKGRIDEDITOR_API auto TextMute() -> FLinearColor; // #5A6277 — counts / secondary
    CKGRIDEDITOR_API auto Accent()   -> FLinearColor; // #F5C842 — accent (gold)
    CKGRIDEDITOR_API auto Info()     -> FLinearColor; // #5FB3D4 — info (teal)

    // ----- Spacing tokens -------------------------------------------------------------------------------------
    constexpr auto SpaceXS = 2.0f;
    constexpr auto SpaceS  = 4.0f;
    constexpr auto SpaceM  = 8.0f;
    constexpr auto SpaceL  = 12.0f;

    // ----- Fonts ----------------------------------------------------------------------------------------------
    CKGRIDEDITOR_API auto Font_Bold(int32 InSize = 9) -> FSlateFontInfo;
    CKGRIDEDITOR_API auto Font_Regular(int32 InSize = 9) -> FSlateFontInfo;

    // ----- Brushes (square, no rounding — matches the debugger) -----------------------------------------------
    CKGRIDEDITOR_API auto WhiteBrush() -> const FSlateBrush*;   // FAppStyle GenericWhiteBox (tint via color)

    // ----- Composable builders --------------------------------------------------------------------------------

    // Uppercase bold dim section header (no background). Used above the tool selector and inline labels.
    CKGRIDEDITOR_API auto Make_SectionHeader(const FText& InLabel) -> TSharedRef<SWidget>;

    // A bordered "card": a tinted header strip (uppercase bold title + optional left dot) over a body panel.
    // InBody is the caller-built content placed inside the body panel.
    CKGRIDEDITOR_API auto Make_LabeledGroup(
        const FText&              InTitle,
        TSharedRef<SWidget>       InBody,
        TOptional<FLinearColor>   InDotColor = {}) -> TSharedRef<SWidget>;

    // A key: value row — dim label on the left, primary-colored bold value on the right (live-bound).
    CKGRIDEDITOR_API auto Make_KeyValueRow(
        const FText&      InKey,
        TAttribute<FText> InValue) -> TSharedRef<SWidget>;

    // A small dark square badge holding a number (e.g. cell count). Bold, muted.
    CKGRIDEDITOR_API auto Make_CountBadge(int32 InCount) -> TSharedRef<SWidget>;

    // A solid square color swatch (CategoryDot equivalent).
    CKGRIDEDITOR_API auto Make_Swatch(const FLinearColor& InColor, float InSize = 12.0f) -> TSharedRef<SWidget>;
}

// --------------------------------------------------------------------------------------------------------------------
