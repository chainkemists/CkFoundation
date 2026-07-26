#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// Defined in CkJolt/Body/CkJoltBody_Fragment_Data.h; forward-declared here so this header (pulled into the
// World include path) does not drag the Body reflection surface along with it.
enum class ECk_Jolt_SleepState : uint8;

// --------------------------------------------------------------------------------------------------------------------

/// Body activation/deactivation captured on a Jolt worker thread and queued for the game thread, because
/// ECS mutation is not thread-safe. Entity resolution is deliberately NOT done here — the raw body UserData
/// is carried verbatim and the consumer resolves it against its own registry.
struct CKJOLT_API FCk_Jolt_ActivationEvent
{
    // Stable while the body is alive — pose-buffer / liveness bookkeeping.
    uint32 BodyIndexAndSeq;

    uint64 UserData;

    // Awake for OnBodyActivated, Asleep for OnBodyDeactivated.
    ECk_Jolt_SleepState NewState;
};

// --------------------------------------------------------------------------------------------------------------------
