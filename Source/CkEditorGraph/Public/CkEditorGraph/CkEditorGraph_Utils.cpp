#include "CkEditorGraph_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_EditorGraph_UE::
    Get_PinDirection_ToUnreal(
        ECk_EditorGraph_PinDirection InPinDirection)
    -> TEnumAsByte<EEdGraphPinDirection>
{
    switch(InPinDirection)
    {
        case ECk_EditorGraph_PinDirection::Input:
        {
            return EGPD_Input;
        }
        case ECk_EditorGraph_PinDirection::Output:
        {
            return EGPD_Output;
        }
        default:
        {
            CK_INVALID_ENUM(InPinDirection);
            return EGPD_MAX;
        }
    }
}

auto
    UCk_Utils_EditorGraph_UE::
    Get_PinDirection_FromUnreal(
        TEnumAsByte<EEdGraphPinDirection> InPinDirection)
    -> ECk_EditorGraph_PinDirection
{
    switch(InPinDirection.GetValue())
    {
        case EGPD_Input:
        {
            return ECk_EditorGraph_PinDirection::Input;
        }
        case EGPD_Output:
        {
            return ECk_EditorGraph_PinDirection::Output;
        }
        case EGPD_MAX:
            [[fallthrough]];
        default:
        {
            CK_INVALID_ENUM(InPinDirection.GetValue());
            return ECk_EditorGraph_PinDirection::Input;
        }
    }
}

auto
    UCk_Utils_EditorGraph_UE::
    Get_Pin(
        FName InPinName,
        ECk_EditorGraph_PinDirection InExpectedPinDirection,
        const TArray<UEdGraphPin*>& InPins)
    -> GraphPinType
{

    const auto& FoundPin = ck::algo::FindIf(InPins, [&](const UEdGraphPin* InPinToCompare)
    {
        if (ck::Is_NOT_Valid(InPinToCompare, ck::IsValid_Policy_NullptrOnly{}))
        { return false; }

        return InPinToCompare->PinName == InPinName;

    }, ck::policy::ReturnOptional{});

    if (ck::Is_NOT_Valid(FoundPin))
    { return {}; }

    const auto& PinDirection = Get_PinDirection_FromUnreal((*FoundPin)->Direction);
    CK_ENSURE_IF_NOT(PinDirection == InExpectedPinDirection, TEXT("Pin direction [{}] is NOT the expected direction [{}]"), PinDirection, InExpectedPinDirection)
    { return {}; }

    return *FoundPin;
}

auto
    UCk_Utils_EditorGraph_UE::
    Get_Pin(
        FName InPinName,
        ECk_EditorGraph_PinDirection InExpectedPinDirection,
        const UEdGraphNode& InGraphNode)
    -> GraphPinType
{
    return Get_Pin(InPinName, InExpectedPinDirection, InGraphNode.Pins);
}

auto
    UCk_Utils_EditorGraph_UE::
    Get_Pin(
        const UEdGraphNode& InGraphNode,
        ECk_EditorGraph_DefaultPinType InPinType)
    -> GraphPinType
{
        switch(InPinType)
    {
        case ECk_EditorGraph_DefaultPinType::Then:
        {
            const auto& Res = InGraphNode.FindPin(UEdGraphSchema_K2::PN_Then);

            if (ck::Is_NOT_Valid(Res, ck::IsValid_Policy_NullptrOnly{}))
            { return {}; }

            CK_ENSURE_IF_NOT(Res->Direction == EGPD_Output, TEXT("The [{}] pin's direction is invalid"), InPinType)
            { return {}; }

            return Res;
        }
        case ECk_EditorGraph_DefaultPinType::Exec:
        {
            const auto& Res = InGraphNode.FindPin(UEdGraphSchema_K2::PN_Execute);

            if (ck::Is_NOT_Valid(Res, ck::IsValid_Policy_NullptrOnly{}))
            { return {}; }

            CK_ENSURE_IF_NOT(Res->Direction == EGPD_Input, TEXT("The [{}] pin's direction is invalid"), InPinType)
            { return {}; }

            return Res;
        }
        case ECk_EditorGraph_DefaultPinType::Result:
        {
            const auto& Res = InGraphNode.FindPin(UEdGraphSchema_K2::PN_ReturnValue);

            if (ck::Is_NOT_Valid(Res, ck::IsValid_Policy_NullptrOnly{}))
            { return {}; }

            CK_ENSURE_IF_NOT(Res->Direction == EGPD_Output, TEXT("The [{}] pin's direction is invalid"), InPinType)
            { return {}; }

            return Res;
        }
        case ECk_EditorGraph_DefaultPinType::Self:
        {
            const auto& Res = InGraphNode.FindPin(UEdGraphSchema_K2::PN_Self);

            if (ck::Is_NOT_Valid(Res, ck::IsValid_Policy_NullptrOnly{}))
            { return {}; }

            CK_ENSURE_IF_NOT(Res->Direction == EGPD_Input, TEXT("The [{}] pin's direction is invalid"), InPinType)
            { return {}; }

            return Res;
        }
        default:
        {
            CK_INVALID_ENUM(InPinType);
            break;
        }
    }

    return {};
}

