#include "CkProceduralAnimation/Core/CkProceduralSurfaceMotion.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_surface_motion
{
    // A pending contact stays consistent while each new sighting's normal lies within this angle of the last one.
    constexpr auto CandidateConsistencyDegrees = 15.0;
    constexpr auto ConfirmTolerance = FCk_Time{1.0e-4};
    // The look-ahead probes this many clearances ahead of the body and accepts only the surface the body is already on:
    // a normal within LookAheadMaxDegrees of the support normal.
    constexpr auto LookAheadClearances = 1.5f;
    constexpr auto LookAheadMaxDegrees = 15.0;
    // Past a convex edge the down ray can see a lower floor while the surface the body just climbed onto lies ahead; the
    // look-ahead wins when its hit lies higher than the down hit by more than this share of the clearance.
    constexpr auto LookAheadRiseClearances = 0.25f;

    struct FContact
    {
        ck::FProceduralSurfaceHit Hit;
        ck::EProceduralSurfaceContactSource Source = ck::EProceduralSurfaceContactSource::None;
    };

    // The contact the rays propose for this substep and the down ray's own trusted hit. The support still holds while the
    // down ray meets a face along its length that it does not trust (grazed along the seam two solids share, or facing
    // away): that is not the missing support of a convex edge. A ray that starts on or inside a solid tells nothing about
    // the ground below and counts as a miss.
    struct FContacts
    {
        TOptional<FContact> Proposal;
        TOptional<FContact> Down;
        bool SupportHolds = false;
    };

    auto
        Get_IsTrusted(
            const ck::FProceduralSurfaceHit& InHit,
            const FVector& InRayDirection)
        -> bool
    {
        return InHit.Get_Hit() && InHit.Get_Fraction() > 0.0f && InHit.Get_Fraction() <= 1.0f
            && NOT InHit.Get_Position().ContainsNaN()
            && NOT InHit.Get_Normal().ContainsNaN() && NOT InHit.Get_Normal().IsNearlyZero()
            && FVector::DotProduct(InHit.Get_Normal(), InRayDirection) < -KINDA_SMALL_NUMBER;
    }

    auto
        Get_AngleDegrees(
            const FVector& InA,
            const FVector& InB)
        -> double
    {
        return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(InA.GetSafeNormal(), InB.GetSafeNormal()), -1.0, 1.0)));
    }

    auto
        DoCast(
            ck::FProceduralSurfaceRayCast InRayCast,
            const FVector& InStart,
            const FVector& InEnd,
            ck::EProceduralSurfaceContactSource InSource)
        -> TOptional<FContact>
    {
        const auto Hit = InRayCast(InStart, InEnd);
        if (NOT Get_IsTrusted(Hit, (InEnd - InStart).GetSafeNormal()))
        { return {}; }

        return FContact{Hit, InSource};
    }

    // The surface the body is on, one and a half clearances ahead of it; unset when that ray finds another surface.
    auto
        DoCast_LookAhead(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            ck::FProceduralSurfaceRayCast InRayCast,
            const FVector& InPosition,
            const FVector& InUp,
            const FVector& InForward)
        -> TOptional<FContact>
    {
        const auto Start = InPosition + InForward * (LookAheadClearances * InSettings.Get_Clearance());
        auto LookAhead = DoCast(InRayCast, Start, Start - InUp * InSettings.Get_ProbeReach(),
            ck::EProceduralSurfaceContactSource::LookAhead);
        if (LookAhead.IsSet() && Get_AngleDegrees(LookAhead->Hit.Get_Normal(), InUp) > LookAheadMaxDegrees)
        { return {}; }

        return LookAhead;
    }

    // Forward contact is used only within the body's clearance, so a distant wall cannot pull a creature off its floor.
    // Every candidate is a current query; held feet never masquerade as newly observed support normals. The forward ray and
    // the fan look where the body is going, so they are cast only while it moves; the down ray and the look-ahead hold the
    // surface it stands at, so a body stopped just past a crest keeps the top.
    auto
        DoFind_Contacts(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            ck::FProceduralSurfaceRayCast InRayCast,
            FCk_Time InStep,
            const FVector& InPosition,
            const FVector& InUp,
            const FVector& InForward,
            bool InMoving)
        -> FContacts
    {
        using ESource = ck::EProceduralSurfaceContactSource;
        const auto Clearance = InSettings.Get_Clearance();
        const auto Reach = InSettings.Get_ProbeReach();

        auto Contacts = FContacts{};
        const auto DownHit = InRayCast(InPosition + InUp * Clearance, InPosition - InUp * Reach);
        Contacts.SupportHolds = DownHit.Get_Hit() && DownHit.Get_Fraction() > 0.0f;
        if (Get_IsTrusted(DownHit, -InUp))
        { Contacts.Down = FContact{DownHit, ESource::Down}; }
        if (InMoving)
        { Contacts.Proposal = DoCast(InRayCast, InPosition, InPosition + InForward * Clearance, ESource::Forward); }
        if (Contacts.Proposal.IsSet())
        { return Contacts; }

        if (Contacts.Down.IsSet())
        {
            Contacts.Proposal = Contacts.Down;
            const auto DownHeight = FVector::DotProduct(InPosition - Contacts.Down->Hit.Get_Position(), InUp);
            const auto BodyWouldDescend = DownHeight > Clearance + InSettings.Get_ClearanceSpeed() * InStep.Get_Seconds();
            if (NOT BodyWouldDescend)
            { return Contacts; }

            const auto LookAhead = DoCast_LookAhead(InSettings, InRayCast, InPosition, InUp, InForward);
            if (LookAhead.IsSet()
                && FVector::DotProduct(InPosition - LookAhead->Hit.Get_Position(), InUp) < DownHeight - LookAheadRiseClearances * Clearance)
            { Contacts.Proposal = LookAhead; }
            return Contacts;
        }

        Contacts.Proposal = DoCast_LookAhead(InSettings, InRayCast, InPosition, InUp, InForward);
        if (Contacts.Proposal.IsSet() || NOT InMoving)
        { return Contacts; }

        // Recover around a convex edge from a bounded fan rather than extending a ray forever.
        const auto FanStart = InPosition + InForward * Clearance;
        const auto FanDirection = (-InUp - InForward).GetSafeNormal();
        Contacts.Proposal = DoCast(InRayCast, FanStart, FanStart + FanDirection * Reach, ESource::Fan);
        return Contacts;
    }

    // Counts a sighting of a contact whose normal turns beyond the confirm angle: consecutive sightings within
    // CandidateConsistencyDegrees of each other add up. Returns whether the contact is still pending after this one.
    auto
        DoSee_Candidate(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            const FContact& InContact,
            FCk_Time InStep,
            ck::FProceduralSurfaceMotionState& InOutState)
        -> bool
    {
        const auto Normal = InContact.Hit.Get_Normal().GetSafeNormal();
        const auto Consistent = InOutState.Get_CandidateSeen() > FCk_Time{}
            && Get_AngleDegrees(InOutState.Get_CandidateNormal(), Normal) <= CandidateConsistencyDegrees;
        InOutState.Set_CandidateSeen((Consistent ? InOutState.Get_CandidateSeen() : FCk_Time{}) + InStep);
        InOutState.Set_CandidateNormal(Normal);
        InOutState.Set_CandidatePoint(InContact.Hit.Get_Position());
        return InOutState.Get_CandidateSeen() + ConfirmTolerance < InSettings.Get_ConfirmTime();
    }

    // The first of the directions with a component in the plane, projected onto it; zero when none has one.
    auto
        DoGet_FirstInPlane(
            TArrayView<const FVector> InDirections,
            const FVector& InNormal)
        -> FVector
    {
        for (const auto& Direction : InDirections)
        {
            const auto InPlane = FVector::VectorPlaneProject(Direction, InNormal).GetSafeNormal();
            if (NOT InPlane.IsNearlyZero())
            { return InPlane; }
        }
        return FVector::ZeroVector;
    }

    // The steering projected onto the support plane, unless the projection is shorter than the steer floor's share of the
    // steering: on a face nearly perpendicular to the steering the projection's direction flips from facet to facet, so the
    // body keeps its travel tangent instead.
    auto
        DoGet_Forward(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            const ck::FProceduralSurfaceMotionState& InState,
            const FVector& InSteerDirection,
            const FQuat& InBodyRotation)
        -> FVector
    {
        const auto Up = InState.Get_SupportNormal();
        const auto Projected = FVector::VectorPlaneProject(InSteerDirection, Up);
        const auto SteerHolds = NOT Projected.IsNearlyZero()
            && Projected.Size() >= InSettings.Get_SteerFloor() * InSteerDirection.Size();
        if (SteerHolds)
        { return Projected.GetSafeNormal(); }

        const FVector Fallbacks[] = {InState.Get_TravelTangent(), InSteerDirection, InBodyRotation.GetAxisX()};
        return DoGet_FirstInPlane(Fallbacks, Up);
    }

    auto
        DoTurn(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            const FVector& InNormal,
            const FVector& InForward,
            double InStep,
            FTransform& InOutBody)
        -> void
    {
        const auto OldRotation = InOutBody.GetRotation();
        const auto TargetRotation = FRotationMatrix::MakeFromZX(InNormal, InForward).ToQuat();
        const auto Angle = OldRotation.AngularDistance(TargetRotation);
        const auto Alpha = Angle > KINDA_SMALL_NUMBER
            ? FMath::Min(1.0, FMath::DegreesToRadians(InSettings.Get_SurfaceTurnRateDegrees()) * InStep / Angle) : 1.0;
        InOutBody.SetRotation(FQuat::Slerp(OldRotation, TargetRotation, Alpha).GetNormalized());
    }

    // The contact becomes the support: the body turns toward its frame, carries the travel direction onto its plane and
    // eases its distance from the hit toward the clearance at the clearance speed.
    auto
        DoAdopt(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            const FContact& InContact,
            const FVector& InUp,
            const FVector& InForward,
            const FVector& InCandidate,
            double InStep,
            FTransform& InOutBody,
            ck::FProceduralSurfaceMotionState& InOutState)
        -> void
    {
        const auto Normal = InContact.Hit.Get_Normal().GetSafeNormal();
        const auto TargetForward = FQuat::FindBetweenNormals(InUp, Normal).RotateVector(InForward);
        InOutState.Set_SupportNormal(Normal);
        InOutState.Set_TravelTangent(TargetForward);
        InOutState.Set_ContactSource(InContact.Source);

        DoTurn(InSettings, Normal, TargetForward, InStep, InOutBody);

        const auto Height = FVector::DotProduct(InCandidate - InContact.Hit.Get_Position(), Normal);
        const auto MaxCorrection = InSettings.Get_ClearanceSpeed() * InStep;
        const auto Correction = FMath::Clamp(InSettings.Get_Clearance() - Height, -MaxCorrection, MaxCorrection);
        InOutBody.SetLocation(InCandidate + Normal * Correction);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        StepProceduralSurfaceMotion(
            const FProceduralSurfaceMotionSettings& InSettings,
            const FVector& InSteerDirection,
            float InSpeed,
            FCk_Time InStep,
            FProceduralSurfaceRayCast InRayCast,
            FTransform& InOutBody,
            FProceduralSurfaceMotionState& InOutState)
        -> void
    {
        const auto Step = InStep.Get_Seconds();
        const auto Clearance = InSettings.Get_Clearance();
        const auto OldRotation = InOutBody.GetRotation();
        const auto OldPosition = InOutBody.GetLocation();
        // Attachment owns the accepted surface frame. It must not use the visual body's partially eased up axis: doing so
        // alternates floor and wall hits during a corner turn.
        const auto Up = InOutState.Get_SupportNormal();
        const auto Forward = ck_procedural_surface_motion::DoGet_Forward(InSettings, InOutState, InSteerDirection, OldRotation);
        const auto Candidate = OldPosition + Forward * (InSpeed * Step);
        const auto Moving = InSpeed > 0.0f;

        const auto Contacts = ck_procedural_surface_motion::DoFind_Contacts(InSettings, InRayCast, InStep, Candidate, Up, Forward,
            Moving);
        if (Contacts.Proposal.IsSet())
        {
            const auto& Proposal = *Contacts.Proposal;
            const auto NeedsConfirmation = Contacts.SupportHolds
                && ck_procedural_surface_motion::Get_AngleDegrees(Up, Proposal.Hit.Get_Normal()) > InSettings.Get_ConfirmAngleDegrees();
            const auto Pending = NeedsConfirmation
                && ck_procedural_surface_motion::DoSee_Candidate(InSettings, Proposal, InStep, InOutState);
            if (NOT Pending)
            { InOutState.Set_CandidateSeen(FCk_Time{}); }

            InOutState.Set_ContactTrusted(true);
            InOutState.Set_MissingContact(FCk_Time{});
            InOutState.Set_Grounded(true);

            // While a large turn waits for confirmation the body keeps the support under it, or coasts along its plane
            // when the down ray proposed the turn or found nothing it trusts. A face the rays only graze for a few
            // substeps is never adopted; a body longer than its clearance can overrun a head-on wall by up to its speed
            // times the confirm time. A coast adopts no contact, so the source stays the one last accepted.
            const auto SupportUnder = Pending && Contacts.Down.IsSet()
                && ck_procedural_surface_motion::Get_AngleDegrees(Up, Contacts.Down->Hit.Get_Normal()) <= InSettings.Get_ConfirmAngleDegrees();
            if (Pending && NOT SupportUnder)
            {
                ck_procedural_surface_motion::DoTurn(InSettings, Up, Forward, Step, InOutBody);
                InOutBody.SetLocation(Candidate);
                InOutState.Set_TravelTangent(Forward);
            }
            else
            {
                ck_procedural_surface_motion::DoAdopt(InSettings, Pending ? *Contacts.Down : Proposal, Up, Forward, Candidate, Step,
                    InOutBody, InOutState);
            }
            InOutState.Set_Velocity((InOutBody.GetLocation() - OldPosition) / Step);
            return;
        }

        InOutState.Set_CandidateSeen(FCk_Time{});
        InOutState.Set_ContactTrusted(false);
        InOutState.Set_ContactSource(EProceduralSurfaceContactSource::None);
        InOutState.Set_MissingContact(InOutState.Get_MissingContact() + InStep);
        if (InOutState.Get_Grounded() && InOutState.Get_MissingContact() <= InSettings.Get_ContactGrace())
        {
            InOutBody.SetLocation(Candidate);
            InOutState.Set_Velocity(Forward * InSpeed);
            return;
        }

        InOutState.Set_Grounded(false);
        InOutState.Set_Velocity(InOutState.Get_Velocity() + InSettings.Get_Gravity() * Step);
        const auto FallEnd = OldPosition + InOutState.Get_Velocity() * Step;
        const auto FallDirection = (FallEnd - OldPosition).GetSafeNormal();
        const auto FallHit = InRayCast(OldPosition, FallEnd + FallDirection * Clearance);
        if (NOT ck_procedural_surface_motion::Get_IsTrusted(FallHit, FallDirection))
        {
            InOutBody.SetLocation(FallEnd);
            return;
        }

        // The landing keeps the heading the body travelled with: the tangent it had, laid onto the landing plane, then the
        // steering, then the body's own forward. The fall direction often runs along the landing normal and orients nothing.
        const auto Normal = FallHit.Get_Normal().GetSafeNormal();
        const FVector Headings[] = {InOutState.Get_TravelTangent(), InSteerDirection, OldRotation.GetAxisX()};
        const auto LandingForward = ck_procedural_surface_motion::DoGet_FirstInPlane(Headings, Normal);
        InOutBody.SetLocation(FallHit.Get_Position() + Normal * Clearance);
        InOutBody.SetRotation(FRotationMatrix::MakeFromZX(Normal, LandingForward).ToQuat());
        InOutState.Set_ContactTrusted(true);
        InOutState.Set_ContactSource(EProceduralSurfaceContactSource::Fall);
        InOutState.Set_Velocity(FVector::ZeroVector);
        InOutState.Set_Grounded(true);
        InOutState.Set_SupportNormal(Normal);
        InOutState.Set_TravelTangent(InOutBody.GetRotation().GetAxisX());
        InOutState.Set_MissingContact(FCk_Time{});
    }
}

// --------------------------------------------------------------------------------------------------------------------
