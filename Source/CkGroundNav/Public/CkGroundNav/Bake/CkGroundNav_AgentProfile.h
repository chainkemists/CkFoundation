#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkShapes/Box/CkShapeBox_Fragment_Data.h"
#include "CkShapes/Capsule/CkShapeCapsule_Fragment_Data.h"
#include "CkShapes/Cylinder/CkShapeCylinder_Fragment_Data.h"
#include "CkShapes/Sphere/CkShapeSphere_Fragment_Data.h"
#include "CkShapes/CkShapes_Common.h"

#include <CoreMinimal.h>

#include "CkGroundNav_AgentProfile.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * What one class of walker can stand on and climb.
 *
 * RADIUS IS DELIBERATELY ABSENT. Radius is answered at query time as a predicate against the
 * clearance field (clearance >= R), which is what lets ONE bake serve every agent size. Adding a
 * radius here would re-introduce per-radius baked fields, the approach this representation exists
 * to avoid.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNav_AgentProfile
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_GroundNav_AgentProfile);

private:
    // The authored standing volume. A shape type, not bare floats: this one IS authored gameplay data.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_AnyShape _StandingExtents;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _MaxSlopeDegrees = 45.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _MaxSlopeChangeDegrees = 30.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _StepHeightUu = 40.0f;

    // How readily the ledge filter demotes a span, expressed as the reciprocal of how many of a
    // span's four sides must drop away before it stops being standable ground.
    //
    // At the default 1.0 a SINGLE dropping side is enough — the most conservative setting, and the
    // one the constrain processor's correctness rests on. Lowering it demands more dropping sides
    // and is how narrow authored geometry survives: a one-cell catwalk drops away on both sides and
    // would otherwise be demoted along its entire length. Zero disables the filter outright.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _LedgeSensitivity = 1.0f;

    // Height delta, in unreal units, under which two neighbouring surfaces are treated as the same
    // rough surface and connect regardless of how sharply their normals disagree.
    //
    // Rasterizing a gently uneven floor produces alternating facet normals whose slope-change
    // exceeds any sane cone, which would shatter one walkable surface into disconnected fragments.
    // Zero applies the slope-change test everywhere with no relief.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _RoughPerchToleranceUu = 0.0f;

public:
    CK_PROPERTY_GET(_StandingExtents);
    CK_PROPERTY(_MaxSlopeDegrees);
    CK_PROPERTY(_MaxSlopeChangeDegrees);
    CK_PROPERTY(_StepHeightUu);
    CK_PROPERTY(_LedgeSensitivity);
    CK_PROPERTY(_RoughPerchToleranceUu);

public:
    /**
     * Full standing height in unreal units, derived from whichever shape was authored. Zero when no
     * shape was authored at all, which admission rejects.
     */
    auto Get_StandingHeightUu() const -> float;

public:
    CK_DEFINE_CONSTRUCTORS(FCk_GroundNav_AgentProfile, _StandingExtents);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    /**
     * Why an agent profile was refused at admission.
     *
     * A refused profile TERMINATES the bake with a status and publishes nothing. It is never
     * silently clamped into range: a clamped profile bakes a field that quietly disagrees with what
     * the caller asked for, and every downstream query then answers confidently and wrongly.
     */
    enum class EProfileRejection
    {
        None,
        SlopeOutOfRange,
        SlopeChangeOutOfRange,
        NegativeStepHeight,
        NegativeRoughPerchTolerance,
        NegativeLedgeSensitivity,
        NonFiniteValue,

        // No standing shape was authored, so the agent has no height. Headroom is undecidable, and a
        // bake that skipped the headroom test would report every crawlspace as walkable.
        MissingStandingExtents
    };

    /**
     * Validate one profile. Returns EProfileRejection::None when the profile is admissible.
     * Pure: no logging, no ensure, no side effect — the caller decides how loudly to fail.
     */
    CKGROUNDNAV_API auto
    Get_ProfileRejection(
        const FCk_GroundNav_AgentProfile& InProfile) -> EProfileRejection;
}

// --------------------------------------------------------------------------------------------------------------------
