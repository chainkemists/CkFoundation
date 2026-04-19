#pragma once

#include <CoreMinimal.h>
#include <Styling/SlateStyle.h>

// --------------------------------------------------------------------------------------------------------------------

class CKANIMATIONEDITOR_API FCkAnimationToolboxStyle
{
public:
    static auto Initialize() -> void;
    static auto Shutdown() -> void;
    static auto Get() -> const ISlateStyle&;
    static auto GetStyleSetName() -> FName;

private:
    static auto Create() -> TSharedRef<FSlateStyleSet>;
    static TSharedPtr<FSlateStyleSet> StyleInstance;
};

// --------------------------------------------------------------------------------------------------------------------
