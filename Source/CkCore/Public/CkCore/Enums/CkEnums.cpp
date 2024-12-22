#include "CkEnums.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Enum_UE::
    ConvertToECollisionEnabled(
        ECk_Collision InEnum)
    -> ECollisionEnabled::Type
{
    switch (InEnum)
    {
        case ECk_Collision::NoCollision:     return ECollisionEnabled::NoCollision;
        case ECk_Collision::QueryOnly:       return ECollisionEnabled::QueryOnly;
        case ECk_Collision::PhysicsOnly:     return ECollisionEnabled::PhysicsOnly;
        case ECk_Collision::QueryAndPhysics: return ECollisionEnabled::QueryAndPhysics;
        default:                             return ECollisionEnabled::NoCollision;
    }
}

auto
    UCk_Utils_Enum_UE::
    ConvertFromECollisionEnabled(
        ECollisionEnabled::Type InEnum)
    -> ECk_Collision
{
    switch (InEnum)
    {
        case ECollisionEnabled::NoCollision:     return ECk_Collision::NoCollision;
        case ECollisionEnabled::QueryOnly:       return ECk_Collision::QueryOnly;
        case ECollisionEnabled::PhysicsOnly:     return ECk_Collision::PhysicsOnly;
        case ECollisionEnabled::QueryAndPhysics: return ECk_Collision::QueryAndPhysics;
        default:                                 return ECk_Collision::NoCollision;
    }
}

// --------------------------------------------------------------------------------------------------------------------
