#pragma once

#include "CkCore/Enums/CkEnums.h"

#include <K2Node.h>
#include <BlueprintNodeSpawner.h>
#include <KismetCompiler.h>

#include "CkUFunctionBase_K2Node.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract)
class CKK2NODES_API UCk_K2Node_UFunction_Base : public UK2Node
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_K2Node_UFunction_Base);

public:
    auto
    AllocateDefaultPins() -> void override;

    auto
    GetMenuActions(
        FBlueprintActionDatabaseRegistrar& InActionRegistrar) const -> void override;

    auto
    ExpandNode(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph) -> void override;

    auto
    PinDefaultValueChanged(
        UEdGraphPin* InPin) -> void override;

protected:
    virtual auto
    DoAllocate_DefaultPins() -> void { }

    virtual auto
    DoExpandNode(
        class FKismetCompilerContext& InCompilerContext,
        UEdGraph* InSourceGraph,
        ECk_ValidInvalid InNodeValidity) -> void { };

    virtual auto
    DoPinDefaultValueChanged(
        UEdGraphPin* InPin) -> void { }

    virtual auto
    DoGet_Menu_NodeTitle() const -> FText { return FText::FromName(TEXT("UFunction Base - Not to be used directly")); }

    virtual auto
    DoGet_Menu_SlateIcon() const -> FSlateIcon { return {}; }

    virtual auto
    DoValidateNodePins(
        const TOptional<FKismetCompilerContext*>& InCompilerContext = {}) const -> ECk_ValidInvalid { return ECk_ValidInvalid::Valid; }
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Transient)
class CKK2NODES_API UCk_K2NodeSpawner_UFunction_Base : public UBlueprintNodeSpawner
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_K2NodeSpawner_UFunction_Base);

public:
    static auto
    Create(
        TSubclassOf<UK2Node> InNodeClass,
        FText InNodeName,
        FSlateIcon InSlateIcon,
        UObject* InOuter = nullptr) -> UCk_K2NodeSpawner_UFunction_Base*;
};

// --------------------------------------------------------------------------------------------------------------------
