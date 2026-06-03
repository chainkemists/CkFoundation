#pragma once

#include "CoreMinimal.h"

#include "Toolkits/BaseToolkit.h"

// --------------------------------------------------------------------------------------------------------------------

// Minimal toolkit shell for the Grid Paint editor mode. For now it just hosts a placeholder palette
// panel; the paint-tool palette is filled in by a later task.
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
    TSharedPtr<SWidget> InlineContent;
};

// --------------------------------------------------------------------------------------------------------------------
