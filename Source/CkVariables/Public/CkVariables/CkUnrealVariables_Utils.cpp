#include "CkUnrealVariables_Utils.h"

#include "CkVariables_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include <Blueprint/BlueprintExceptionInfo.h>

#define LOCTEXT_NAMESPACE "CkUnrealVariables"

// --------------------------------------------------------------------------------------------------------------------

#define CK_UTILS_VARIABLES_DEFINITION(_UtilsName_, _Type_)                                            \
auto                                                                                                  \
    _UtilsName_::                                                                                     \
    Get(                                                                                              \
        const FCk_Handle& InHandle,                                                                   \
        FGameplayTag InVariableName,                                                                  \
        ECk_SucceededFailed& OutSuccessFail)                                                          \
    -> _Type_                                                                                         \
{                                                                                                     \
    if (NOT UtilsType::Has(InHandle, InVariableName))                                                 \
    {                                                                                                 \
        static std::remove_cv_t<std::remove_reference_t<_Type_>> Invalid;                             \
        OutSuccessFail = ECk_SucceededFailed::Failed;                                                 \
        return Invalid;                                                                               \
    }                                                                                                 \
                                                                                                      \
    OutSuccessFail = ECk_SucceededFailed::Succeeded;                                                  \
    return UtilsType::Get(InHandle, InVariableName);                                                  \
}                                                                                                     \
                                                                                                      \
auto                                                                                                  \
    _UtilsName_::                                                                                     \
    Get_Exec(                                                                                         \
        const FCk_Handle& InHandle,                                                                   \
        FGameplayTag InVariableName,                                                                  \
        ECk_SucceededFailed& OutSuccessFail)                                                          \
    -> _Type_                                                                                         \
{                                                                                                     \
    return Get(InHandle, InVariableName, OutSuccessFail);                                             \
}                                                                                                     \
                                                                                                      \
auto                                                                                                  \
    _UtilsName_::                                                                                     \
    Set(                                                                                              \
        FCk_Handle& InHandle,                                                                         \
        FGameplayTag InVariableName,                                                                  \
        _Type_ InValue)                                                                               \
    -> void                                                                                           \
{                                                                                                     \
    UtilsType::Set(InHandle, InVariableName, InValue);                                                \
}                                                                                                     \
auto                                                                                                  \
    _UtilsName_::                                                                                     \
    Get_ByName(                                                                                       \
        const FCk_Handle& InHandle,                                                                   \
        FName InVariableName,                                                                         \
        ECk_SucceededFailed& OutSuccessFail)                                                          \
    -> _Type_                                                                                         \
{                                                                                                     \
    if (NOT UtilsType::Has(InHandle, InVariableName))                                                 \
    {                                                                                                 \
        static std::remove_cv_t<std::remove_reference_t<_Type_>> Invalid;                             \
        OutSuccessFail = ECk_SucceededFailed::Failed;                                                 \
        return Invalid;                                                                               \
    }                                                                                                 \
                                                                                                      \
    OutSuccessFail = ECk_SucceededFailed::Succeeded;                                                  \
    return UtilsType::Get(InHandle, InVariableName);                                                  \
}                                                                                                     \
                                                                                                      \
auto                                                                                                  \
    _UtilsName_::                                                                                     \
    Set_ByName(                                                                                       \
        FCk_Handle& InHandle,                                                                         \
        FName InVariableName,                                                                         \
        _Type_ InValue)                                                                               \
    -> void                                                                                           \
{                                                                                                     \
    UtilsType::Set(InHandle, InVariableName, InValue);                                                \
}                                                                                                     \
auto                                                                                                  \
    _UtilsName_::                                                                                     \
    Get_All(                                                                                          \
        const FCk_Handle& InHandle)                                                                   \
    -> const TMap<FName, std::remove_cv_t<std::remove_reference_t<_Type_>>>&                          \
{                                                                                                     \
    if (NOT UtilsType::Has(InHandle))                                                                 \
    { static TMap<FName, std::remove_cv_t<std::remove_reference_t<_Type_>>> Invalid; return Invalid; }\
                                                                                                      \
    return InHandle.Get<FragmentType>().Get_Variables();                                              \
}

// --------------------------------------------------------------------------------------------------------------------

CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Bool_UE, bool)
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Byte_UE, uint8)
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Int32_UE, int32);
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Int64_UE, int64);
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Float_UE, float);
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Name_UE, FName);
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_String_UE, const FString&);
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Text_UE, FText);
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Vector_UE, FVector);
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Vector2D_UE, FVector2D);
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Rotator_UE, FRotator);
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Transform_UE, const FTransform&);
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_InstancedStruct_UE, const FInstancedStruct&);
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_GameplayTag_UE, const FGameplayTag);
CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_GameplayTagContainer_UE, const FGameplayTagContainer);

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Variables_UObject_UE::
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject,
        ECk_Recursion InRecursion,
        ECk_SucceededFailed& OutSuccessFail)
    -> UObject*
{
    OutSuccessFail = ECk_SucceededFailed::Failed;

    auto MaybeEntity = UtilsType::Has(InHandle, InVariableName) ? InHandle : FCk_Handle{};

    if (ck::Is_NOT_Valid(MaybeEntity) && InRecursion == ECk_Recursion::Recursive)
    {
        MaybeEntity = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InHandle,
        [&](const FCk_Handle& Handle)
        {
            return UtilsType::Has(Handle, InVariableName);
        });
    }

    if (ck::Is_NOT_Valid(MaybeEntity))
    { return {}; }

    const auto& Var = UtilsType::Get(InHandle, InVariableName);
    if (ck::Is_NOT_Valid(Var))
    { return {}; }

    OutSuccessFail = ECk_SucceededFailed::Succeeded;
    return Var.Get();
}

auto
    UCk_Utils_Variables_UObject_UE::
    Get_Exec(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject,
        ECk_Recursion InRecursion,
        ECk_SucceededFailed& OutSuccessFail)
    -> UObject*
{
    return Get(InHandle, InVariableName, InObject, InRecursion, OutSuccessFail);
}

auto
    UCk_Utils_Variables_UObject_UE::
    Set(
        FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        UObject* InValue)
    -> void
{
    UtilsType::Set(InHandle, InVariableName, InValue);
}

auto
    UCk_Utils_Variables_UObject_UE::
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        TSubclassOf<UObject> InObject,
        ECk_SucceededFailed& OutSuccessFail)
    -> UObject*
{
    OutSuccessFail = ECk_SucceededFailed::Failed;

    if (NOT UtilsType::Has(InHandle, InVariableName))
    { return {}; }

    const auto& Var = UtilsType::Get(InHandle, InVariableName);
    if (ck::Is_NOT_Valid(Var))
    { return {}; }

    OutSuccessFail = ECk_SucceededFailed::Succeeded;
    return Var.Get();
}

auto
    UCk_Utils_Variables_UObject_UE::
    Set_ByName(
        FCk_Handle& InHandle,
        FName InVariableName,
        UObject* InValue)
    -> void
{
    UtilsType::Set(InHandle, InVariableName, InValue);
}

auto
    UCk_Utils_Variables_UObject_UE::
    Get_All(
        const FCk_Handle& InHandle)
    -> TMap<FName, UObject*>
{
    if (NOT UtilsType::Has(InHandle))
    {
        static TMap<FName, UObject*> Invalid;
        return Invalid;
    }

    const auto& Variables = InHandle.Get<FragmentType>().Get_Variables();

    auto Res = TMap<FName, UObject*>{};

    for (auto Iterator : Variables)
    {
        Res.Add(Iterator.Key, ck::IsValid(Iterator.Value) ? Iterator.Value.Get() : nullptr);
    }

    return Res;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Variables_UObject_SubclassOf_UE::
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject,
        ECk_Recursion InRecursion,
        ECk_SucceededFailed& OutSuccessFail)
    -> TSubclassOf<UObject>
{
    OutSuccessFail = ECk_SucceededFailed::Failed;

    auto MaybeEntity = UtilsType::Has(InHandle, InVariableName) ? InHandle : FCk_Handle{};

    if (ck::Is_NOT_Valid(MaybeEntity) && InRecursion == ECk_Recursion::Recursive)
    {
        MaybeEntity = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InHandle,
        [&](const FCk_Handle& Handle)
        {
            return UtilsType::Has(Handle, InVariableName);
        });
    }

    if (ck::Is_NOT_Valid(MaybeEntity))
    { return {}; }

    OutSuccessFail = ECk_SucceededFailed::Succeeded;
    return UtilsType::Get(MaybeEntity, InVariableName);
}

auto
    UCk_Utils_Variables_UObject_SubclassOf_UE::
    Get_Exec(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject,
        ECk_Recursion InRecursion,
        ECk_SucceededFailed& OutSuccessFail)
    -> TSubclassOf<UObject>
{
    return Get(InHandle, InVariableName, InObject, InRecursion, OutSuccessFail);
}

