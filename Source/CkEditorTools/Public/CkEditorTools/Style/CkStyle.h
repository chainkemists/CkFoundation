#pragma once

#include "CoreMinimal.h"
#include "Fonts/SlateFontInfo.h"

// ====================================================================================================================
// Color, font-size, and node-visual tokens shared by Ck editor tooling UIs
// (debuggers, analyzers, and future Slate tools). All non-constexpr values
// route through UCk_Style_UserSettings_UE so users can tune the palette from
// Editor Preferences → Ck → Style. The function accessors are lazy-resolved
// so there are no static init order surprises.
// ====================================================================================================================

struct FSlateBrush;

// --------------------------------------------------------------------------------------------------------------------

/**
 * Semantic tone for status-style UI (pills, badges, severity-colored text).
 * Resolve to a color via CkStyle::GetToneColor.
 */
enum class ECk_Tone : uint8
{
	Neutral,
	Info,
	Ok,
	Warn,
	Err,
	Accent,
};

// --------------------------------------------------------------------------------------------------------------------

namespace CkStyle
{
	// ----- Helpers ------------------------------------------------------------
	inline auto Rgb(uint8 R, uint8 G, uint8 B) -> FLinearColor
	{
		return FLinearColor(FColor(R, G, B, 255));
	}

	inline auto OverlayOf(const FLinearColor& InBase, float InAlpha) -> FLinearColor
	{
		auto C = InBase; C.A = InAlpha; return C;
	}

	// ----- Palette ------------------------------------------------------------
	CKEDITORTOOLS_API auto BgRoot()       -> FLinearColor;
	CKEDITORTOOLS_API auto Bg1()          -> FLinearColor;
	CKEDITORTOOLS_API auto Bg2()          -> FLinearColor;
	CKEDITORTOOLS_API auto Bg3()          -> FLinearColor;

	CKEDITORTOOLS_API auto Border()       -> FLinearColor;
	CKEDITORTOOLS_API auto BorderStrong() -> FLinearColor;

	CKEDITORTOOLS_API auto Text()         -> FLinearColor;
	CKEDITORTOOLS_API auto TextDim()      -> FLinearColor;
	CKEDITORTOOLS_API auto TextMute()     -> FLinearColor;
	CKEDITORTOOLS_API auto TextStrong()   -> FLinearColor;

	CKEDITORTOOLS_API auto Accent()       -> FLinearColor;
	CKEDITORTOOLS_API auto Ok()           -> FLinearColor;
	CKEDITORTOOLS_API auto Err()          -> FLinearColor;
	CKEDITORTOOLS_API auto Warn()         -> FLinearColor;
	CKEDITORTOOLS_API auto Info()         -> FLinearColor;

	// Per-tone dim surfaces (chip/pill fills).
	CKEDITORTOOLS_API auto AccentDim()    -> FLinearColor;
	CKEDITORTOOLS_API auto OkDim()        -> FLinearColor;
	CKEDITORTOOLS_API auto ErrDim()       -> FLinearColor;
	CKEDITORTOOLS_API auto WarnDim()      -> FLinearColor;
	CKEDITORTOOLS_API auto InfoDim()      -> FLinearColor;
	CKEDITORTOOLS_API auto NeutralDim()   -> FLinearColor;

	// ----- Selection & Hover --------------------------------------------------
	CKEDITORTOOLS_API auto Selection()         -> FLinearColor;
	CKEDITORTOOLS_API auto SelectionInactive() -> FLinearColor;
	CKEDITORTOOLS_API auto Hover()             -> FLinearColor;

	// ----- Domain (ECS / component semantics) --------------------------------
	CKEDITORTOOLS_API auto None()               -> FLinearColor;
	CKEDITORTOOLS_API auto EntityId()           -> FLinearColor;
	CKEDITORTOOLS_API auto Transform()          -> FLinearColor;
	CKEDITORTOOLS_API auto Network()            -> FLinearColor;
	CKEDITORTOOLS_API auto Relationship()       -> FLinearColor;
	CKEDITORTOOLS_API auto Attribute()          -> FLinearColor;
	CKEDITORTOOLS_API auto Reference()          -> FLinearColor;
	CKEDITORTOOLS_API auto PickMarker_Default() -> FLinearColor;
	CKEDITORTOOLS_API auto PickMarker_Hover()   -> FLinearColor;

