#include "CkCVar_Data.h"

#include <UObject/UnrealType.h>

// --------------------------------------------------------------------------------------------------------------------
// FCk_CVarRef
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_CVarRef::
    IsValid() const
    -> bool
{
    return _Name != NAME_None;
}

// --------------------------------------------------------------------------------------------------------------------
// FCk_CVarCallbackHandle
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_CVarCallbackHandle::
    IsValid() const
    -> bool
{
    return _ID != INDEX_NONE;
}

// --------------------------------------------------------------------------------------------------------------------
// FCk_CVar_DelegateSignatureHolder
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_CVar_DelegateSignatureHolder::
    GetSignatureFunctionForType(
        ECk_CVarType InType)
    -> UFunction*
{
    auto* Struct = StaticStruct();

    auto PropertyName = FName{};
    switch (InType)
    {
        case ECk_CVarType::Int32:  PropertyName = GET_MEMBER_NAME_CHECKED(FCk_CVar_DelegateSignatureHolder, _OnChanged_Int32);  break;
        case ECk_CVarType::Float:  PropertyName = GET_MEMBER_NAME_CHECKED(FCk_CVar_DelegateSignatureHolder, _OnChanged_Float);  break;
        case ECk_CVarType::Bool:   PropertyName = GET_MEMBER_NAME_CHECKED(FCk_CVar_DelegateSignatureHolder, _OnChanged_Bool);   break;
        case ECk_CVarType::String:  PropertyName = GET_MEMBER_NAME_CHECKED(FCk_CVar_DelegateSignatureHolder, _OnChanged_String);  break;
        case ECk_CVarType::Command: PropertyName = GET_MEMBER_NAME_CHECKED(FCk_CVar_DelegateSignatureHolder, _OnCommand);       break;
        default: return nullptr;
    }

    auto* Prop = CastField<FDelegateProperty>(Struct->FindPropertyByName(PropertyName));
    if (Prop == nullptr)
    {
        return nullptr;
    }

    return Prop->SignatureFunction;
}

// --------------------------------------------------------------------------------------------------------------------
