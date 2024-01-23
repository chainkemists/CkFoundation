#include "CkProvider_Data.h"

// --------------------------------------------------------------------------------------------------------------------
// Bool Provider
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Bool_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> bool
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Bool_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> bool
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Provider_Bool::
    FCk_Provider_Bool(
        UCk_Provider_Bool_PDA* InProvider)
    : _Provider(InProvider)
{
}

// --------------------------------------------------------------------------------------------------------------------
// Float Provider
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Float_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> float
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Float_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> float
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Provider_Float::
    FCk_Provider_Float(
        UCk_Provider_Float_PDA* InProvider)
    : _Provider(InProvider)
{
}

// --------------------------------------------------------------------------------------------------------------------
// Int Provider
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Int_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> int32
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Int_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> int32
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Provider_Int::
    FCk_Provider_Int(
        UCk_Provider_Int_PDA* InProvider)
    : _Provider(InProvider)
{
}

// --------------------------------------------------------------------------------------------------------------------
// Vector3 Provider
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Vector3_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FVector
{
    return {};
}

auto
    UCk_Provider_Vector3_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FVector
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Provider_Vector3::
    FCk_Provider_Vector3(
        UCk_Provider_Vector3_PDA* InProvider)
    : _Provider(InProvider)
{
}

// --------------------------------------------------------------------------------------------------------------------
// Vector2 Provider
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Vector2_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FVector2D
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Vector2_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FVector2D
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Provider_Vector2::
    FCk_Provider_Vector2(
        UCk_Provider_Vector2_PDA* InProvider)
    : _Provider(InProvider)
{
}

// --------------------------------------------------------------------------------------------------------------------
// Rotator Provider
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Rotator_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FRotator
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Rotator_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FRotator
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Provider_Rotator::
    FCk_Provider_Rotator(
        UCk_Provider_Rotator_PDA* InProvider)
    : _Provider(InProvider)
{
}

// --------------------------------------------------------------------------------------------------------------------
// Transform Provider
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Transform_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FTransform
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Transform_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FTransform
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Provider_Transform::
    FCk_Provider_Transform(
        UCk_Provider_Transform_PDA* InProvider)
    : _Provider(InProvider)
{
}

// --------------------------------------------------------------------------------------------------------------------
// Meter Provider
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Meter_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FCk_Meter
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_Meter_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FCk_Meter
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Provider_Meter::
    FCk_Provider_Meter(
        UCk_Provider_Meter_PDA* InProvider)
    : _Provider(InProvider)
{
}

// --------------------------------------------------------------------------------------------------------------------
// GameplayTag Provider
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_GameplayTag_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FGameplayTag
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Provider_GameplayTag::
    FCk_Provider_GameplayTag(
        UCk_Provider_GameplayTag_PDA* InProvider)
    : _Provider(InProvider)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_GameplayTag_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FGameplayTag
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------
// GameplayTagContainer Provider
// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_GameplayTagContainer_Literal_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FGameplayTagContainer
{
    return _Value;
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Provider_GameplayTagContainer::
    FCk_Provider_GameplayTagContainer(
        UCk_Provider_GameplayTagContainer_PDA* InProvider)
    : _Provider(InProvider)
{
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Provider_GameplayTagContainer_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FGameplayTagContainer
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------
