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
    Conv_HandleTypeSafeToHandle(
        UStruct*,
        FCk_Handle&)
        -> void
{
	// We should never hit this! stubs to avoid NoExport on the class.
	checkNoEntry();
}

DEFINE_FUNCTION(UCk_Utils_Handle_UE::execConv_HandleTypeSafeToHandle)
{
    ON_SCOPE_EXIT
    {
        P_FINISH
    };

    P_GET_STRUCT(FCk_Handle, InHandle);
    P_GET_STRUCT_REF(FCk_Handle, OutHandle);

    OutHandle = InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
