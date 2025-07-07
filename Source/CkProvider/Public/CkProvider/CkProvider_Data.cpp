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

auto
    UCk_Provider_GameplayTagContainer_PDA::
    Get_Value_Implementation(
        FCk_Handle InOptionalHandle) const
    -> FGameplayTagContainer
{
    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
	UCk_Provider_EntityScript_PDA::
	Get_Value_Implementation(
		FCk_Handle InOptionalLifetimeOwner) const
	-> FCk_Handle_PendingEntityScript
{
	return {};
}
// --------------------------------------------------------------------------------------------------------------------
