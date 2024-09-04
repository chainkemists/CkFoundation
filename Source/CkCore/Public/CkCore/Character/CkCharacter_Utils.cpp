#include "CkCharacter_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Character_UE::
    Add_CharacterHalfHeightToLocation(
        ACharacter* InCharacter,
        FVector InLocation)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCharacter),
        TEXT("Invalid Character supplied to UCk_Utils_Character_UE::Add_CharacterHalfHeightToLocation"))
    { return InLocation; }

    return Add_CapsuleHalfHeightToLocation(InCharacter->GetCapsuleComponent(), InLocation);
}

auto
    UCk_Utils_Character_UE::
    Subtract_CharacterHalfHeightFromLocation(
        ACharacter* InCharacter,
        FVector InLocation)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCharacter),
        TEXT("Invalid Character supplied to UCk_Utils_Character_UE::Subtract_CharacterHalfHeightFromLocation"))
    { return InLocation; }

    return Subtract_CapsuleHalfHeightFromLocation(InCharacter->GetCapsuleComponent(), InLocation);
}

auto
    UCk_Utils_Character_UE::
    Add_CapsuleHalfHeightToLocation(
        UCapsuleComponent* InCapsule,
        FVector InLocation)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCapsule),
        TEXT("Invalid Capsule supplied to UCk_Utils_Character_UE::Add_CapsuleHalfHeightToLocation"))
    { return InLocation; }

    const auto& HalfHeight = InCapsule->GetComponentRotation().RotateVector(FVector{0, 0, InCapsule->GetScaledCapsuleHalfHeight()});

    return InLocation + HalfHeight;
}

auto
    UCk_Utils_Character_UE::
    Subtract_CapsuleHalfHeightFromLocation(
        UCapsuleComponent* InCapsule,
        FVector InLocation)
    -> FVector
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCapsule),
        TEXT("Invalid Capsule supplied to UCk_Utils_Character_UE::Subtract_CapsuleHalfHeightFromLocation"))
    { return InLocation; }

    const auto& HalfHeight = InCapsule->GetComponentRotation().RotateVector(FVector{0, 0, InCapsule->GetScaledCapsuleHalfHeight()});

    return InLocation - HalfHeight;
}

// --------------------------------------------------------------------------------------------------------------------
