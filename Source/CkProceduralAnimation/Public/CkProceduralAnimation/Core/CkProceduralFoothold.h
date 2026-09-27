#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    enum class EProceduralFootholdSource : uint8
    {
        None,
        Ideal,
        Held,
        Front,
        Inward,
        Outward,
        Ring
    };

    enum class EProceduralFootholdVerdict : uint8
    {
        Usable,
        Miss,
        Unreachable,
        TooSteep,
        Occluded,
        Inboard,
        UnderBody
    };

    enum class EProceduralFootholdHold : uint8
    {
        None,
        Held
    };

    // How a candidate was found, and so how a hold on it is checked again: a down ray under it, or a face cast whose hit is
    // checked along its own normal (a down ray under a point on a vertical face runs along the face and never meets it).
    enum class EProceduralFootholdCast : uint8
    {
        Down,
        Face
    };

    // --------------------------------------------------------------------------------------------------------------------

    // A validated probe hit a leg might step to; position and normal are in the support frame.
    struct CKPROCEDURALANIMATION_API FProceduralFootholdCandidate
    {
        CK_GENERATED_BODY(FProceduralFootholdCandidate);

    private:
        FVector _Position = FVector::ZeroVector;
        FVector _Normal = FVector::UpVector;
        EProceduralFootholdSource _Source = EProceduralFootholdSource::None;
        EProceduralFootholdVerdict _Verdict = EProceduralFootholdVerdict::Miss;

    public:
        CK_PROPERTY(_Position);
        CK_PROPERTY(_Normal);
        CK_PROPERTY(_Source);
        CK_PROPERTY(_Verdict);

    public:
        CK_DEFINE_CONSTRUCTORS(FProceduralFootholdCandidate, _Position, _Normal, _Source, _Verdict);
    };

    // --------------------------------------------------------------------------------------------------------------------

    struct CKPROCEDURALANIMATION_API FProceduralFootholdSettings
    {
        CK_GENERATED_BODY(FProceduralFootholdSettings);

    private:
        float _SlopeWeight = 0.5f;
        float _ContinuityWeight = 0.25f;
        float _MaxAngleDegrees = 90.0f;

    public:
        CK_PROPERTY(_SlopeWeight);
        CK_PROPERTY(_ContinuityWeight);
        CK_PROPERTY(_MaxAngleDegrees);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // The leg's held foothold, in world space so a support-frame change cannot move it; re-validated every solve.
    // _SearchedSolve is the solve whose search last found nothing better than the target the leg already had, unset once
    // a search picks a new foothold.
    struct CKPROCEDURALANIMATION_API FProceduralFootholdState
    {
        CK_GENERATED_BODY(FProceduralFootholdState);

    private:
        FVector _Position = FVector::ZeroVector;
        FVector _Normal = FVector::UpVector;
        EProceduralFootholdSource _Source = EProceduralFootholdSource::None;
        EProceduralFootholdHold _Hold = EProceduralFootholdHold::None;
        EProceduralFootholdCast _Cast = EProceduralFootholdCast::Down;
        TOptional<uint64> _SearchedSolve;

    public:
        CK_PROPERTY(_Position);
        CK_PROPERTY(_Normal);
        CK_PROPERTY(_Source);
        CK_PROPERTY(_Hold);
        CK_PROPERTY(_Cast);
        CK_PROPERTY(_SearchedSolve);
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Whether InNormal (support frame) lies within InMaxAngleDegrees of +Z; a non-finite or zero normal does not.
    CKPROCEDURALANIMATION_API auto
        Get_IsFootholdLevelEnough(
            const FVector& InNormal,
            float InMaxAngleDegrees)
        -> bool;

    // Whether a point InHipToPoint from the hip lies on the leg's own side: no farther than InTolerance inboard of the hip
    // along InOutboard, the direction from the hip to its rest foot in the support plane. A foothold inboard of the hip folds
    // the leg under the body. A zero or non-finite InOutboard is a leg with no own side, so every point is on it; a
    // non-finite point or tolerance is not.
    CKPROCEDURALANIMATION_API auto
        Get_IsFootholdOnOwnSide(
            const FVector& InHipToPoint,
            const FVector& InOutboard,
            float InTolerance)
        -> bool;

    // Whether a point lies under the body: closer to the body's centreline than its widest hip less InMargin
    // (InLateralOffset, the point's support-frame Y from the body; InMaxHipLateral, the largest |Y| of the enabled hips),
    // and deeper below its leg's hip than a quarter of the leg's rest drop (InDepthBelowHip, positive downward). A foothold
    // there folds a leg whose rest direction runs along the body under the belly, which the own-side rule cannot see. A
    // body whose hips all lie on its centreline has no width to be under; any non-finite input is not under the body.
    CKPROCEDURALANIMATION_API auto
        Get_IsFootholdUnderBody(
            float InLateralOffset,
            float InDepthBelowHip,
            float InMaxHipLateral,
            float InRestDrop,
            float InMargin)
        -> bool;

    // |p - ideal| / reach + SlopeWeight x (1 - dot(n, +Z)) + (planted ? ContinuityWeight x |p.Z - plant.Z| / reach : 0), in
    // the support frame. A non-positive or non-finite reach costs the most a double holds.
    CKPROCEDURALANIMATION_API auto
        ComputeProceduralFootholdCost(
            const FProceduralFootholdCandidate& InCandidate,
            const FVector& InIdeal,
            const FVector& InPlant,
            bool InPlanted,
            float InReach,
            const FProceduralFootholdSettings& InSettings)
        -> double;

    // The index of the Usable candidate with the least cost, INDEX_NONE when none is Usable, the input is empty, InReach
    // is not positive or any input is non-finite. Ties go to the lower index. Never mutates its inputs.
    CKPROCEDURALANIMATION_API auto
        SelectProceduralFoothold(
            TArrayView<const FProceduralFootholdCandidate> InCandidates,
            const FVector& InIdeal,
            const FVector& InPlant,
            bool InPlanted,
            float InReach,
            const FProceduralFootholdSettings& InSettings)
        -> int32;
}

// --------------------------------------------------------------------------------------------------------------------
