#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// "Write Back to Script" — the asset-editor toolbar button for AngelScript literal assets.
//
// A literal asset (`asset <Name> of <Type> { ... }`) has no .uasset behind it, so Ctrl+S refuses it
// ("Cannot save asset declared as an angelscript asset literal"). This writes the edited property
// values back into the declaring `.as` file instead, as a surgical per-statement patch.
//
// Everything UObject- and editor-coupled lives here; the text patcher, the accessor resolver, and
// the value diff are headless so the automation tests can drive them directly.

namespace ck::angelscriptgenerator::write_back
{
    class CKANGELSCRIPTGENERATOR_API FCkAsAssetWriteBack
    {
    public:
        // Adds a dynamic section to `AssetEditor.DefaultToolBar`, the parent every asset-editor
        // toolbar registers against (`AssetEditorToolkit.cpp:1453`). The section adds nothing unless
        // the edited object qualifies, so non-literal asset editors are untouched.
        static auto Register_ToolbarExtension() -> void;

        static auto Unregister_ToolbarExtension() -> void;

        // Outer is the AngelScript assets package and the declared class is not itself script-declared.
        static auto Get_IsWriteBackCandidate(
            const UObject* InAsset) -> bool;
    };
}

// --------------------------------------------------------------------------------------------------------------------
