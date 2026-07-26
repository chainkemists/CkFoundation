#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

/// Contact data captured on a Jolt worker thread and queued for the game thread, because ECS mutation is
/// not thread-safe. Entity resolution is deliberately NOT done here — the raw body UserData is carried
/// verbatim and consumers (ck::FCk_Jolt_ContactEventRouter registrants) resolve it themselves.
struct CKJOLT_API FCk_Jolt_ContactEvent
{
    enum class EType : uint8 { Added, Persisted, Removed };

    EType Type;

    uint64 Body1UserData;
    uint64 Body2UserData;

    // Contact geometry (only meaningful for Added/Persisted)
    TArray<FVector> ContactPointsOn1;
    TArray<FVector> ContactPointsOn2;
    FVector WorldSpaceNormal;

    // Contact detail (only meaningful for Added/Persisted; defaults left for Removed).
    bool IsSensor1 = false;
    bool IsSensor2 = false;
    float PenetrationDepth = 0.0f;
    float RelativeNormalVelocity = 0.0f;

    // Populated for ALL event types — consumers disambiguate multi-body entities against their own body id.
    uint32 Body1IndexAndSeq;
    uint32 Body2IndexAndSeq;
};

// --------------------------------------------------------------------------------------------------------------------
