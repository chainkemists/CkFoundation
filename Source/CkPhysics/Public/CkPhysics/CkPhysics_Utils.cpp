#pragma once

#include "CkPhysics_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"

#include <Components/PrimitiveComponent.h>
#include <Engine/CollisionProfile.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Physics_UE::
    Request_SetGenerateOverlapEvents(
        UPrimitiveComponent* InComp,
        ECk_EnableDisable InEnableDisable,
        const UObject* InContext)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InComp, ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("Cannot set Generate Overlap Events on component because it is invalid![{}]"), ck::Context(InContext))
    { return; }

    switch(InEnableDisable)
    {
        case ECk_EnableDisable::Enable:
        {
            InComp->SetGenerateOverlapEvents(true);
            InComp->UpdateOverlapsImpl();
            break;
        }
        case ECk_EnableDisable::Disable:
        {
            InComp->SetGenerateOverlapEvents(false);
            InComp->UpdateOverlapsImpl();
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InEnableDisable);
            break;
        }
    }
}

auto
    UCk_Utils_Physics_UE::
    Request_SetCollisionDetectionType(
        UPrimitiveComponent* InComp,
        ECk_CollisionDetectionType InCollisionType,
        const UObject* InContext)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InComp, ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("Cannot set Collision Detection Type on component because it is invalid![{}]"), ck::Context(InContext))
    { return; }

    switch(InCollisionType)
    {
        case ECk_CollisionDetectionType::Normal:
        {
            InComp->SetUseCCD(false);
            InComp->SetComponentTickEnabled(false);
            break;
        }
        case ECk_CollisionDetectionType::CCD:
        {
            InComp->SetUseCCD(true);
            InComp->SetSimulatePhysics(true);
            InComp->SetEnableGravity(false);
            break;

        }
        case ECk_CollisionDetectionType::CCD_All:
        {
            InComp->SetAllUseCCD(true);
            InComp->SetSimulatePhysics(true);
            InComp->SetEnableGravity(false);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InCollisionType);
            break;
        }
    }
}

auto
    UCk_Utils_Physics_UE::
    Request_SetNavigationEffects(
        UPrimitiveComponent* InComp,
        ECk_NavigationEffect InNavigationEffect,
        const UObject* InContext)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InComp, ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("Cannot set Navigation Effect on component because it is invalid![{}]"), ck::Context(InContext))
    { return; }

    switch(InNavigationEffect)
    {
        case ECk_NavigationEffect::DoesNotAffectNavigation:
        {
            InComp->SetCanEverAffectNavigation(false);
            break;
        }
        case ECk_NavigationEffect::AffectsNavigation:
        {
            InComp->SetCanEverAffectNavigation(true);
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InNavigationEffect);
            break;
        }
    }
}

auto
    UCk_Utils_Physics_UE::
    Request_SetOverlapBehavior(
        UPrimitiveComponent* InComp,
        ECk_ComponentOverlapBehavior InOverlapBehavior,
        const UObject* InContext)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InComp, ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("Cannot set Overlap Behavior on component because it is invalid![{}]"), ck::Context(InContext))
    { return; }

    switch(InOverlapBehavior)
    {
        case ECk_ComponentOverlapBehavior::OtherActorComponents:
        {
            InComp->IgnoreActorWhenMoving(InComp->GetOwner(), true);
            break;
        }
        case ECk_ComponentOverlapBehavior::AllActorComponents:
        {
            break;
        }
        default:
        {
            CK_INVALID_ENUM(InOverlapBehavior);
            break;
        }
    }
}

auto
    UCk_Utils_Physics_UE::
    Request_SetCollisionProfileName(
        UPrimitiveComponent* InComp,
        FName                InCollisionProfileName,
        const UObject*       InContext)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InComp, ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("Cannot set Collision Profile Name on component because it is invalid![{}]"), ck::Context(InContext))
    { return; }

    CK_ENSURE_IF_NOT(Get_IsValidCollisionProfileName(InCollisionProfileName),
        TEXT("Trying to set Collision Profile Name [{}] on component [{}] that does NOT exist in the list of collision profiles.[{}]"),
        InCollisionProfileName,
        InComp,
        ck::Context(InContext))
    { return; }

    InComp->SetCollisionProfileName(InCollisionProfileName);
}

auto
    UCk_Utils_Physics_UE::
    Get_IsValidCollisionProfileName(
        FName          InCollisionProfileName,
        const UObject* InContext)
    -> bool
{
    TArray<TSharedPtr<FName>> OutExistingProfileNames;

    UCollisionProfile::GetProfileNames(OutExistingProfileNames);

    return OutExistingProfileNames.ContainsByPredicate([&](TSharedPtr<FName> InProfileName) -> bool
    {
        return ck::IsValid(InProfileName) && InProfileName->IsEqual(InCollisionProfileName);
    });
}

auto
    UCk_Utils_Physics_UE::
    Get_CollisionProfileName(
        const UPrimitiveComponent* InComp,
        const UObject*             InContext)
    -> FName
{
    CK_ENSURE_IF_NOT(ck::IsValid(InComp, ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("Cannot get Collision Profile Name of component because it is invalid![{}]"), ck::Context(InContext))
    { return {}; }

    return InComp->GetCollisionProfileName();
}

auto
    UCk_Utils_Physics_UE::
    Request_SetCollisionEnabled(
        UPrimitiveComponent*                 InComp,
        TEnumAsByte<ECollisionEnabled::Type> InCollisionEnabled,
        const UObject*                       InContext)
    -> void
{
    CK_ENSURE_IF_NOT(ck::IsValid(InComp, ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("Cannot set Collision Enabled on component because it is invalid![{}]"), ck::Context(InContext))
    { return; }

    InComp->SetCollisionEnabled(InCollisionEnabled);
}

auto
    UCk_Utils_Physics_UE::
    Make_BoxShapeDimensions(
        const FCk_BoxExtents& InBoxExtents)
    -> FCk_ShapeDimensions
{
    return FCk_ShapeDimensions{InBoxExtents};
}

auto
    UCk_Utils_Physics_UE::
    Make_SphereShapeDimensions(
        const FCk_SphereRadius& InSphereRadius)
    -> FCk_ShapeDimensions
{
    return FCk_ShapeDimensions{InSphereRadius};
}

auto
    UCk_Utils_Physics_UE::
    Make_CapsuleShapeDimensions(
        const FCk_CapsuleSize& InCapsuleSize)
    -> FCk_ShapeDimensions
{
    return FCk_ShapeDimensions{InCapsuleSize};
}

// --------------------------------------------------------------------------------------------------------------------

