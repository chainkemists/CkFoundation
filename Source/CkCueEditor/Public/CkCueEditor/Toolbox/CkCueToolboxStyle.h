#pragma once

#include <Styling/SlateStyle.h>
#include <Styling/SlateStyleRegistry.h>
#include <Framework/Application/SlateApplication.h>
#include <Interfaces/IPluginManager.h>

// --------------------------------------------------------------------------------------------------------------------

class CKCUEEDITOR_API FCkCueToolboxStyle
{
public:
    static auto Initialize() -> void;
    static auto Shutdown() -> void;
    static auto ReloadTextures() -> void;
    static auto Get() -> const ISlateStyle& { return *StyleInstance; }
    static auto Get_StyleSetName() -> FName;

private:
    static auto Create() -> TSharedRef<FSlateStyleSet>;

private:
    static TSharedPtr<FSlateStyleSet> StyleInstance;
};

// --------------------------------------------------------------------------------------------------------------------