#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

/// Contact event data captured during Jolt callbacks for deferred processing on the game thread.
/// This is necessary because Jolt's JobSystemThreadPool fires contact callbacks from worker threads,
/// and ECS mutations are not thread-safe. Entity resolution is deliberately NOT done here — the raw
/// body UserData (a versioned entity id baked in at body registration) is carried verbatim, and
/// consumers (ck::FCk_Jolt_ContactEventRouter registrants, routed by FProcessor_JoltWorld_DrainEvents)
/// resolve it against their own registry.
struct CKJOLT_API FCk_Jolt_ContactEvent
{
    enum class EType : uint8 { Added, Persisted, Removed };

    EType Type;

    // Entity identification (captured from body UserData during callback)
    uint64 Body1UserData;
    uint64 Body2UserData;

    // Contact geometry (only meaningful for Added/Persisted)
    TArray<FVector> ContactPointsOn1;
    TArray<FVector> ContactPointsOn2;
    FVector WorldSpaceNormal;

    // Body ID index+sequence for BodyIdToUserData map management (only for Added/Removed)
    uint32 Body1IndexAndSeq;
    uint32 Body2IndexAndSeq;
};

// --------------------------------------------------------------------------------------------------------------------
