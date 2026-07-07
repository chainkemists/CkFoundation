#pragma once

#include "CkCore/Format/CkFormat.h"

#include "CkPool_Common.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Policies shared by both pooling flavors (EntityPool and ObjectPool)

UENUM(BlueprintType)
enum class ECk_Pool_ExhaustionPolicy : uint8
{
    // When the pool has no dormant/free instance: create a new one (subject to the CapacityPolicy).
    // EntityPool at Bounded capacity parks the acquire until the next Release; ObjectPool (synchronous)
    // returns null at capacity
    Grow,

    // When the pool has no dormant/free instance: fail immediately (EntityPool fulfills the promise with
    // Failed; ObjectPool returns null). The pool never creates instances on demand
    Fail
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pool_ExhaustionPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Pool_CapacityPolicy : uint8
{
    Unbounded,
    Bounded
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Pool_CapacityPolicy);

// --------------------------------------------------------------------------------------------------------------------
