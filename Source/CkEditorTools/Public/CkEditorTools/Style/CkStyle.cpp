#include "CkStyle.h"

#include "CkEditorTools/Settings/CkStyleSettings.h"

#include "Styling/AppStyle.h"

// ====================================================================================================================

namespace ck_style
{
	auto DoSettings() -> const UCk_Style_UserSettings_UE*
	{
		return GetDefault<UCk_Style_UserSettings_UE>();
	}
}

#define CK_STYLE_GETTER(Name) \
	auto CkStyle::Name() -> decltype(UCk_Style_UserSettings_UE::Name) \
	{ \
		return ck_style::DoSettings()->Name; \
	}

// ====================================================================================================================

CK_STYLE_GETTER(BgRoot)
CK_STYLE_GETTER(Bg1)
CK_STYLE_GETTER(Bg2)
CK_STYLE_GETTER(Bg3)
CK_STYLE_GETTER(Border)
CK_STYLE_GETTER(BorderStrong)
CK_STYLE_GETTER(Text)
CK_STYLE_GETTER(TextDim)
CK_STYLE_GETTER(TextMute)
CK_STYLE_GETTER(TextStrong)
CK_STYLE_GETTER(Accent)
CK_STYLE_GETTER(Ok)
CK_STYLE_GETTER(Err)
CK_STYLE_GETTER(Warn)
CK_STYLE_GETTER(Info)

CK_STYLE_GETTER(Selection)
CK_STYLE_GETTER(SelectionInactive)
CK_STYLE_GETTER(Hover)

CK_STYLE_GETTER(None)
CK_STYLE_GETTER(EntityId)
CK_STYLE_GETTER(Transform)
CK_STYLE_GETTER(Network)
CK_STYLE_GETTER(Relationship)
CK_STYLE_GETTER(Attribute)
CK_STYLE_GETTER(Reference)
CK_STYLE_GETTER(PickMarker_Default)
CK_STYLE_GETTER(PickMarker_Hover)

CK_STYLE_GETTER(Value_Bool_True)
CK_STYLE_GETTER(Value_Bool_False)
CK_STYLE_GETTER(Value_Numeric)
CK_STYLE_GETTER(Value_String)
CK_STYLE_GETTER(Value_Math)
CK_STYLE_GETTER(Value_Tag)
CK_STYLE_GETTER(Value_Enum)
CK_STYLE_GETTER(Value_Object)
CK_STYLE_GETTER(Value_Handle)

CK_STYLE_GETTER(State_Enabled)
CK_STYLE_GETTER(State_Disabled)
CK_STYLE_GETTER(State_Overlapping)
CK_STYLE_GETTER(State_Config)

CK_STYLE_GETTER(Status_NotStarted)
CK_STYLE_GETTER(Status_Active)
CK_STYLE_GETTER(Status_Completed)
CK_STYLE_GETTER(Status_Failed)

CK_STYLE_GETTER(Graph_Background)
CK_STYLE_GETTER(Graph_Edge)
CK_STYLE_GETTER(Graph_Node_Center)
CK_STYLE_GETTER(Graph_Node_Default)
CK_STYLE_GETTER(Graph_Node_Border_Default)
CK_STYLE_GETTER(Graph_Node_Border_Center)

CK_STYLE_GETTER(CategoryGather)
CK_STYLE_GETTER(CategoryBuild)
CK_STYLE_GETTER(CategoryResearch)
CK_STYLE_GETTER(CategoryTrain)
CK_STYLE_GETTER(CategoryAge)
CK_STYLE_GETTER(CategoryTrade)

CK_STYLE_GETTER(FontSizeH2)
CK_STYLE_GETTER(FontSizeH3)
CK_STYLE_GETTER(FontSizeH4)
CK_STYLE_GETTER(FontSizeBody)
CK_STYLE_GETTER(FontSizeSmall)
CK_STYLE_GETTER(FontSizeMicro)

CK_STYLE_GETTER(PaneHeadingFontSize)
CK_STYLE_GETTER(PaneHeadingColor)

CK_STYLE_GETTER(PlanStrip_TitleFontSize)
CK_STYLE_GETTER(PlanStrip_MetaFontSize)
CK_STYLE_GETTER(PlanStrip_GoalLabelFontSize)
CK_STYLE_GETTER(PlanStrip_GoalNameFontSize)
CK_STYLE_GETTER(PlanStrip_StepNameFontSize)
CK_STYLE_GETTER(PlanStrip_StepCostFontSize)
CK_STYLE_GETTER(PlanStrip_StepStateFontSize)

CK_STYLE_GETTER(PlanStep_Fill_Pending)
CK_STYLE_GETTER(PlanStep_Border_Pending)
CK_STYLE_GETTER(PlanStep_Badge_Pending)
CK_STYLE_GETTER(PlanStep_Fill_Active)
CK_STYLE_GETTER(PlanStep_Border_Active)
CK_STYLE_GETTER(PlanStep_Badge_Active)
CK_STYLE_GETTER(PlanStep_Fill_Done)
CK_STYLE_GETTER(PlanStep_Border_Done)
CK_STYLE_GETTER(PlanStep_Badge_Done)
CK_STYLE_GETTER(PlanStrip_GoalFill)
CK_STYLE_GETTER(PlanStrip_GoalBorder)

CK_STYLE_GETTER(NodeFill_Inactive)
CK_STYLE_GETTER(NodeBorder_Inactive)
CK_STYLE_GETTER(NodeFill_InPlan)
CK_STYLE_GETTER(NodeBorder_InPlan)
CK_STYLE_GETTER(NodeFill_Goal)
CK_STYLE_GETTER(NodeBorder_Goal)
CK_STYLE_GETTER(NodeFill_GoalInactive)
CK_STYLE_GETTER(NodeTitleFontSize)
CK_STYLE_GETTER(NodeCostFontSize)
CK_STYLE_GETTER(NodeMetaFontSize)
CK_STYLE_GETTER(NodeBorderThickness)
CK_STYLE_GETTER(NodeInactiveOpacity)

#undef CK_STYLE_GETTER

// ====================================================================================================================

auto
	CkStyle::
	GetFilledBrush()
	-> const FSlateBrush*
{
	return FAppStyle::GetBrush(TEXT("GenericWhiteBox"));
}

auto
	CkStyle::
	GetRoundedBrush()
	-> const FSlateBrush*
{
	// RoundedSelection_16x is not reliably registered across UE builds — when
	// missing, Slate falls back to the "missing texture" checkerboard. Square
	// corners with GenericWhiteBox render reliably everywhere.
	return FAppStyle::GetBrush(TEXT("GenericWhiteBox"));
}

auto
	CkStyle::
	GetToneColor(ECk_Tone InTone)
	-> FLinearColor
{
	switch (InTone)
	{
		case ECk_Tone::Info:   return Info();
		case ECk_Tone::Ok:     return Ok();
		case ECk_Tone::Warn:   return Warn();
		case ECk_Tone::Err:    return Err();
		case ECk_Tone::Accent: return Accent();
		default:               return TextDim();
	}
}

// ====================================================================================================================
