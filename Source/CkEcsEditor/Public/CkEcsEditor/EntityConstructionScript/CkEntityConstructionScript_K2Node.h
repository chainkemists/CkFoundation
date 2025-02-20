#pragma once

#include "CkCore/IO/CkIO_Utils.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"

#include "CkEditorGraph/CkEditorGraph_Utils.h"

#include "CkEditorGraph/CkUFunctionBase_K2Node.h"

#include <K2Node_CallFunction.h>

#include "CkEntityConstructionScript_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UEdGraphPin;
class FBlueprintActionDatabaseRegistrar;

// --------------------------------------------------------------------------------------------------------------------


UCLASS(MinimalAPI)
class UCk_K2Node_EntityConstructionScript : public UCk_K2Node_UFunction_Base
{
    GENERATED_BODY()

public:
    // UObject interface
    auto PostEditChangeProperty(struct FPropertyChangedEvent& PropertyChangedEvent) -> void override;
    auto ShouldShowNodeProperties() const -> bool override;
    // End of UObject interface

    // K2Node implementation
    auto IsNodePure() const -> bool override;
    auto ReallocatePinsDuringReconstruction(TArray<UEdGraphPin*>& InOldPins) -> void override;
    auto GetMenuCategory() const -> FText override;
    auto IsCompatibleWithGraph(UEdGraph const* InGraph) const -> bool override;
    // End of K2Node implementation

    // UEdGraphNode implementation
    auto GetNodeTitle(ENodeTitleType::Type InTitleType) const -> FText override;
    auto GetIconAndTint(FLinearColor& OutColor) const -> FSlateIcon override;
    // End of UEdGraphNode implementation

protected:
    auto
    DoAllocate_DefaultPins()-> void override;

    auto
    DoExpandNode(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity)-> void override;

    auto
    DoGet_Menu_NodeTitle() const -> FText override;

private:
    static auto
    DoGet_IsSpawnVarPin(
        const UEdGraphPin* InPin) -> bool;

public:
    auto CreatePinsFromConstructionScript() -> void;

private:
    UPROPERTY(EditDefaultsOnly)
    TSubclassOf<UCk_Entity_ConstructionScript_PDA> _ConstructionScript;
};

// --------------------------------------------------------------------------------------------------------------------