	// ----- Value-type colors -------------------------------------------------
	CKEDITORTOOLS_API auto Value_Bool_True()  -> FLinearColor;
	CKEDITORTOOLS_API auto Value_Bool_False() -> FLinearColor;
	CKEDITORTOOLS_API auto Value_Numeric()    -> FLinearColor;
	CKEDITORTOOLS_API auto Value_String()     -> FLinearColor;
	CKEDITORTOOLS_API auto Value_Math()       -> FLinearColor;
	CKEDITORTOOLS_API auto Value_Tag()        -> FLinearColor;
	CKEDITORTOOLS_API auto Value_Enum()       -> FLinearColor;
	CKEDITORTOOLS_API auto Value_Object()     -> FLinearColor;
	CKEDITORTOOLS_API auto Value_Handle()     -> FLinearColor;

	// ----- State colors ------------------------------------------------------
	CKEDITORTOOLS_API auto State_Enabled()     -> FLinearColor;
	CKEDITORTOOLS_API auto State_Disabled()    -> FLinearColor;
	CKEDITORTOOLS_API auto State_Overlapping() -> FLinearColor;
	CKEDITORTOOLS_API auto State_Config()      -> FLinearColor;

	// ----- Status colors (objective / task progress) -------------------------
	CKEDITORTOOLS_API auto Status_NotStarted() -> FLinearColor;
	CKEDITORTOOLS_API auto Status_Active()     -> FLinearColor;
	CKEDITORTOOLS_API auto Status_Completed()  -> FLinearColor;
	CKEDITORTOOLS_API auto Status_Failed()     -> FLinearColor;

	// ----- Graph canvas colors -----------------------------------------------
	CKEDITORTOOLS_API auto Graph_Background()         -> FLinearColor;
	CKEDITORTOOLS_API auto Graph_Edge()               -> FLinearColor;
	CKEDITORTOOLS_API auto Graph_Node_Center()        -> FLinearColor;
	CKEDITORTOOLS_API auto Graph_Node_Default()       -> FLinearColor;
	CKEDITORTOOLS_API auto Graph_Node_Border_Default()-> FLinearColor;
	CKEDITORTOOLS_API auto Graph_Node_Border_Center() -> FLinearColor;

	CKEDITORTOOLS_API auto CategoryGather()   -> FLinearColor;
	CKEDITORTOOLS_API auto CategoryBuild()    -> FLinearColor;
	CKEDITORTOOLS_API auto CategoryResearch() -> FLinearColor;
	CKEDITORTOOLS_API auto CategoryTrain()    -> FLinearColor;
	CKEDITORTOOLS_API auto CategoryAge()      -> FLinearColor;
	CKEDITORTOOLS_API auto CategoryTrade()    -> FLinearColor;

	// ----- Typography ---------------------------------------------------------
	CKEDITORTOOLS_API auto FontSizeH2()    -> int32;
	CKEDITORTOOLS_API auto FontSizeH3()    -> int32;
	CKEDITORTOOLS_API auto FontSizeH4()    -> int32;
	CKEDITORTOOLS_API auto FontSizeBody()  -> int32;
	CKEDITORTOOLS_API auto FontSizeSmall() -> int32;
	CKEDITORTOOLS_API auto FontSizeMicro() -> int32;

	// ----- Pane headings ------------------------------------------------------
	CKEDITORTOOLS_API auto PaneHeadingFontSize() -> int32;
	CKEDITORTOOLS_API auto PaneHeadingColor()    -> FLinearColor;

	// ----- Plan strip ---------------------------------------------------------
	CKEDITORTOOLS_API auto PlanStrip_TitleFontSize()     -> int32;
	CKEDITORTOOLS_API auto PlanStrip_MetaFontSize()      -> int32;
	CKEDITORTOOLS_API auto PlanStrip_GoalLabelFontSize() -> int32;
	CKEDITORTOOLS_API auto PlanStrip_GoalNameFontSize()  -> int32;
	CKEDITORTOOLS_API auto PlanStrip_StepNameFontSize()  -> int32;
	CKEDITORTOOLS_API auto PlanStrip_StepCostFontSize()  -> int32;
	CKEDITORTOOLS_API auto PlanStrip_StepStateFontSize() -> int32;