auto
    UCk_Utils_EditorGraph_UE::
    Get_Pin_Then(
        const UEdGraphNode& InGraphNode)
    -> GraphPinType
{
    return Get_Pin(InGraphNode, ECk_EditorGraph_DefaultPinType::Then);
}

auto
    UCk_Utils_EditorGraph_UE::
    Get_Pin_Result(
        const UEdGraphNode& InGraphNode)
    -> GraphPinType
{
    return Get_Pin(InGraphNode, ECk_EditorGraph_DefaultPinType::Result);
}

auto
    UCk_Utils_EditorGraph_UE::
    Get_Pin_Exec(
        const UEdGraphNode& InGraphNode)
    -> GraphPinType
{
    return Get_Pin(InGraphNode, ECk_EditorGraph_DefaultPinType::Exec);
}

auto
    UCk_Utils_EditorGraph_UE::
    Get_Pin_Self(
        const UEdGraphNode& InGraphNode)
    -> GraphPinType
{
    return Get_Pin(InGraphNode, ECk_EditorGraph_DefaultPinType::Self);
}

auto
    UCk_Utils_EditorGraph_UE::
    Request_LinkPins(
        FKismetCompilerContext& InCompilerContext,
        GraphPinPairListType InGraphPins,
        ECk_EditorGraph_PinLinkType InLinkType)
    -> ECk_SucceededFailed
{
    for (const auto& PinPair : InGraphPins)
    {
        if (DoModifyMultiplePins(InCompilerContext, PinPair,
        [&](const UEdGraphSchema_K2* InSchema, const GraphPinType& InSourcePin, const GraphPinType& InIntermediatePin)
        {
            switch(InLinkType)
            {
                case ECk_EditorGraph_PinLinkType::Move:
                {
                    if (NOT InCompilerContext.MovePinLinksToIntermediate(**InSourcePin, **InIntermediatePin).CanSafeConnect())
                    {
                        InCompilerContext.MessageLog.Error(*ck::Format_UE(
                            TEXT("Could NOT Move from Pin [{}] to Pin [{}]. MovePinLinksToIntermediate FAILED."),
                            (*InSourcePin)->GetName(),
                            (*InIntermediatePin)->GetName()));
                        return ECk_SucceededFailed::Failed;
                    }

                    return ECk_SucceededFailed::Succeeded;
                }
                case ECk_EditorGraph_PinLinkType::Copy:
                {
                    if (NOT InCompilerContext.CopyPinLinksToIntermediate(**InSourcePin, **InIntermediatePin).CanSafeConnect())
                    {
                        InCompilerContext.MessageLog.Error(*ck::Format_UE(
                            TEXT("Could NOT Copy from Pin [{}] to Pin [{}]. CopyPinLinksToIntermediate FAILED."),
                            (*InSourcePin)->GetName(),
                            (*InIntermediatePin)->GetName()));
                        return ECk_SucceededFailed::Failed;
                    }

                    return ECk_SucceededFailed::Succeeded;
                }
                default:
                {
                    CK_INVALID_ENUM(InLinkType);
                    return ECk_SucceededFailed::Failed;
                }
            }
        }) == ECk_SucceededFailed::Failed)
        { return ECk_SucceededFailed::Failed; }
    }

    return ECk_SucceededFailed::Succeeded;
}

