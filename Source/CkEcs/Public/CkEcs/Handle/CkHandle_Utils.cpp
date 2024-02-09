#include "CkHandle_Utils.h"

#include "CkEcs/CkEcsLog.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Handle_UE::
    Get_IsValid(
        const FCk_Handle& InHandle)
    -> bool
{
    return ck::IsValid(InHandle);
}

auto
    UCk_Utils_Handle_UE::
    IsEqual(
        const FCk_Handle& InHandleA,
        const FCk_Handle& InHandleB)
    -> bool
{
    return InHandleA == InHandleB;
}

auto
    UCk_Utils_Handle_UE::
    IsNotEqual(
        const FCk_Handle& InHandleA,
        const FCk_Handle& InHandleB)
    -> bool
{
    return InHandleA != InHandleB;
}

auto
    UCk_Utils_Handle_UE::
    Conv_HandleToText(
        const FCk_Handle& InHandle)
    -> FText
{
    return FText::FromString(Conv_HandleToString(InHandle));
}

auto
    UCk_Utils_Handle_UE::
    Conv_HandleToString(
        const FCk_Handle& InHandle)
    -> FString
{
    return ck::Format_UE(TEXT("{}"), InHandle);
}

// --------------------------------------------------------------------------------------------------------------------
UE_DISABLE_OPTIMIZATION_SHIP
auto
    UCk_Utils_Handle_UE::
    Debug_Handle(
        const FCk_Handle& InHandle)
    -> void
{
    ck::ecs::Warning(TEXT("Debugging the Handle [{}]"), InHandle);
}
UE_ENABLE_OPTIMIZATION_SHIP
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Handle_UE::
    Set_DebugName(
        FCk_Handle InHandle,
        FName InDebugName)
    -> void
{
#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING
    InHandle.AddOrGet<DEBUG_NAME>()._Name = InDebugName;
    ck::ecs::Log(TEXT("[EntityMap] [{}]"), InHandle, InDebugName);
#endif
}

UE_ENABLE_OPTIMIZATION_SHIP

auto
    UCk_Utils_Handle_UE::
    Break_Handle(
        const FCk_Handle& InHandle,
        FCk_Entity& OutEntity)
    -> void
{
   OutEntity = InHandle.Get_Entity();
}

auto
    UCk_Utils_Handle_UE::
    Get_RawHandle(
        UStruct*,
        FCk_Handle&)
        -> void
{
    // We should never hit this! stubs to avoid NoExport on the class.
    checkNoEntry();
}

DEFINE_FUNCTION(UCk_Utils_Handle_UE::execGet_RawHandle)
{
    //// Read wildcard Value input.
    //Stack.MostRecentPropertyAddress = nullptr;
    //Stack.MostRecentPropertyContainer = nullptr;
    //Stack.StepCompiledIn<FStructProperty>(nullptr);

    //const FStructProperty* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
    //const void* ValuePtr = Stack.MostRecentPropertyAddress;

    //P_FINISH;

    //if (!ValueProp || !ValuePtr)
    //{
    //    FBlueprintExceptionInfo ExceptionInfo(
    //        EBlueprintExceptionType::AbortExecution,
    //        LOCTEXT("InstancedStruct_MakeInvalidValueWarning", "Failed to resolve the Value for Make Instanced Struct")
    //    );

    //    FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);

    //    P_NATIVE_BEGIN;
    //    (*(FInstancedStruct*)RESULT_PARAM).Reset();
    //    P_NATIVE_END;
    //}
    //else
    //{
    //    P_NATIVE_BEGIN;
    //    (*(FCk_Handle*)RESULT_PARAM).InitializeAs(ValueProp->Struct, (const uint8*)ValuePtr);
    //    P_NATIVE_END;
    //}

    ON_SCOPE_EXIT
    {
        P_FINISH
    };

    P_GET_STRUCT(FCk_Handle, InHandle);
    P_GET_STRUCT_REF(FCk_Handle, OutHandle);

    OutHandle = InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
