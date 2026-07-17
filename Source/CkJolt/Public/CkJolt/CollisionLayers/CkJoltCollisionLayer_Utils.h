#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkJolt/CollisionLayers/CkJoltCollisionLayer_Data.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt
{
    // Derives a Jolt collision signature from a UCollisionProfile name for one body domain — the same
    // per-profile derivation FCk_Jolt_CollisionLayerTable::Build_FromCollisionProfiles applies at seed time,
    // resolved by name here. Unset when the profile does not exist / has collision disabled. Shared by the
    // JoltBody and JoltCharacter setup processors (both place a body/character on a profile-derived layer).
    CKJOLT_API auto TryDerive_SignatureFromProfile(
        FName InProfileName,
        ECk_Jolt_BodyDomain InDomain) -> TOptional<FCk_Jolt_CollisionSignature>;
}

// --------------------------------------------------------------------------------------------------------------------