	CKEDITORTOOLS_API auto PlanStep_Fill_Pending()   -> FLinearColor;
	CKEDITORTOOLS_API auto PlanStep_Border_Pending() -> FLinearColor;
	CKEDITORTOOLS_API auto PlanStep_Badge_Pending()  -> FLinearColor;
	CKEDITORTOOLS_API auto PlanStep_Fill_Active()    -> FLinearColor;
	CKEDITORTOOLS_API auto PlanStep_Border_Active()  -> FLinearColor;
	CKEDITORTOOLS_API auto PlanStep_Badge_Active()   -> FLinearColor;
	CKEDITORTOOLS_API auto PlanStep_Fill_Done()      -> FLinearColor;
	CKEDITORTOOLS_API auto PlanStep_Border_Done()    -> FLinearColor;
	CKEDITORTOOLS_API auto PlanStep_Badge_Done()     -> FLinearColor;
	CKEDITORTOOLS_API auto PlanStrip_GoalFill()      -> FLinearColor;
	CKEDITORTOOLS_API auto PlanStrip_GoalBorder()    -> FLinearColor;

	// ----- Graph node visuals -------------------------------------------------
	CKEDITORTOOLS_API auto NodeFill_Inactive()     -> FLinearColor;
	CKEDITORTOOLS_API auto NodeBorder_Inactive()   -> FLinearColor;
	CKEDITORTOOLS_API auto NodeFill_InPlan()       -> FLinearColor;
	CKEDITORTOOLS_API auto NodeBorder_InPlan()     -> FLinearColor;
	CKEDITORTOOLS_API auto NodeFill_Goal()         -> FLinearColor;
	CKEDITORTOOLS_API auto NodeBorder_Goal()       -> FLinearColor;
	CKEDITORTOOLS_API auto NodeFill_GoalInactive() -> FLinearColor;
	CKEDITORTOOLS_API auto NodeTitleFontSize()     -> int32;
	CKEDITORTOOLS_API auto NodeCostFontSize()      -> int32;
	CKEDITORTOOLS_API auto NodeMetaFontSize()      -> int32;
	CKEDITORTOOLS_API auto NodeBorderThickness()   -> float;
	CKEDITORTOOLS_API auto NodeInactiveOpacity()   -> float;

	// ----- Spacing (compile-time constants — no need to tune) -----------------
	constexpr auto SpaceXS  = 2.0f;
	constexpr auto SpaceS   = 4.0f;
	constexpr auto SpaceM   = 8.0f;
	constexpr auto SpaceL   = 12.0f;
	constexpr auto SpaceXL  = 16.0f;
	constexpr auto SpaceXXL = 24.0f;

	// ----- Typography helpers -------------------------------------------------
	// Single authority for font families so every debugger surface agrees:
	// data/ids/costs render Mono, headers Bold, prose Regular.
	CKEDITORTOOLS_API auto RegularFont(int32 InSize) -> FSlateFontInfo;
	CKEDITORTOOLS_API auto BoldFont(int32 InSize)    -> FSlateFontInfo;
	CKEDITORTOOLS_API auto MonoFont(int32 InSize)    -> FSlateFontInfo;

	// ----- Brushes / helpers --------------------------------------------------
	// The rounded brushes are WHITE procedural rounded boxes — tint at the use
	// site (SBorder::BorderBackgroundColor / SImage::ColorAndOpacity). Nest an
	// outer (border tint, 1px padding) around an inner (fill tint) for the
	// bordered-chip look.
	CKEDITORTOOLS_API auto GetFilledBrush()        -> const FSlateBrush*;
	CKEDITORTOOLS_API auto GetRoundedBrush()       -> const FSlateBrush*;   // 6px — chips, buttons, value pills
	CKEDITORTOOLS_API auto GetRoundedBrush_Small() -> const FSlateBrush*;   // 3px — badges, meters, count chips
	CKEDITORTOOLS_API auto GetRoundedBrush_Large() -> const FSlateBrush*;   // 8px — cards, panels, graph nodes
	CKEDITORTOOLS_API auto GetRoundedBrush_Pill()  -> const FSlateBrush*;   // fully-rounded — dots, switches, pills
	CKEDITORTOOLS_API auto GetToneColor(ECk_Tone InTone)    -> FLinearColor;
	CKEDITORTOOLS_API auto GetToneDimColor(ECk_Tone InTone) -> FLinearColor;
}

// ====================================================================================================================
