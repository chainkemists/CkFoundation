#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Enums/CkEnums.h"

#include <Kismet/BlueprintFunctionLibrary.h>
#include <EdGraph/EdGraphPin.h>
#include <EdGraph/EdGraphNode.h>
#include <Editor/BlueprintGraph/Classes/EdGraphSchema_K2.h>
#include <K2Node_Event.h>
#include <KismetCompiler.h>
#include <CoreMinimal.h>
#include <functional>

#include "CkEditorGraph_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EditorGraph_PinDirection : uint8
{
    Input,
    Output
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EditorGraph_PinDirection);
CK_DEFINE_CUSTOM_FORMATTER_ENUM(EEdGraphPinDirection);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EditorGraph_DefaultPinType : uint8
{
    Then,
    Exec,
    Result,
    Self,
    OutputDelegate
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EditorGraph_DefaultPinType);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EditorGraph_PinLinkType : uint8
{
    Move,
    Copy
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EditorGraph_PinLinkType);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKEDITORGRAPH_API UCk_Utils_EditorGraph_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_EditorGraph_UE);

public:
    using GraphPinType             = TOptional<UEdGraphPin*>;
    using GraphPinListType         = std::initializer_list<GraphPinType>;
    using GraphPinPairType         = std::pair<GraphPinType, GraphPinType>;
    using GraphPinPairListType     = std::initializer_list<GraphPinPairType>;
    using CompilerContextOptType   = TOptional<FKismetCompilerContext*>;
    using MultiplePinsFuncType     = TFunction<ECk_SucceededFailed(const UEdGraphSchema_K2*, const GraphPinType&, const GraphPinType&)>;
    using NodePinPredicateFuncType = TFunction<bool(const UEdGraphPin*)>;
    using NodePinUnaryFuncType     = TFunction<void(UEdGraphPin*)>;

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Editor|Graph",
              DisplayName = "[Ck] Get Graph Pin Direction (To Unreal)")
    static TEnumAsByte<enum EEdGraphPinDirection>
    Get_PinDirection_ToUnreal(
        ECk_EditorGraph_PinDirection InPinDirection);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|Editor|Graph",
              DisplayName = "[Ck] Get Graph Pin Direction (From Unreal)")
    static ECk_EditorGraph_PinDirection
    Get_PinDirection_FromUnreal(
        TEnumAsByte<enum EEdGraphPinDirection> InPinDirection);

public:
    static auto
    Get_Pin(
        FName InPinName,
        ECk_EditorGraph_PinDirection InExpectedPinDirection,
        const TArray<UEdGraphPin*>& InPins) -> GraphPinType;

    static auto
    Get_Pin(
        FName InPinName,
        ECk_EditorGraph_PinDirection InExpectedPinDirection,
        const UEdGraphNode& InGraphNode) -> GraphPinType;

    static auto
    Get_Pin(
        const UEdGraphNode& InGraphNode,
        ECk_EditorGraph_DefaultPinType InPinType) -> GraphPinType;

    static auto
    Get_Pin_Then(
        const UEdGraphNode& InGraphNode) -> GraphPinType;

    static auto
    Get_Pin_Result(
        const UEdGraphNode& InGraphNode) -> GraphPinType;

    static auto
    Get_Pin_Exec(
        const UEdGraphNode& InGraphNode) -> GraphPinType;

    static auto
    Get_Pin_Self(
        const UEdGraphNode& InGraphNode) -> GraphPinType;

    static auto
    Get_Pin_OutputDelegate(
        const UEdGraphNode& InGraphNode) -> GraphPinType;

public:
    template <typename T_Enum>
    static auto
    Get_Pin_EnumValue (
        FName InPinName,
        ECk_EditorGraph_PinDirection InExpectedPinDirection,
        const UEdGraphNode& InGraphNode) -> TOptional<T_Enum>;

public:
    static auto
    Request_LinkPins(
        FKismetCompilerContext& InCompilerContext,
        GraphPinPairListType InGraphPins,
        ECk_EditorGraph_PinLinkType  InLinkType) -> ECk_SucceededFailed;

    static auto
    Request_ValidatePins(
        GraphPinListType InGraphPins,
        CompilerContextOptType InCompilerContext = {}) -> ECk_ValidInvalid;

    static auto
    Request_TryCreateConnection(
        FKismetCompilerContext& InCompilerContext,
        GraphPinPairListType InGraphPins) -> ECk_SucceededFailed;

    static auto
    Request_CopyPinValues(
        FKismetCompilerContext& InCompilerContext,
        GraphPinPairListType InGraphPins) -> ECk_SucceededFailed;

    static auto
    Request_ForceRefreshNode(
        UEdGraphNode& InGraphNode) -> void;

public:
    static auto
    ForEach_NodePins(
        UEdGraphNode& InGraphNode,
        const NodePinUnaryFuncType& InFunc) -> void;

    static auto
    ForEach_NodePins_If(
        UEdGraphNode& InGraphNode,
        const NodePinPredicateFuncType& InPredicate,
        const NodePinUnaryFuncType& InFunc) -> void;

private:
    static auto
    DoModifyMultiplePins(
        FKismetCompilerContext& InCompilerContext,
        GraphPinPairType InGraphPins,
        MultiplePinsFuncType InFunc) -> ECk_SucceededFailed;
};

// --------------------------------------------------------------------------------------------------------------------
// Definitions

template <typename T_Enum>
auto
    UCk_Utils_EditorGraph_UE::
    Get_Pin_EnumValue(
        FName InPinName,
        ECk_EditorGraph_PinDirection InExpectedPinDirection,
        const UEdGraphNode& InGraphNode)
    -> TOptional<T_Enum>
{
    const auto& Pin = Get_Pin(InPinName, InExpectedPinDirection, InGraphNode);

    if (ck::Is_NOT_Valid(Pin))
    { return {}; }

    return ck::Get_EnumValueByName<T_Enum>(FName((*Pin)->DefaultValue));
}

// --------------------------------------------------------------------------------------------------------------------
