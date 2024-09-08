#include "CkEnums.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Enum_UE::ConvertToECollisionEnabled(
        ECk_Collision InEnum)
        -> ECollisionEnabled::Type
{
    switch (InEnum)
    {
        case ECk_Collision::NoCollision:
            return ECollisionEnabled::NoCollision;
        case ECk_Collision::QueryOnly:
            return ECollisionEnabled::QueryOnly;
        case ECk_Collision::PhysicsOnly:
            return ECollisionEnabled::PhysicsOnly;
        case ECk_Collision::QueryAndPhysics:
            return ECollisionEnabled::QueryAndPhysics;
        default:
            return ECollisionEnabled::NoCollision; // Default case
    }
}

auto
    UCk_Utils_Enum_UE::ConvertFromECollisionEnabled(
        ECollisionEnabled::Type InEnum)
        -> ECk_Collision
{
    using EMyCollisionEnabled = ECk_Collision;

    switch (InEnum)
    {
        case ECollisionEnabled::NoCollision:
            return EMyCollisionEnabled::NoCollision;
        case ECollisionEnabled::QueryOnly:
            return EMyCollisionEnabled::QueryOnly;
        case ECollisionEnabled::PhysicsOnly:
            return EMyCollisionEnabled::PhysicsOnly;
        case ECollisionEnabled::QueryAndPhysics:
            return EMyCollisionEnabled::QueryAndPhysics;
        default:
            return EMyCollisionEnabled::NoCollision; // Default case
    }
}

// --------------------------------------------------------------------------------------------------------------------
