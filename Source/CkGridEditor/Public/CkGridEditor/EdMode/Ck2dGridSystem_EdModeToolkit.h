#pragma once

#include "CoreMinimal.h"

#include "Toolkits/BaseToolkit.h"

// --------------------------------------------------------------------------------------------------------------------

enum class ECk_GridPaint_Tool : uint8;

// --------------------------------------------------------------------------------------------------------------------

// Toolkit for the Grid Paint editor mode. Hosts a 3-tool selector (Shape / Tags / Blocker) that sets
// the active tool on the owning UCk_2dGridSystem_EdMode. Only Shape is functional this task; the
// other two are selectable placeholders for the next tasks.
class FCk_2dGridSystem_EdModeToolkit : public FModeToolkit
{
public:
    FCk_2dGridSystem_EdModeToolkit();

    // FModeToolkit interface
    virtual void Init(const TSharedPtr<IToolkitHost>& InToolkitHost, TWeakObjectPtr<UEdMode> InOwningMode) override;
    virtual void GetToolPaletteNames(TArray<FName>& OutPaletteNames) const override;

    // IToolkit interface
    virtual FName GetToolkitFName() const override;
    virtual FText GetBaseToolkitName() const override;
    virtual TSharedPtr<SWidget> GetInlineContent() const override;

private:
    // Pushes the chosen tool onto the owning EdMode.
    auto Set_ActiveTool(ECk_GridPaint_Tool InTool) -> void;

    // Reads the owning EdMode's current tool (defaults to Shape if the mode is gone).
    auto Get_ActiveTool() const -> ECk_GridPaint_Tool;

    // Radio-button check state for one tool button.
    auto Get_ToolCheckState(ECk_GridPaint_Tool InTool) const -> ECheckBoxState;
    auto On_ToolCheckChanged(ECheckBoxState InNewState, ECk_GridPaint_Tool InTool) -> void;

private:
    TSharedPtr<SWidget>     InlineContent;
    TWeakObjectPtr<UEdMode> OwningMode;
};

// --------------------------------------------------------------------------------------------------------------------
