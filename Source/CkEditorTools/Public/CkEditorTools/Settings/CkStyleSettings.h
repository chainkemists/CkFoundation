#pragma once

#include "CkSettings/UserSettings/CkUserSettings.h"

#include "CkStyleSettings.generated.h"

// ====================================================================================================================
// Editor-time tunables for Ck editor tooling UIs (debuggers, analyzers, future
// Slate tools). Displayed in Editor Preferences under "Ck → Style".
// All values are read live via GetDefault<UCk_Style_UserSettings_UE>(),
// normally through the CkStyle:: accessors in CkEditorTools/Style/CkStyle.h.
//
// Defaults are set inline on each UPROPERTY below — do NOT add a constructor;
// the inherited UCk_Plugin_UserSettings_UE only accepts FObjectInitializer.
// ====================================================================================================================

UCLASS(meta = (DisplayName = "Style"))
class CKEDITORTOOLS_API UCk_Style_UserSettings_UE : public UCk_Plugin_UserSettings_UE
{
	GENERATED_BODY()

public:
	virtual auto GetCategoryName() const -> FName override { return TEXT("Ck"); }
	virtual auto GetSectionName()  const -> FName override { return TEXT("Style"); }

	// ----- Palette: Backgrounds — each tier slightly lighter than the last --
	UPROPERTY(Config, EditAnywhere, Category = "Palette|Backgrounds")
	FLinearColor BgRoot = FLinearColor(FColor(0x0b, 0x0e, 0x13, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Backgrounds")
	FLinearColor Bg1 = FLinearColor(FColor(0x11, 0x15, 0x1c, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Backgrounds")
	FLinearColor Bg2 = FLinearColor(FColor(0x16, 0x1b, 0x24, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Backgrounds")
	FLinearColor Bg3 = FLinearColor(FColor(0x1b, 0x22, 0x30, 255));

	// ----- Palette: Borders --------------------------------------------------
	UPROPERTY(Config, EditAnywhere, Category = "Palette|Borders")
	FLinearColor Border = FLinearColor(FColor(0x23, 0x2a, 0x38, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Borders")
	FLinearColor BorderStrong = FLinearColor(FColor(0x32, 0x39, 0x4a, 255));

	// ----- Palette: Text -----------------------------------------------------
	UPROPERTY(Config, EditAnywhere, Category = "Palette|Text")
	FLinearColor Text = FLinearColor(FColor(0xe5, 0xe7, 0xed, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Text")
	FLinearColor TextDim = FLinearColor(FColor(0x8a, 0x92, 0xa4, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Text")
	FLinearColor TextMute = FLinearColor(FColor(0x5a, 0x62, 0x77, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Text")
	FLinearColor TextStrong = FLinearColor(0.95f, 0.95f, 0.95f);

	// ----- Palette: Selection & Hover ---------------------------------------
	UPROPERTY(Config, EditAnywhere, Category = "Palette|Selection")
	FLinearColor Selection = FLinearColor(0.2f, 0.4f, 0.8f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Selection")
	FLinearColor SelectionInactive = FLinearColor(0.15f, 0.15f, 0.2f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Selection")
	FLinearColor Hover = FLinearColor(0.06f, 0.06f, 0.08f);

	// ----- Palette: Domain (ECS / component semantics) ----------------------
	UPROPERTY(Config, EditAnywhere, Category = "Palette|Domain")
	FLinearColor None = FLinearColor(0.4f, 0.4f, 0.4f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Domain")
	FLinearColor EntityId = FLinearColor(0.51f, 0.69f, 1.0f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Domain")
	FLinearColor Transform = FLinearColor(0.76f, 0.91f, 0.55f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Domain")
	FLinearColor Network = FLinearColor(1.0f, 0.8f, 0.01f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Domain")
	FLinearColor Relationship = FLinearColor(0.97f, 0.73f, 0.85f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Domain")
	FLinearColor Attribute = FLinearColor(0.55f, 0.85f, 0.95f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Domain")
	FLinearColor Reference = FLinearColor(0.51f, 0.69f, 1.0f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Domain")
	FLinearColor PickMarker_Default = FLinearColor(0.35f, 0.75f, 0.95f, 0.65f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Domain")
	FLinearColor PickMarker_Hover = FLinearColor(1.0f, 0.9f, 0.2f, 1.0f);

	// ----- Palette: Value-type colors ---------------------------------------
	UPROPERTY(Config, EditAnywhere, Category = "Palette|Values")
	FLinearColor Value_Bool_True = FLinearColor(0.2f, 1.0f, 0.4f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Values")
	FLinearColor Value_Bool_False = FLinearColor(1.0f, 0.4f, 0.4f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Values")
	FLinearColor Value_Numeric = FLinearColor(0.6f, 0.9f, 0.6f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Values")
	FLinearColor Value_String = FLinearColor(1.0f, 0.85f, 0.5f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Values")
	FLinearColor Value_Math = FLinearColor(0.7f, 0.7f, 1.0f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Values")
	FLinearColor Value_Tag = FLinearColor(0.8f, 0.6f, 1.0f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Values")
	FLinearColor Value_Enum = FLinearColor(0.5f, 0.9f, 0.9f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Values")
	FLinearColor Value_Object = FLinearColor(0.9f, 0.7f, 0.4f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Values")
	FLinearColor Value_Handle = FLinearColor(0.4f, 0.8f, 1.0f);

	// ----- Palette: State colors --------------------------------------------
	UPROPERTY(Config, EditAnywhere, Category = "Palette|States")
	FLinearColor State_Enabled = FLinearColor(0.0f, 1.0f, 0.5f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|States")
	FLinearColor State_Disabled = FLinearColor(1.0f, 0.5f, 0.5f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|States")
	FLinearColor State_Overlapping = FLinearColor(1.0f, 0.95f, 0.0f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|States")
	FLinearColor State_Config = FLinearColor(1.0f, 0.8f, 0.01f);

	// ----- Palette: Status colors (objective / task progress) --------------
	UPROPERTY(Config, EditAnywhere, Category = "Palette|Status")
	FLinearColor Status_NotStarted = FLinearColor(0.5f, 0.5f, 0.5f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Status")
	FLinearColor Status_Active = FLinearColor(0.55f, 0.78f, 0.95f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Status")
	FLinearColor Status_Completed = FLinearColor(0.6f, 0.85f, 0.55f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Status")
	FLinearColor Status_Failed = FLinearColor(0.95f, 0.35f, 0.3f);

	// ----- Palette: Graph canvas colors -------------------------------------
	UPROPERTY(Config, EditAnywhere, Category = "Palette|Graph")
	FLinearColor Graph_Background = FLinearColor(FColor(0x0D, 0x0D, 0x14));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Graph")
	FLinearColor Graph_Edge = FLinearColor(0.4f, 0.4f, 0.45f);

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Graph")
	FLinearColor Graph_Node_Center = FLinearColor(FColor(0x2D, 0x2D, 0x3D));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Graph")
	FLinearColor Graph_Node_Default = FLinearColor(FColor(0x1E, 0x1E, 0x2E));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Graph")
	FLinearColor Graph_Node_Border_Default = FLinearColor(FColor(0x60, 0x7D, 0x8B));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Graph")
	FLinearColor Graph_Node_Border_Center = FLinearColor(FColor(0x4C, 0xAF, 0x50));

	// ----- Palette: Semantic -------------------------------------------------
	UPROPERTY(Config, EditAnywhere, Category = "Palette|Semantic")
	FLinearColor Accent = FLinearColor(FColor(0xf5, 0xc8, 0x42, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Semantic")
	FLinearColor Ok = FLinearColor(FColor(0x55, 0xc4, 0x7a, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Semantic")
	FLinearColor Err = FLinearColor(FColor(0xd6, 0x5a, 0x5a, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Semantic")
	FLinearColor Warn = FLinearColor(FColor(0xe6, 0xa5, 0x45, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Semantic")
	FLinearColor Info = FLinearColor(FColor(0x5f, 0xb3, 0xd4, 255));

	// ----- Palette: Action Categories ---------------------------------------
	UPROPERTY(Config, EditAnywhere, Category = "Palette|Categories")
	FLinearColor CategoryGather = FLinearColor(FColor(0x4e, 0xa8, 0x4e, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Categories")
	FLinearColor CategoryBuild = FLinearColor(FColor(0xc2, 0x8a, 0x2a, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Categories")
	FLinearColor CategoryResearch = FLinearColor(FColor(0x5f, 0xb3, 0xd4, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Categories")
	FLinearColor CategoryTrain = FLinearColor(FColor(0xc7, 0x4c, 0x4c, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Categories")
	FLinearColor CategoryAge = FLinearColor(FColor(0xb4, 0x6f, 0xd0, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Palette|Categories")
	FLinearColor CategoryTrade = FLinearColor(FColor(0xd4, 0xb1, 0x5f, 255));

	// ----- Typography --------------------------------------------------------
	// Global scale. Per-widget sizes below override these where needed.
	UPROPERTY(Config, EditAnywhere, Category = "Typography", meta = (ClampMin = 6, ClampMax = 24))
	int32 FontSizeH2 = 12;

	UPROPERTY(Config, EditAnywhere, Category = "Typography", meta = (ClampMin = 6, ClampMax = 24))
	int32 FontSizeH3 = 10;

	UPROPERTY(Config, EditAnywhere, Category = "Typography", meta = (ClampMin = 6, ClampMax = 24))
	int32 FontSizeH4 = 9;

	UPROPERTY(Config, EditAnywhere, Category = "Typography", meta = (ClampMin = 6, ClampMax = 24))
	int32 FontSizeBody = 9;

	UPROPERTY(Config, EditAnywhere, Category = "Typography", meta = (ClampMin = 6, ClampMax = 24))
	int32 FontSizeSmall = 9;

	UPROPERTY(Config, EditAnywhere, Category = "Typography", meta = (ClampMin = 6, ClampMax = 24))
	int32 FontSizeMicro = 8;

	// ----- Pane Headings -----------------------------------------------------
	// Every pane header is a separate tunable so nothing sneaks through with a
	// hardcoded size/color.
	UPROPERTY(Config, EditAnywhere, Category = "Pane Headings", meta = (ClampMin = 6, ClampMax = 24))
	int32 PaneHeadingFontSize = 9;

	UPROPERTY(Config, EditAnywhere, Category = "Pane Headings")
	FLinearColor PaneHeadingColor = FLinearColor(FColor(0x8a, 0x92, 0xa4, 255));

	// ----- Plan Strip --------------------------------------------------------
	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip", meta = (ClampMin = 6, ClampMax = 24))
	int32 PlanStrip_TitleFontSize = 9;

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip", meta = (ClampMin = 6, ClampMax = 24))
	int32 PlanStrip_MetaFontSize = 9;

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip", meta = (ClampMin = 6, ClampMax = 24))
	int32 PlanStrip_GoalLabelFontSize = 9;

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip", meta = (ClampMin = 6, ClampMax = 24))
	int32 PlanStrip_GoalNameFontSize = 10;

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip", meta = (ClampMin = 6, ClampMax = 24))
	int32 PlanStrip_StepNameFontSize = 10;

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip", meta = (ClampMin = 6, ClampMax = 24))
	int32 PlanStrip_StepCostFontSize = 9;

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip", meta = (ClampMin = 6, ClampMax = 24))
	int32 PlanStrip_StepStateFontSize = 7;

	// Step pill color trios. Opaque fills, not tints — flat translucent
	// overlays read as a saturated wash, opaque gives depth.
	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip|Pending Step")
	FLinearColor PlanStep_Fill_Pending = FLinearColor(FColor(0x16, 0x1b, 0x24, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip|Pending Step")
	FLinearColor PlanStep_Border_Pending = FLinearColor(FColor(0x23, 0x2a, 0x38, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip|Pending Step")
	FLinearColor PlanStep_Badge_Pending = FLinearColor(FColor(0x1b, 0x22, 0x30, 255));

	// Active (currently-executing) step uses a teal family so it reads as
	// distinct from goals (amber Accent) and the rest of the plan (Info blue).
	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip|Active Step")
	FLinearColor PlanStep_Fill_Active = FLinearColor(FColor(0x0a, 0x28, 0x22, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip|Active Step")
	FLinearColor PlanStep_Border_Active = FLinearColor(FColor(0x5f, 0xd4, 0xb3, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip|Active Step")
	FLinearColor PlanStep_Badge_Active = FLinearColor(FColor(0x5f, 0xd4, 0xb3, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip|Done Step")
	FLinearColor PlanStep_Fill_Done = FLinearColor(FColor(0x10, 0x22, 0x17, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip|Done Step")
	FLinearColor PlanStep_Border_Done = FLinearColor(FColor(0x55, 0xc4, 0x7a, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip|Done Step")
	FLinearColor PlanStep_Badge_Done = FLinearColor(FColor(0x55, 0xc4, 0x7a, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip|Goal Pill")
	FLinearColor PlanStrip_GoalFill = FLinearColor(FColor(0x2a, 0x22, 0x0a, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Plan Strip|Goal Pill")
	FLinearColor PlanStrip_GoalBorder = FLinearColor(FColor(0xf5, 0xc8, 0x42, 255));

	// ----- Graph Nodes -------------------------------------------------------
	UPROPERTY(Config, EditAnywhere, Category = "Graph Nodes|Inactive")
	FLinearColor NodeFill_Inactive = FLinearColor(FColor(0x16, 0x1b, 0x24, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Graph Nodes|Inactive")
	FLinearColor NodeBorder_Inactive = FLinearColor(FColor(0x23, 0x2a, 0x38, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Graph Nodes|In Plan")
	FLinearColor NodeFill_InPlan = FLinearColor(FColor(0x14, 0x23, 0x35, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Graph Nodes|In Plan")
	FLinearColor NodeBorder_InPlan = FLinearColor(FColor(0x5f, 0xb3, 0xd4, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Graph Nodes|Goal")
	FLinearColor NodeFill_Goal = FLinearColor(FColor(0x2a, 0x22, 0x0a, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Graph Nodes|Goal")
	FLinearColor NodeBorder_Goal = FLinearColor(FColor(0xf5, 0xc8, 0x42, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Graph Nodes|Goal")
	FLinearColor NodeFill_GoalInactive = FLinearColor(FColor(0x15, 0x13, 0x08, 255));

	UPROPERTY(Config, EditAnywhere, Category = "Graph Nodes|Typography", meta = (ClampMin = 6, ClampMax = 24))
	int32 NodeTitleFontSize = 10;

	UPROPERTY(Config, EditAnywhere, Category = "Graph Nodes|Typography", meta = (ClampMin = 6, ClampMax = 24))
	int32 NodeCostFontSize = 9;

	UPROPERTY(Config, EditAnywhere, Category = "Graph Nodes|Typography", meta = (ClampMin = 6, ClampMax = 24))
	int32 NodeMetaFontSize = 9;

	UPROPERTY(Config, EditAnywhere, Category = "Graph Nodes", meta = (ClampMin = 0.5, ClampMax = 5))
	float NodeBorderThickness = 1.5f;

	UPROPERTY(Config, EditAnywhere, Category = "Graph Nodes", meta = (ClampMin = 0.1, ClampMax = 1))
	float NodeInactiveOpacity = 0.55f;
};

// ====================================================================================================================
