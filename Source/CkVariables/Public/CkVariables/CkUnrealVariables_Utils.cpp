#include "CkUnrealVariables_Utils.h"

#include "CkVariables_Utils.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

// --------------------------------------------------------------------------------------------------------------------

#define CK_UTILS_VARIABLES_DEFINITION(_UtilsName_, _Type_)\
auto\
    _UtilsName_::\
    Get(\
        FCk_Handle InHandle,\
        FGameplayTag InVariableName)\
    -> _Type_\
{\
    return UtilsType::Get(InHandle, InVariableName);\
}\
\
auto\
    _UtilsName_::\
    Set(\
        FCk_Handle InHandle,\
        FGameplayTag InVariableName,\
        _Type_ InValue)\
    -> void\
{\
    UtilsType::Set(InHandle, InVariableName, InValue);\
}\
auto\
    _UtilsName_::\
    Get_ByName(\
        const FCk_Handle& InHandle,\
        FName InVariableName)\
    -> _Type_\
{\
    return UtilsType::Get(InHandle, InVariableName);\
}\
\
auto\
    _UtilsName_::\
    Set_ByName(\
        FCk_Handle& InHandle,\
        FName InVariableName,\
        _Type_ InValue)\
    -> void\
{\
    UtilsType::Set(InHandle, InVariableName, InValue);\
}\
auto\
    _UtilsName_::\
    Get_All(\
        const FCk_Handle& InHandle)\
    -> const TMap<FName, std::remove_cv_t<std::remove_reference_t<_Type_>>>&\
{\
    if (NOT UtilsType::Has(InHandle))\
    { static TMap<FName, std::remove_cv_t<std::remove_reference_t<_Type_>>> Invalid; return Invalid; }\
\
    return InHandle.Get<FragmentType>().Get_Variables();\
}

// --------------------------------------------------------------------------------------------------------------------

CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Bool_UE, bool)
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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Variables_UObject_UE::
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject)
    -> UObject*
{
    const auto& Var = UtilsType::Get(InHandle, InVariableName);
    if (ck::Is_NOT_Valid(Var))
    { return {}; }

    return Var.Get();
}

auto
    UCk_Utils_Variables_UObject_UE::
    Set(
        FCk_Handle InHandle,
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
        TSubclassOf<UObject> InObject)
    -> UObject*
{
    const auto& Var = UtilsType::Get(InHandle, InVariableName);
    if (ck::Is_NOT_Valid(Var))
    { return {}; }

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
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject)
    -> TSubclassOf<UObject>
{
    return UtilsType::Get(InHandle, InVariableName);
}

auto
    UCk_Utils_Variables_UObject_SubclassOf_UE::
    Set(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InValue)
    -> void
{
    return UtilsType::Set(InHandle, InVariableName, InValue);
}

auto
    UCk_Utils_Variables_UObject_SubclassOf_UE::
    Get_ByName(
        const FCk_Handle&    InHandle,
        FName                InVariableName,
        TSubclassOf<UObject> InObject)
    -> TSubclassOf<UObject>
{
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

