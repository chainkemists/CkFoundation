#include "CkCVar_Data.h"

#include <UObject/UnrealType.h>

// --------------------------------------------------------------------------------------------------------------------
// FCk_CVarRef
// --------------------------------------------------------------------------------------------------------------------

FCk_CVarRef::
    FCk_CVarRef(
        FName InName)
    : _Name(InName)
{
}

auto
    FCk_CVarRef::
    IsValid() const
    -> bool
{
    return _Name != NAME_None;
}

auto
    FCk_CVarRef::
    Get_Name() const
    -> FName
{
    return _Name;
}

// --------------------------------------------------------------------------------------------------------------------
// FCk_CVarDefinition
// --------------------------------------------------------------------------------------------------------------------

FCk_CVarDefinition::
    FCk_CVarDefinition(
        FName InName,
        ECk_CVarType InType,
        const FString& InDefaultValue,
        const FString& InHelpText)
    : _Name(InName)
    , _Type(InType)
    , _DefaultValue(InDefaultValue)
    , _HelpText(InHelpText)
{
}

auto
    FCk_CVarDefinition::
    Get_Name() const
    -> FName
{
    return _Name;
}

auto
    FCk_CVarDefinition::
    Get_Type() const
    -> ECk_CVarType
{
    return _Type;
}

auto
    FCk_CVarDefinition::
    Get_DefaultValue() const
    -> const FString&
{
    return _DefaultValue;
}

auto
    FCk_CVarDefinition::
    Get_HelpText() const
    -> const FString&
{
    return _HelpText;
}

// --------------------------------------------------------------------------------------------------------------------
// FCk_CVarCallbackHandle
// --------------------------------------------------------------------------------------------------------------------

FCk_CVarCallbackHandle::
    FCk_CVarCallbackHandle(
        int32 InID)
    : _ID(InID)
{
}

auto
    FCk_CVarCallbackHandle::
    IsValid() const
    -> bool
{
    return _ID != INDEX_NONE;
}

auto
    FCk_CVarCallbackHandle::
    Get_ID() const
    -> int32
{
    return _ID;
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
