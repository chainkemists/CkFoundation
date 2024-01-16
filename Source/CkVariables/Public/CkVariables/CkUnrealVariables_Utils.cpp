#include "CkUnrealVariables_Utils.h"

#include "CkVariables_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

#define CK_UTILS_VARIABLES_DEFINITION(_UtilsName_, _Type_)\
auto\
    _UtilsName_::\
    Declare(\
        FCk_Handle InHandle,\
        FGameplayTag InVariableName,\
        _Type_ InDefaultValue)\
    -> void\
{\
    UtilsType::Add(InHandle);\
    UtilsType::Declare(InHandle, InVariableName, InDefaultValue);\
}\
\
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
}

// --------------------------------------------------------------------------------------------------------------------

CK_UTILS_VARIABLES_DEFINITION(UCk_Utils_Variables_Bool_UE, bool);
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
    Declare(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        UObject* InDefaultValue)
    -> void
{
    UtilsType::Add(InHandle);
    UtilsType::Declare(InHandle, InVariableName, InDefaultValue);
}

auto
    UCk_Utils_Variables_UObject_UE::
    Get(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InObject)
    -> UObject*
{
    return UtilsType::Get(InHandle, InVariableName);
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

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Variables_UObject_SubclassOf_UE::
    Declare(
        FCk_Handle InHandle,
        FGameplayTag InVariableName,
        TSubclassOf<UObject> InDefaultValue)
    -> void
{
    UtilsType::Add(InHandle);
    UtilsType::Declare(InHandle, InVariableName, InDefaultValue);
}

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

// --------------------------------------------------------------------------------------------------------------------