auto
    UCk_Utils_Variables_UObject_SubclassOf_UE::
    Set(
        FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InValue)
    -> void
{
    return UtilsType::Set(InHandle, InVariableName, InValue);
}

auto
    UCk_Utils_Variables_UObject_SubclassOf_UE::
    Get_ByName(
        const FCk_Handle& InHandle,
        FName InVariableName,
        TSubclassOf<UObject> InObject,
        ECk_SucceededFailed& OutSuccessFail)
    -> TSubclassOf<UObject>
{
    OutSuccessFail = ECk_SucceededFailed::Failed;

    if (NOT UtilsType::Has(InHandle, InVariableName))
    { return {}; }

    OutSuccessFail = ECk_SucceededFailed::Succeeded;
    return UtilsType::Get(InHandle, InVariableName);
}

auto
    UCk_Utils_Variables_UObject_SubclassOf_UE::
    Set_ByName(
        FCk_Handle& InHandle,
        FName      InVariableName,
        TSubclassOf<UObject> InValue)
    -> void
{
    return UtilsType::Set(InHandle, InVariableName, InValue);
}

auto
    UCk_Utils_Variables_UObject_SubclassOf_UE::
    Get_All(
        const FCk_Handle& InHandle)
    -> const TMap<FName, TSubclassOf<UObject>>&
{
    if (NOT UtilsType::Has(InHandle))
    {
        static TMap<FName, TSubclassOf<UObject>> Invalid;
        return Invalid;
    }

    return InHandle.Get<FragmentType>().Get_Variables();
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Variables_Entity_UE::
    Get(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_Recursion InRecursion,
        ECk_SucceededFailed& OutSuccessFail)
    -> FCk_Handle
{
    OutSuccessFail = ECk_SucceededFailed::Failed;

    auto MaybeEntity = UtilsType::Has(InHandle, InVariableName) ? InHandle : FCk_Handle{};

    if (ck::Is_NOT_Valid(MaybeEntity) && InRecursion == ECk_Recursion::Recursive)
    {
        MaybeEntity = UCk_Utils_EntityLifetime_UE::Get_EntityInOwnershipChain_If(InHandle,
        [&](const FCk_Handle& Handle)
        {
            return UtilsType::Has(Handle, InVariableName);
        });
    }

    if (ck::Is_NOT_Valid(MaybeEntity))
    { return {}; }

    OutSuccessFail = ECk_SucceededFailed::Succeeded;
    return UtilsType::Get(MaybeEntity, InVariableName);
}

auto
    UCk_Utils_Variables_Entity_UE::
    Get_Exec(
        const FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        ECk_Recursion InRecursion,
        ECk_SucceededFailed& OutSuccessFail)
    -> FCk_Handle
{
    return Get(InHandle, InVariableName, InRecursion, OutSuccessFail);
}

auto
    UCk_Utils_Variables_Entity_UE::
    Set(
        FCk_Handle& InHandle,
        FGameplayTag InVariableName,
        FCk_Handle& InValue) -> void
{
    return UtilsType::Set(InHandle, InVariableName, InValue);
}

auto
    UCk_Utils_Variables_Entity_UE::
    Get_All(
        const FCk_Handle& InHandle)
    -> TMap<FName, FCk_Handle>
{
    return InHandle.Get<FragmentType>().Get_Variables();
}

// --------------------------------------------------------------------------------------------------------------------

DEFINE_FUNCTION(UCk_Utils_Variables_InstancedStruct_UE::execINTERNAL__Set_ByName)
{
    P_GET_STRUCT_REF(FCk_Handle, Handle);
    P_GET_PROPERTY(FNameProperty, VariableName);

    // Read wildcard Value input.
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.StepCompiledIn<FStructProperty>(nullptr);

    const auto* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
    const void* ValuePtr = Stack.MostRecentPropertyAddress;

    P_FINISH;

    if (!ValueProp || !ValuePtr)
    {
        const FBlueprintExceptionInfo ExceptionInfo(
            EBlueprintExceptionType::AbortExecution,
            LOCTEXT("CkInstancedStructVariable_SetInvalidValueWarning", "Failed to resolve the Value for Set Value (By Name)"));

        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
    }
    else
    {
        P_NATIVE_BEGIN;
        FInstancedStruct InstancedStruct;
        InstancedStruct.InitializeAs(ValueProp->Struct, static_cast<const uint8*>(ValuePtr));
        Set_ByName(Handle, VariableName, InstancedStruct);
        P_NATIVE_END;
    }
}

// --------------------------------------------------------------------------------------------------------------------

DEFINE_FUNCTION(UCk_Utils_Variables_InstancedStruct_UE::execINTERNAL__Set)
{
    P_GET_STRUCT(FCk_Handle, Handle);
    P_GET_STRUCT(FGameplayTag, VariableName);

    // Read wildcard Value input.
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.StepCompiledIn<FStructProperty>(nullptr);

    const auto* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
    const void* ValuePtr = Stack.MostRecentPropertyAddress;

    P_FINISH;

    if (!ValueProp || !ValuePtr)
    {
        const FBlueprintExceptionInfo ExceptionInfo(
            EBlueprintExceptionType::AbortExecution,
            LOCTEXT("CkInstancedStructVariable_SetInvalidValueWarning", "Failed to resolve the Value for Set Value (By GameplayTag)"));

        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
    }
    else
    {
        P_NATIVE_BEGIN;
        FInstancedStruct InstancedStruct;
        InstancedStruct.InitializeAs(ValueProp->Struct, static_cast<const uint8*>(ValuePtr));
        Set(Handle, VariableName, InstancedStruct);
        P_NATIVE_END;
    }
}

// --------------------------------------------------------------------------------------------------------------------

DEFINE_FUNCTION(UCk_Utils_Variables_InstancedStruct_UE::execINTERNAL__Get_ByName)
{
    P_GET_STRUCT(FCk_Handle, Handle);
    P_GET_PROPERTY(FNameProperty, VariableName);
    P_GET_ENUM(ECk_Recursion, Recursion)
    P_GET_ENUM_REF(ECk_SucceededFailed, SucceededFailed)

    // Read wildcard Value input.
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.StepCompiledIn<FStructProperty>(nullptr);

    const auto* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
    void* ValuePtr = Stack.MostRecentPropertyAddress;

    P_FINISH;

    SucceededFailed = ECk_SucceededFailed::Failed;

    if (!ValueProp || !ValuePtr)
    {
        const FBlueprintExceptionInfo ExceptionInfo(
            EBlueprintExceptionType::AbortExecution,
            LOCTEXT("CkInstancedStructVariable_GetInvalidValueWarning", "Failed to resolve the Value for Get Value (By Name)"));

        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
    }

    auto CurrentHandle = Handle;

    while (ck::IsValid(CurrentHandle))
    {
        const auto& InstancedStruct = Get_ByName(CurrentHandle, VariableName, SucceededFailed);

        if (SucceededFailed == ECk_SucceededFailed::Succeeded)
        {
            P_NATIVE_BEGIN;
            if (InstancedStruct.IsValid() && InstancedStruct.GetScriptStruct()->IsChildOf(ValueProp->Struct))
            {
                ValueProp->Struct->CopyScriptStruct(ValuePtr, InstancedStruct.GetMemory());
                return;
            }
            P_NATIVE_END;
        }

        if (Recursion == ECk_Recursion::NotRecursive)
        { return; }

        if (UCk_Utils_EntityLifetime_UE::Get_IsTransientEntity(CurrentHandle))
        { return; }

        CurrentHandle = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(CurrentHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------

DEFINE_FUNCTION(UCk_Utils_Variables_InstancedStruct_UE::execINTERNAL__Get_ByName_Exec)
{
    P_GET_STRUCT(FCk_Handle, Handle);
    P_GET_PROPERTY(FNameProperty, VariableName);
    P_GET_ENUM(ECk_Recursion, Recursion)
    P_GET_ENUM_REF(ECk_SucceededFailed, SucceededFailed)

    // Read wildcard Value input.
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.StepCompiledIn<FStructProperty>(nullptr);

    const auto* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
    void* ValuePtr = Stack.MostRecentPropertyAddress;

    P_FINISH;

    SucceededFailed = ECk_SucceededFailed::Failed;

    if (!ValueProp || !ValuePtr)
    {
        const FBlueprintExceptionInfo ExceptionInfo(
            EBlueprintExceptionType::AbortExecution,
            LOCTEXT("CkInstancedStructVariable_GetInvalidValueWarning", "Failed to resolve the Value for Get Value (By Name)"));

        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
    }

    auto CurrentHandle = Handle;

    while (ck::IsValid(CurrentHandle))
    {
        const auto& InstancedStruct = Get_ByName(CurrentHandle, VariableName, SucceededFailed);

        if (SucceededFailed == ECk_SucceededFailed::Succeeded)
        {
            P_NATIVE_BEGIN;
            if (InstancedStruct.IsValid() && InstancedStruct.GetScriptStruct()->IsChildOf(ValueProp->Struct))
            {
                ValueProp->Struct->CopyScriptStruct(ValuePtr, InstancedStruct.GetMemory());
                return;
            }
            P_NATIVE_END;
        }

        if (Recursion == ECk_Recursion::NotRecursive)
        { return; }

        if (UCk_Utils_EntityLifetime_UE::Get_IsTransientEntity(CurrentHandle))
        { return; }

        CurrentHandle = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(CurrentHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------

DEFINE_FUNCTION(UCk_Utils_Variables_InstancedStruct_UE::execINTERNAL__Get)
{
    P_GET_STRUCT(FCk_Handle, Handle);
    P_GET_STRUCT(FGameplayTag, VariableName);
    P_GET_ENUM(ECk_Recursion, Recursion)
    P_GET_ENUM_REF(ECk_SucceededFailed, SucceededFailed)

    // Read wildcard Value input.
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.StepCompiledIn<FStructProperty>(nullptr);

    const auto* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
    void* ValuePtr = Stack.MostRecentPropertyAddress;

    P_FINISH;

    SucceededFailed = ECk_SucceededFailed::Failed;

    if (!ValueProp || !ValuePtr)
    {
        const FBlueprintExceptionInfo ExceptionInfo(
            EBlueprintExceptionType::AbortExecution,
            LOCTEXT("CkInstancedStructVariable_GetInvalidValueWarning", "Failed to resolve the Value for Get Value (By GameplayTag)"));

        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
    }

    auto CurrentHandle = Handle;

    while (ck::IsValid(CurrentHandle))
    {
        const auto& InstancedStruct = Get(CurrentHandle, VariableName, SucceededFailed);

        if (SucceededFailed == ECk_SucceededFailed::Succeeded)
        {
            P_NATIVE_BEGIN;
            if (InstancedStruct.IsValid() && InstancedStruct.GetScriptStruct()->IsChildOf(ValueProp->Struct))
            {
                ValueProp->Struct->CopyScriptStruct(ValuePtr, InstancedStruct.GetMemory());
                SucceededFailed = ECk_SucceededFailed::Succeeded;
                return;
            }
            P_NATIVE_END;
        }

        if (Recursion == ECk_Recursion::NotRecursive)
        { return; }

        if (UCk_Utils_EntityLifetime_UE::Get_IsTransientEntity(CurrentHandle))
        { return; }

        CurrentHandle = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(CurrentHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------

DEFINE_FUNCTION(UCk_Utils_Variables_InstancedStruct_UE::execINTERNAL__Get_Exec)
{
    P_GET_STRUCT(FCk_Handle, Handle);
    P_GET_STRUCT(FGameplayTag, VariableName);
    P_GET_ENUM(ECk_Recursion, Recursion)
    P_GET_ENUM_REF(ECk_SucceededFailed, SucceededFailed)

    // Read wildcard Value input.
    Stack.MostRecentPropertyAddress = nullptr;
    Stack.MostRecentPropertyContainer = nullptr;
    Stack.StepCompiledIn<FStructProperty>(nullptr);

    const auto* ValueProp = CastField<FStructProperty>(Stack.MostRecentProperty);
    void* ValuePtr = Stack.MostRecentPropertyAddress;

    P_FINISH;

    SucceededFailed = ECk_SucceededFailed::Failed;

    if (!ValueProp || !ValuePtr)
    {
        const FBlueprintExceptionInfo ExceptionInfo(
            EBlueprintExceptionType::AbortExecution,
            LOCTEXT("CkInstancedStructVariable_GetInvalidValueWarning", "Failed to resolve the Value for Get Value (By GameplayTag)"));

        FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
    }

    auto CurrentHandle = Handle;

    while (ck::IsValid(CurrentHandle))
    {
        const auto& InstancedStruct = Get(CurrentHandle, VariableName, SucceededFailed);

        if (SucceededFailed == ECk_SucceededFailed::Succeeded)
        {
            P_NATIVE_BEGIN;
            if (InstancedStruct.IsValid() && InstancedStruct.GetScriptStruct()->IsChildOf(ValueProp->Struct))
            {
                ValueProp->Struct->CopyScriptStruct(ValuePtr, InstancedStruct.GetMemory());
                SucceededFailed = ECk_SucceededFailed::Succeeded;
                return;
            }
            P_NATIVE_END;
        }

        if (Recursion == ECk_Recursion::NotRecursive)
        { return; }

        if (UCk_Utils_EntityLifetime_UE::Get_IsTransientEntity(CurrentHandle))
        { return; }

        CurrentHandle = UCk_Utils_EntityLifetime_UE::Get_LifetimeOwner(CurrentHandle);
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCTEXT_NAMESPACE