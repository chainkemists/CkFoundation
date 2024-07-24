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
        FCk_Handle& InHandle,
        FName InDebugName,
        ECk_Override InOverride)
    -> void
{
    QUICK_SCOPE_CYCLE_COUNTER(Set_DebugName)

#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING
    if (NOT InHandle.Has<DEBUG_NAME>())
    {
        InHandle.Add<DEBUG_NAME>(InDebugName);
    }
    else
    {
        InHandle.Get<DEBUG_NAME>().DoSet_DebugName(InDebugName, InOverride);
    }

    ck::ecs::LogIf(UCk_Utils_Ecs_Settings_UE::Get_EntityMapPolicy() == ECk_Ecs_EntityMap_Policy::AlwaysLog,
        TEXT("[EntityMap] [{}]"), InHandle, InDebugName);
#endif
}

auto
    UCk_Utils_Handle_UE::
    Get_DebugName(
        const FCk_Handle& InHandle)
    -> FName
{
#if CK_ECS_DISABLE_HANDLE_DEBUGGING
    return {};
#else
    if (NOT InHandle.Has<DEBUG_NAME>())
    { return FName(Conv_HandleToString(InHandle)); }

    return InHandle.Get<DEBUG_NAME>().Get_Name();
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
    Get_InvalidHandle()
    -> const FCk_Handle&
{
    static FCk_Handle Invalid;
    return Invalid;
}

auto
    UCk_Utils_Handle_UE::
    Get_RawHandle(
        UStruct*)
        -> FCk_Handle
{
    // We should never hit this! stubs to avoid NoExport on the class.
    checkNoEntry();
    return {};
}

DEFINE_FUNCTION(UCk_Utils_Handle_UE::execGet_RawHandle)
{
    ON_SCOPE_EXIT
    {
        P_FINISH
    };

    P_GET_STRUCT(FCk_Handle, InHandle);

    *static_cast<FCk_Handle*>(RESULT_PARAM) = InHandle;
}

// --------------------------------------------------------------------------------------------------------------------
