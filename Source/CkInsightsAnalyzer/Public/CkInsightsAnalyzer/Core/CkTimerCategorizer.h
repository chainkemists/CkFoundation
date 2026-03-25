#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Timer category definition.
 * Each category has a name, a display priority, and a list of keywords
 * that match timer names (case-insensitive substring match).
 */
struct CKINSIGHTSANALYZER_API FCk_TimerCategory
{
    /** Display name (e.g., "Physics (UE Overlaps)"). */
    FString Name;

    /** Keywords that match this category (case-insensitive). */
    TArray<FString> Keywords;

    /** Display order priority (lower = shown first). */
    int32 Priority = 0;
};

// --------------------------------------------------------------------------------------------------------------------

/**
 * Categorizes timers by name and simplifies verbose Unreal timer names.
 *
 * Ported from analyze_frame.py's CATEGORY_KEYWORDS, NAME_REPLACEMENTS,
 * and simplify_name() logic.
 *
 * Usage:
 *   FCk_TimerCategorizer Categorizer;
 *   FString Category = Categorizer.Categorize(TEXT("UpdateOverlaps"));
 *   // -> "Physics (UE Overlaps)"
 *
 *   FString Simple = FCk_TimerCategorizer::SimplifyName(
 *       TEXT("UCharacterMovementComponent_TickComponent"));
 *   // -> "CharMovement Tick"
 */
class CKINSIGHTSANALYZER_API FCk_TimerCategorizer
{
public:
    FCk_TimerCategorizer();

    /** Categorize a timer by name. Returns category name or "Other". */
    auto Categorize(const FString& TimerName) const -> FString;

    /** Get all categories in display order. */
    auto GetCategories() const -> const TArray<FCk_TimerCategory>&;

    /** Get the display priority for a category name (lower = first). Returns MAX_int32 for unknown. */
    auto GetCategoryPriority(const FString& CategoryName) const -> int32;

    // ---- Static Name Simplification ----

    /** Simplify a verbose UE timer name for readability. */
    static auto SimplifyName(const FString& TimerName) -> FString;

    /** Format milliseconds for display (e.g., "24.3ms", "1.50ms", "0.234ms"). */
    static auto FormatMs(double Ms) -> FString;

    /** Get severity icon based on inclusive time in ms. */
    static auto SeverityIcon(double Ms) -> FString;

    /** Format call count with separator for large numbers (e.g., "1,234x"). */
    static auto FormatCount(uint32 Count) -> FString;

private:
    auto InitializeCategories() -> void;
    auto InitializeNameReplacements() -> void;

    TArray<FCk_TimerCategory> _Categories;

    /** Exact name→replacement mappings. */
    TMap<FString, FString> _NameReplacements;

    /** Prefixes to strip from timer names. */
    TArray<FString> _PrefixesToStrip;
};

// --------------------------------------------------------------------------------------------------------------------
