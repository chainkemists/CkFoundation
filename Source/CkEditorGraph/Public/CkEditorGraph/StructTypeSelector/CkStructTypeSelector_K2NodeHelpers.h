#pragma once

#include "CkCore/StructTypeSelector/CkStructTypeSelector.h"

#include <EdGraph/EdGraphNode.h>
#include <EdGraph/EdGraphPin.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    struct CKEDITORGRAPH_API FStructTypeSelectorHelpers
    {
        // Create a non-connectable selector pin on a node.
        // InBaseStructFilter: optional base struct to filter the dropdown (nullptr = show all)
        static auto CreateSelectorPin(
            UEdGraphNode& InNode,
            FName InPinName,
            UScriptStruct* InBaseStructFilter = nullptr) -> UEdGraphPin*;

        // Read the selected struct type from the selector pin
        static auto GetSelectedStruct(
            const UEdGraphNode& InNode,
            FName InPinName) -> UScriptStruct*;

        // Read the base struct filter stored on the selector pin
        static auto GetBaseStructFilter(
            const UEdGraphPin& InPin) -> UScriptStruct*;
    };
}

// --------------------------------------------------------------------------------------------------------------------
