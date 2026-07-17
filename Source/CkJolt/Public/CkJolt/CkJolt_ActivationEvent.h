#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

// ECk_Jolt_SleepState's full definition lives in CkJolt/Body/CkJoltBody_Fragment_Data.h. It is only
// forward-declared here (its size is fixed at uint8) so this header — pulled into the World include
// path via CkJoltWorld.h — does not drag the Body reflection surface along with it. The producer
// (CkBodyActivationListener) and the consumer (FProcessor_JoltBody_SleepStateMirror) include the full
// definition where they set / compare NewState.
enum class ECk_Jolt_SleepState : uint8;

// --------------------------------------------------------------------------------------------------------------------

/// Body activation/deactivation event captured during Jolt callbacks for deferred processing on the
/// game thread. Jolt's JobSystemThreadPool fires activation callbacks from worker threads, and ECS
/// mutations are not thread-safe, so events are queued and drained on the game thread. Mirrors
/// FCk_Jolt_ContactEvent: entity resolution is deliberately NOT done here — the raw body UserData
/// (a versioned entity id baked in at body registration) is carried verbatim and the consumer
/// (FProcessor_JoltBody_SleepStateMirror) resolves it against its own registry.
struct CKJOLT_API FCk_Jolt_ActivationEvent
{
    // Body ID index+sequence (stable while the body is alive) — for pose-buffer/liveness bookkeeping.
    uint32 BodyIndexAndSeq;

    // Entity identification (captured from body UserData during the callback).
    uint64 UserData;

    // Awake for OnBodyActivated, Asleep for OnBodyDeactivated.
    ECk_Jolt_SleepState NewState;
};

// --------------------------------------------------------------------------------------------------------------------