auto
    UCk_Utils_EditorGraph_UE::
    Request_ValidatePins(
        GraphPinListType InGraphPins,
        CompilerContextOptType InCompilerContext)
    -> ECk_ValidInvalid
{
    auto SuccessFailed = ECk_ValidInvalid::Valid;

    const auto& LogCompilerError = [&](const auto& InErrorMessage)
    {
        if (ck::Is_NOT_Valid(InCompilerContext))
        { return; }

        (*InCompilerContext)->MessageLog.Error(InErrorMessage);
    };

    for (const auto& GraphPin : InGraphPins)
    {
        if (ck::Is_NOT_Valid(GraphPin))
        {
            LogCompilerError(*ck::Format_UE(TEXT("GraphPin is INVALID.")));
            SuccessFailed = ECk_ValidInvalid::Invalid;
            continue;
        }

        const auto& GraphPinObject = (*GraphPin)->DefaultObject;

        if (ck::Is_NOT_Valid(GraphPinObject) && (*GraphPin)->LinkedTo.IsEmpty())
        {
            LogCompilerError
            (
                *ck::Format_UE
                (
                    TEXT("GraphPin [{}] has an INVALID connection."),
                    (*GraphPin)->PinName
                )
            );
            SuccessFailed = ECk_ValidInvalid::Invalid;
        }
    }

    return SuccessFailed;
}

auto
    UCk_Utils_EditorGraph_UE::
    Request_TryCreateConnection(
        FKismetCompilerContext& InCompilerContext,
        GraphPinPairListType InGraphPins)
    -> ECk_SucceededFailed
{
    for (const auto& PinPair : InGraphPins)
    {
        if (DoModifyMultiplePins(InCompilerContext, PinPair,
        [&InCompilerContext](const UEdGraphSchema_K2* InSchema, const GraphPinType& InSourcePin, const GraphPinType& InIntermediatePin)
        {
            if (NOT InSchema->TryCreateConnection(*InSourcePin, *InIntermediatePin))
            {
                InCompilerContext.MessageLog.Error(*ck::Format_UE(
                    TEXT("Could NOT Connect Pin [{}] to Pin [{}]. TryCreateConnection FAILED."),
                    (*InSourcePin)->GetName(),
                    (*InIntermediatePin)->GetName()));
                return ECk_SucceededFailed::Failed;
            }

            return ECk_SucceededFailed::Succeeded;
        }) == ECk_SucceededFailed::Failed)
        { return ECk_SucceededFailed::Failed; }
    }

    return ECk_SucceededFailed::Succeeded;
}

auto
    UCk_Utils_EditorGraph_UE::
    Request_ForceRefreshNode(
        UEdGraphNode& InGraphNode)
    -> void
{
    const auto DisableOrphanPinSaving = InGraphNode.bDisableOrphanPinSaving;
    InGraphNode.bDisableOrphanPinSaving = true;

    InGraphNode.GetSchema()->ReconstructNode(InGraphNode);
    InGraphNode.ClearCompilerMessage();

    InGraphNode.bDisableOrphanPinSaving = DisableOrphanPinSaving;
}

auto
    UCk_Utils_EditorGraph_UE::
    DoModifyMultiplePins(
        FKismetCompilerContext& InCompilerContext,
        GraphPinPairType InGraphPins,
        MultiplePinsFuncType InFunc)
    -> ECk_SucceededFailed
{
    auto* Schema = InCompilerContext.GetSchema();
    if (ck::Is_NOT_Valid(Schema, ck::IsValid_Policy_NullptrOnly{}))
    {
        InCompilerContext.MessageLog.Error(*ck::Format_UE(TEXT("CompilerContext has an INVALID schema.")));
        return ECk_SucceededFailed::Failed;
    }

    if (ck::Is_NOT_Valid(InGraphPins.first))
    {
        InCompilerContext.MessageLog.Error(*ck::Format_UE(TEXT("InSourcePin is INVALID. Subsequent operation will fail.")));
        return ECk_SucceededFailed::Failed;
    }

    if (ck::Is_NOT_Valid(InGraphPins.second))
    {
        InCompilerContext.MessageLog.Error(*ck::Format_UE(TEXT("InIntermediatePin is INVALID. Subsequent operation will fail.")));
        return ECk_SucceededFailed::Failed;
    }

    return InFunc(Schema, InGraphPins.first, InGraphPins.second);
}

// --------------------------------------------------------------------------------------------------------------------

