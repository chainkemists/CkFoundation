#include "CkAnimationToolbox_Style.h"

#include <Styling/SlateStyleRegistry.h>
#include <Styling/CoreStyle.h>
#include <Interfaces/IPluginManager.h>
#include <Styling/SlateStyleMacros.h>

// --------------------------------------------------------------------------------------------------------------------

TSharedPtr<FSlateStyleSet> FCkAnimationToolboxStyle::StyleInstance = nullptr;

auto
    FCkAnimationToolboxStyle::
    Initialize()
    -> void
{
    if (StyleInstance.IsValid())
    { return; }

    StyleInstance = Create();
    FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
}

auto
    FCkAnimationToolboxStyle::
    Shutdown()
    -> void
{
    if (NOT StyleInstance.IsValid())
    { return; }

    FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
    StyleInstance.Reset();
}

auto
    FCkAnimationToolboxStyle::
    Get()
    -> const ISlateStyle&
{
    return *StyleInstance;
}

auto
    FCkAnimationToolboxStyle::
    GetStyleSetName()
    -> FName
{
    return TEXT("CkAnimationToolboxStyle");
}

auto
    FCkAnimationToolboxStyle::
    Create()
    -> TSharedRef<FSlateStyleSet>
{
    TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));

    // Borrow the engine's "Persona" icon for the toolbar button — no custom assets needed.
    Style->Set("CkAnimationToolbox.Icon", new FSlateImageBrush(
        FPaths::EngineContentDir() / TEXT("Editor/Slate/Icons/AssetIcons/AnimSequence_64x.png"),
        FVector2D(40.0f, 40.0f)));

    return Style;
}

// --------------------------------------------------------------------------------------------------------------------
