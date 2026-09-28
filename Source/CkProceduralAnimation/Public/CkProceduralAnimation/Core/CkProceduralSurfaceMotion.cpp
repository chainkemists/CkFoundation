#include "CkProceduralAnimation/Core/CkProceduralSurfaceMotion.h"

#include "CkProceduralAnimation/Core/CkProceduralFeetPlane.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_procedural_surface_motion
{
    // A pending contact stays consistent while each new sighting's normal lies within this angle of the last one.
    constexpr auto CandidateConsistencyDegrees = 15.0;
    constexpr auto ConfirmTolerance = FCk_Time{1.0e-4};
    constexpr auto LookAheadClearances = 1.5f;
    // A surface whose normal lies within this angle of the support normal is the surface the body is already on: the only
    // one the look-ahead accepts, and the one a contact's free ray may meet at a concave corner.
    constexpr auto SameSurfaceMaxDegrees = 15.0;
    // Past a convex edge the down ray can see a lower floor while the surface the body just climbed onto lies ahead; the
    // look-ahead wins when its hit lies higher than the down hit by more than this share of the clearance.
    constexpr auto LookAheadRiseClearances = 0.25f;
    // The planted feet's plane counts only over their footprint grown by this share of the clearance on every side: the
    // feet support the ground within them, and an unbounded plane would carry the body off a ledge before the fan ran.
    constexpr auto FeetFootprintMarginClearances = 0.25;
    // Over one plane the feet contact and the down ray's hit agree to rounding; the feet contact replaces the ray's only
    // when it lies higher by more than this, so on plain ground the ray keeps the contact.
    constexpr auto FeetAboveRayTolerance = 0.01;
    constexpr auto StepProbeAheadClearances = 0.25f;
    // A ray that starts on the contact's own surface can report that surface at fraction 0, so a contact's free ray starts
    // this far off it.
    constexpr auto RoomRayStartOffset = 1.0;
    // A free ray that meets the surface the body stands on within this share of the clearance leaves a concave corner, a
    // legitimate adoption.
    constexpr auto RoomCornerClearances = 0.25;
    // A body in a gap narrower than its clearance stands off the face it cannot adopt by this share of the room, the gap's
    // middle.
    constexpr auto NarrowRoomStandoffShare = 0.5;

    struct FContact
    {
        ck::FProceduralSurfaceHit Hit;
        ck::EProceduralSurfaceContactSource Source = ck::EProceduralSurfaceContactSource::None;
        // The free space off the contact along its normal, up to the clearance, once measured.
        TOptional<double> Room;
    };

    // A face the body is kept off: the substep's travel into it is taken away and the body eases out to the standoff.
    struct FObstruction
    {
        FVector Point = FVector::ZeroVector;
        FVector Normal = FVector::UpVector;
        double Standoff = 0.0;
    };

    // The contact the rays propose for this substep, the down contact under the body, and where they were cast from: the
    // old position plus the travel (with its component into any obstruction taken away) plus the push out of it. The support
    // still holds while the down ray meets a face along its length that it does not trust (grazed along the seam two solids
    // share, or facing away): that is not the missing support of a convex edge. A ray that starts on or inside a solid tells
    // nothing about the ground below and counts as a miss.
    struct FContacts
    {
        TOptional<FContact> Proposal;
        TOptional<FContact> Down;
        bool SupportHolds = false;
        FVector Travel = FVector::ZeroVector;
        FVector Candidate = FVector::ZeroVector;
        TOptional<FObstruction> Obstruction;
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
        if (LookAhead.IsSet() && Get_AngleDegrees(LookAhead->Hit.Get_Normal(), InUp) > SameSurfaceMaxDegrees)
        { return {}; }

        return LookAhead;
    }

    auto
        Get_Height(
            const FContact& InContact,
            const FVector& InPosition,
            const FVector& InUp)
        -> double
    {
        return FVector::DotProduct(InPosition - InContact.Hit.Get_Position(), InUp);
    }

    // The free space off the contact along its normal, where the body would stand: the clearance when a free ray from the
    // hit meets nothing within it, or only the surface the body stands on within a quarter clearance (a concave corner);
    // otherwise the distance to the solid it meets. The planted feet's contact is no hit on a solid and always has the
    // clearance. One ray, cast once per contact.
    auto
        Get_Room(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            ck::FProceduralSurfaceRayCast InRayCast,
            FContact& InOutContact,
            const FVector& InUp)
        -> double
    {
        const auto Clearance = static_cast<double>(InSettings.Get_Clearance());
        if (InOutContact.Source == ck::EProceduralSurfaceContactSource::Feet)
        { return Clearance; }
        if (InOutContact.Room.IsSet())
        { return *InOutContact.Room; }

        const auto Normal = InOutContact.Hit.Get_Normal().GetSafeNormal();
        const auto& Point = InOutContact.Hit.Get_Position();
        const auto Blocker = InRayCast(Point + Normal * RoomRayStartOffset, Point + Normal * Clearance);
        const auto BlockerDistance = FVector::Distance(Blocker.Get_Position(), Point);
        const auto ConcaveCorner = Blocker.Get_Hit() && Blocker.Get_Fraction() > 0.0f
            && BlockerDistance <= RoomCornerClearances * Clearance
            && Get_AngleDegrees(Blocker.Get_Normal(), InUp) <= SameSurfaceMaxDegrees;
        InOutContact.Room = NOT Blocker.Get_Hit() || ConcaveCorner ? Clearance : FMath::Min(BlockerDistance, Clearance);
        return *InOutContact.Room;
    }

    auto
        Get_HasRoom(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            ck::FProceduralSurfaceRayCast InRayCast,
            FContact& InOutContact,
            const FVector& InUp)
        -> bool
    {
        return Get_Room(InSettings, InRayCast, InOutContact, InUp) >= static_cast<double>(InSettings.Get_Clearance());
    }

    // A remote tangent-plane contact can hold the body off a curved surface even though its own down ray misses.
    // Localize the observed patch with one bounded ray from the body. A perpendicular crest side or a contact without
    // room does not replace the original top; keeping the complete original contact preserves crest bridging.
    auto
        DoLocalize_LookAhead(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            ck::FProceduralSurfaceRayCast InRayCast,
            const FVector& InPosition,
            const FVector& InUp,
            FContact& InOutContact)
        -> void
    {
        const auto AheadNormal = InOutContact.Hit.Get_Normal().GetSafeNormal();
        const auto Aim = InOutContact.Hit.Get_Position() - AheadNormal * InSettings.Get_Clearance();
        const auto Delta = Aim - InPosition;
        const auto Distance = Delta.Size();
        if (Distance <= UE_DOUBLE_SMALL_NUMBER)
        { return; }

        const auto End = InPosition + Delta * (FMath::Min(Distance, static_cast<double>(InSettings.Get_ProbeReach())) / Distance);
        auto Local = DoCast(InRayCast, InPosition, End, ck::EProceduralSurfaceContactSource::LookAhead);
        if (Local.IsSet()
            && FVector::DotProduct(Local->Hit.Get_Normal().GetSafeNormal(), AheadNormal) > UE_KINDA_SMALL_NUMBER
            && Get_HasRoom(InSettings, InRayCast, *Local, InUp))
        { InOutContact = *Local; }
    }

    // The face as an obstruction: kept off at the clearance, or, when a body one clearance off it would stand inside the
    // solid across the gap, at the gap's middle.
    auto
        MakeObstruction(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            const FContact& InFace,
            double InRoom)
        -> FObstruction
    {
        const auto Clearance = static_cast<double>(InSettings.Get_Clearance());
        return FObstruction{InFace.Hit.Get_Position(), InFace.Hit.Get_Normal().GetSafeNormal(),
            InRoom >= Clearance ? Clearance : NarrowRoomStandoffShare * InRoom};
    }

    // Keeps the candidate off the obstruction: once it would stand closer than the standoff, the travel loses its component
    // into the face (a glancing face slows the body, a head-on one stops it) and the body is pushed back out along the
    // normal by at most the clearance speed's step, so a face first seen close eases the body out instead of jumping it.
    auto
        DoKeep_Standoff(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            const FObstruction& InObstruction,
            const FVector& InOldPosition,
            double InStep,
            FContacts& InOutContacts)
        -> void
    {
        InOutContacts.Obstruction = InObstruction;
        const auto& Normal = InObstruction.Normal;
        if (FVector::DotProduct(InOutContacts.Candidate - InObstruction.Point, Normal) >= InObstruction.Standoff)
        { return; }

        const auto Push = InOutContacts.Candidate - InOldPosition - InOutContacts.Travel;
        const auto Into = FVector::DotProduct(InOutContacts.Travel, Normal);
        if (Into < 0.0)
        { InOutContacts.Travel -= Normal * Into; }
        InOutContacts.Candidate = InOldPosition + InOutContacts.Travel + Push;

        const auto OffFace = FVector::DotProduct(InOutContacts.Candidate - InObstruction.Point, Normal);
        if (OffFace < InObstruction.Standoff)
        { InOutContacts.Candidate += Normal * FMath::Min(InObstruction.Standoff - OffFace, InSettings.Get_ClearanceSpeed() * InStep); }
    }

    // The top of a face the forward ray met, when it is a step: a trusted hit of a down ray from the max step height plus a
    // clearance above the face hit, a quarter clearance past it, whose top lies above the support point by at most the max
    // step height and faces within the confirm angle of the support normal.
    auto
        DoCast_Step(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            ck::FProceduralSurfaceRayCast InRayCast,
            const FContact& InFace,
            const FVector& InSupportPoint,
            const FVector& InUp,
            const FVector& InForward)
        -> TOptional<FContact>
    {
        const auto Clearance = InSettings.Get_Clearance();
        const auto MaxStepHeight = InSettings.Get_MaxStepHeight();
        const auto Start = InFace.Hit.Get_Position() + InForward * (StepProbeAheadClearances * Clearance) + InUp * (MaxStepHeight + Clearance);
        auto Top = DoCast(InRayCast, Start, Start - InUp * (MaxStepHeight + 2.0f * Clearance), ck::EProceduralSurfaceContactSource::Step);
        if (NOT Top.IsSet())
        { return {}; }

        const auto Rise = FVector::DotProduct(Top->Hit.Get_Position() - InSupportPoint, InUp);
        const auto WithinStep = Rise > 0.0 && Rise <= MaxStepHeight
            && Get_AngleDegrees(Top->Hit.Get_Normal(), InUp) <= InSettings.Get_ConfirmAngleDegrees();
        if (NOT WithinStep)
        { return {}; }

        return Top;
    }

    // The planted feet's contact under the candidate: on their plane straight down the support normal, carrying InNormal.
    // None outside the footprint grown by FeetFootprintMarginClearances, or for a plane too steep for the support (the body
    // turning onto a wall its feet have not reached yet).
    auto
        DoGet_FeetContact(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            const TOptional<ck::FProceduralSurfaceFeetSupport>& InFeetSupport,
            const FVector& InPosition,
            const FVector& InUp,
            const FVector& InNormal)
        -> TOptional<FContact>
    {
        if (NOT InFeetSupport.IsSet())
        { return {}; }

        const auto& Feet = *InFeetSupport;
        const auto Local = Feet.Get_Basis().UnrotateVector(InPosition - Feet.Get_Origin());
        const auto Margin = FeetFootprintMarginClearances * InSettings.Get_Clearance();
        const auto InsideFootprint = Local.X >= Feet.Get_FootprintMin().X - Margin && Local.X <= Feet.Get_FootprintMax().X + Margin
            && Local.Y >= Feet.Get_FootprintMin().Y - Margin && Local.Y <= Feet.Get_FootprintMax().Y + Margin;
        if (NOT InsideFootprint)
        { return {}; }

        const auto Normal = Feet.Get_Normal().GetSafeNormal();
        const auto Alignment = FVector::DotProduct(InUp, Normal);
        const auto RideableFromHere = Alignment >= FMath::Cos(FMath::DegreesToRadians(static_cast<double>(ck::ProceduralFeetPlaneMaxAngleDegrees)));
        if (NOT RideableFromHere)
        { return {}; }

        const auto Height = FVector::DotProduct(InPosition - Feet.Get_Point(), Normal) / Alignment;
        const auto OnPlane = ck::FProceduralSurfaceHit{}
            .Set_Hit(true)
            .Set_Position(InPosition - InUp * Height)
            .Set_Normal(InNormal);
        return FContact{OnPlane, ck::EProceduralSurfaceContactSource::Feet};
    }

    // Forward contact is used only within the body's clearance, so a distant wall cannot pull a creature off its floor.
    // Every candidate is a current query; held feet never masquerade as newly observed support normals. The forward ray and
    // the fan look where the body is going, so they are cast only while it moves; the down ray and the look-ahead hold the
    // surface it stands at, so a body stopped just past a crest keeps the top. A body kept off a wall on the last substep
    // and still steered into it follows the wall with one ray along its normal. A face the forward ray meets is a step when
    // its top lies within the max step height, and the step's top becomes the down contact; otherwise it is a wall, which
    // Climb proposes when the body has room on it; a wall under Slide, or without room, obstructs the body, and the down ray
    // is cast again from where the obstruction left it. The planted feet's contact stands in for the down contact when it
    // lies higher, and keeps the support holding over a down miss. It carries the down contact's normal when there is one,
    // so a crease the feet span still turns the body onto the facet under it, and the feet plane's normal over a miss, where
    // the ray tells nothing about the surface and the feet do: on a convex wall the body's support would otherwise stay on
    // the facet it last saw while the body walks round, and its rays miss the wall for good. A contact with no room off it
    // is never proposed, and the fan's face without room obstructs the body as the forward ray's does.
    auto
        DoFind_Contacts(
            const ck::FProceduralSurfaceMotionSettings& InSettings,
            ck::FProceduralSurfaceRayCast InRayCast,
            const TOptional<ck::FProceduralSurfaceFeetSupport>& InFeetSupport,
            const ck::FProceduralSurfaceMotionState& InState,
            FCk_Time InStep,
            const FVector& InOldPosition,
            const FVector& InTravel,
            const FVector& InUp,
            const FVector& InForward,
            bool InMoving)
        -> FContacts
    {
        using ESource = ck::EProceduralSurfaceContactSource;
        const auto Clearance = InSettings.Get_Clearance();
        const auto Reach = InSettings.Get_ProbeReach();
        const auto StepSeconds = InStep.Get_Seconds();

        auto Contacts = FContacts{};
        Contacts.Travel = InTravel;
        Contacts.Candidate = InOldPosition + InTravel;

        const auto& FollowedNormal = InState.Get_ObstructionNormal();
        const auto FollowsWall = InMoving && InState.Get_Obstruction() == ck::EProceduralSurfaceObstruction::Wall
            && FVector::DotProduct(InForward, FollowedNormal) < 0.0;
        if (FollowsWall)
        {
            const auto Wall = DoCast(InRayCast, Contacts.Candidate, Contacts.Candidate - FollowedNormal * Reach, ESource::None);
            if (Wall.IsSet())
            {
                DoKeep_Standoff(InSettings, FObstruction{Wall->Hit.Get_Position(), Wall->Hit.Get_Normal().GetSafeNormal(),
                    InState.Get_ObstructionStandoff()}, InOldPosition, StepSeconds, Contacts);
            }
        }

        const auto CastDown = [&]() -> void
        {
            const auto DownHit = InRayCast(Contacts.Candidate + InUp * Clearance, Contacts.Candidate - InUp * Reach);
            Contacts.SupportHolds = DownHit.Get_Hit() && DownHit.Get_Fraction() > 0.0f;
            Contacts.Down.Reset();
            if (Get_IsTrusted(DownHit, -InUp))
            { Contacts.Down = FContact{DownHit, ESource::Down}; }
        };
        CastDown();

        if (InMoving)
        {
            auto Face = DoCast(InRayCast, Contacts.Candidate, Contacts.Candidate + InForward * Clearance, ESource::Forward);
            if (Face.IsSet())
            {
                const auto SupportPoint = Contacts.Down.IsSet() ? Contacts.Down->Hit.Get_Position() : Contacts.Candidate - InUp * Clearance;
                auto StepTop = InSettings.Get_MaxStepHeight() > 0.0f
                    ? DoCast_Step(InSettings, InRayCast, *Face, SupportPoint, InUp, InForward)
                    : TOptional<FContact>{};
                if (StepTop.IsSet() && Get_HasRoom(InSettings, InRayCast, *StepTop, InUp))
                {
                    Contacts.Down = StepTop;
                    Contacts.SupportHolds = true;
                }
                else
                {
                    const auto Room = Get_Room(InSettings, InRayCast, *Face, InUp);
                    const auto Climbs = InSettings.Get_WallPolicy() == ck::EProceduralSurfaceWallPolicy::Climb && Room >= Clearance;
                    if (Climbs)
                    { Contacts.Proposal = Face; }
                    else
                    {
                        const auto Before = Contacts.Candidate;
                        DoKeep_Standoff(InSettings, MakeObstruction(InSettings, *Face, Room), InOldPosition, StepSeconds, Contacts);
                        if (Contacts.Candidate != Before)
                        { CastDown(); }
                    }
                }
            }
        }

        const auto Feet = DoGet_FeetContact(InSettings, InFeetSupport, Contacts.Candidate, InUp,
            Contacts.Down.IsSet() ? Contacts.Down->Hit.Get_Normal() : (InFeetSupport.IsSet() ? InFeetSupport->Get_Normal() : InUp));
        if (Feet.IsSet())
        {
            Contacts.SupportHolds = true;
            const auto FeetAboveRay = NOT Contacts.Down.IsSet()
                || Get_Height(*Feet, Contacts.Candidate, InUp) < Get_Height(*Contacts.Down, Contacts.Candidate, InUp) - FeetAboveRayTolerance;
            if (FeetAboveRay)
            { Contacts.Down = Feet; }
        }

        if (Contacts.Proposal.IsSet())
        { return Contacts; }

        auto LookAhead = TOptional<FContact>{};
        auto LookAheadCast = false;
        const auto CastLookAhead = [&]() -> TOptional<FContact>&
        {
            if (NOT LookAheadCast)
            {
                LookAhead = DoCast_LookAhead(InSettings, InRayCast, Contacts.Candidate, InUp, InForward);
                if (LookAhead.IsSet())
                { DoLocalize_LookAhead(InSettings, InRayCast, Contacts.Candidate, InUp, *LookAhead); }
                LookAheadCast = true;
            }
            return LookAhead;
        };

        if (Contacts.Down.IsSet())
        {
            const auto DownHeight = FVector::DotProduct(Contacts.Candidate - Contacts.Down->Hit.Get_Position(), InUp);
            const auto BodyWouldDescend = DownHeight > Clearance + InSettings.Get_ClearanceSpeed() * StepSeconds;
            if (BodyWouldDescend)
            {
                auto& Ahead = CastLookAhead();
                const auto AheadHigher = Ahead.IsSet()
                    && FVector::DotProduct(Contacts.Candidate - Ahead->Hit.Get_Position(), InUp) < DownHeight - LookAheadRiseClearances * Clearance;
                if (AheadHigher && Get_HasRoom(InSettings, InRayCast, *Ahead, InUp))
                {
                    Contacts.Proposal = Ahead;
                    return Contacts;
                }
            }
            if (Get_HasRoom(InSettings, InRayCast, *Contacts.Down, InUp))
            {
                Contacts.Proposal = Contacts.Down;
                return Contacts;
            }
        }

        auto& Ahead = CastLookAhead();
        if (Ahead.IsSet() && Get_HasRoom(InSettings, InRayCast, *Ahead, InUp))
        {
            Contacts.Proposal = Ahead;
            return Contacts;
        }
        if (NOT InMoving)
        { return Contacts; }

        // Recover around a convex edge from a bounded fan rather than extending a ray forever.
        const auto FanStart = Contacts.Candidate + InForward * Clearance;
        const auto FanDirection = (-InUp - InForward).GetSafeNormal();
        auto Fan = DoCast(InRayCast, FanStart, FanStart + FanDirection * Reach, ESource::Fan);
        if (NOT Fan.IsSet())
        { return Contacts; }

        // A refused face obstructs only while the steering points into it, the condition a followed wall is kept by: the
        // fan meets an upright face from beyond it, facing along the travel, and the body is walking away from that one.
        const auto FanRoom = Get_Room(InSettings, InRayCast, *Fan, InUp);
        if (FanRoom >= Clearance)
        { Contacts.Proposal = Fan; }
        else if (FVector::DotProduct(InForward, Fan->Hit.Get_Normal()) < 0.0)
        { DoKeep_Standoff(InSettings, MakeObstruction(InSettings, *Fan, FanRoom), InOldPosition, StepSeconds, Contacts); }
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
            float InVoluntaryScale,
            FTransform& InOutBody)
        -> void
    {
        const auto OldRotation = InOutBody.GetRotation();
        const auto TargetRotation = FRotationMatrix::MakeFromZX(InNormal, InForward).ToQuat();
        const auto Angle = OldRotation.AngularDistance(TargetRotation);
        const auto Alpha = Angle > KINDA_SMALL_NUMBER
            ? FMath::Min(1.0, FMath::DegreesToRadians(InSettings.Get_SurfaceTurnRateDegrees()) * InVoluntaryScale * InStep / Angle)
            : static_cast<double>(InVoluntaryScale);
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
            float InVoluntaryScale,
            FTransform& InOutBody,
            ck::FProceduralSurfaceMotionState& InOutState)
        -> void
    {
        const auto Normal = InContact.Hit.Get_Normal().GetSafeNormal();
        const auto TargetForward = FQuat::FindBetweenNormals(InUp, Normal).RotateVector(InForward);
        InOutState.Set_SupportNormal(Normal);
        InOutState.Set_TravelTangent(TargetForward);
        InOutState.Set_ContactSource(InContact.Source);

        DoTurn(InSettings, Normal, TargetForward, InStep, InVoluntaryScale, InOutBody);

        const auto Height = FVector::DotProduct(InCandidate - InContact.Hit.Get_Position(), Normal);
        const auto MaxCorrection = InSettings.Get_ClearanceSpeed() * InVoluntaryScale * InStep;
        const auto Correction = FMath::Clamp(InSettings.Get_Clearance() - Height, -MaxCorrection, MaxCorrection);
        InOutBody.SetLocation(InCandidate + Normal * Correction);
    }

    auto
        DoSet_Obstruction(
            const TOptional<FObstruction>& InObstruction,
            ck::FProceduralSurfaceMotionState& InOutState)
        -> void
    {
        InOutState.Set_Obstruction(InObstruction.IsSet() ? ck::EProceduralSurfaceObstruction::Wall : ck::EProceduralSurfaceObstruction::None);
        InOutState.Set_ObstructionNormal(InObstruction.IsSet() ? InObstruction->Normal : FVector::ZeroVector);
        InOutState.Set_ObstructionStandoff(InObstruction.IsSet() ? static_cast<float>(InObstruction->Standoff) : 0.0f);
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
            const TOptional<FProceduralSurfaceFeetSupport>& InFeetSupport,
            FTransform& InOutBody,
            FProceduralSurfaceMotionState& InOutState,
            float InVoluntaryScale)
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
        const auto Travel = Forward * (InSpeed * InVoluntaryScale * Step);
        const auto Moving = InSpeed > 0.0f;

        auto Contacts = ck_procedural_surface_motion::DoFind_Contacts(InSettings, InRayCast, InFeetSupport, InOutState, InStep, OldPosition,
            Travel, Up, Forward, Moving);
        const auto& Candidate = Contacts.Candidate;
        ck_procedural_surface_motion::DoSet_Obstruction(Contacts.Obstruction, InOutState);
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
            // when the down ray proposed the turn, found nothing it trusts, or has no room under the body. A face the rays
            // only graze for a few substeps is never adopted; a body longer than its clearance can overrun a head-on wall
            // by up to its speed times the confirm time. A coast adopts no contact, so the source stays the one last
            // accepted.
            const auto SupportUnder = Pending && Contacts.Down.IsSet()
                && ck_procedural_surface_motion::Get_AngleDegrees(Up, Contacts.Down->Hit.Get_Normal()) <= InSettings.Get_ConfirmAngleDegrees()
                && ck_procedural_surface_motion::Get_HasRoom(InSettings, InRayCast, *Contacts.Down, Up);
            if (Pending && NOT SupportUnder)
            {
                ck_procedural_surface_motion::DoTurn(InSettings, Up, Forward, Step, InVoluntaryScale, InOutBody);
                InOutBody.SetLocation(Candidate);
                InOutState.Set_TravelTangent(Forward);
            }
            else
            {
                ck_procedural_surface_motion::DoAdopt(InSettings, Pending ? *Contacts.Down : Proposal, Up, Forward, Candidate, Step,
                    InVoluntaryScale, InOutBody, InOutState);
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
            InOutState.Set_Velocity(Contacts.Obstruction.IsSet() ? Contacts.Travel / Step : Forward * InSpeed * InVoluntaryScale);
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

        // A landing with no room off it (a face across a gap narrower than the clearance) would stand the body inside the
        // solid behind it: the fall loses its velocity into the face and goes on, down along it, kept off it like any
        // obstruction.
        const auto Normal = FallHit.Get_Normal().GetSafeNormal();
        auto Landing = ck_procedural_surface_motion::FContact{FallHit, EProceduralSurfaceContactSource::Fall};
        const auto LandingRoom = ck_procedural_surface_motion::Get_Room(InSettings, InRayCast, Landing, Up);
        if (LandingRoom < Clearance)
        {
            InOutState.Set_Velocity(InOutState.Get_Velocity() - Normal * FVector::DotProduct(InOutState.Get_Velocity(), Normal));
            auto Falling = ck_procedural_surface_motion::FContacts{};
            Falling.Travel = InOutState.Get_Velocity() * Step;
            Falling.Candidate = OldPosition + Falling.Travel;
            ck_procedural_surface_motion::DoKeep_Standoff(InSettings, ck_procedural_surface_motion::MakeObstruction(InSettings, Landing, LandingRoom),
                OldPosition, Step, Falling);
            ck_procedural_surface_motion::DoSet_Obstruction(Falling.Obstruction, InOutState);
            InOutBody.SetLocation(Falling.Candidate);
            return;
        }

        // The landing keeps the heading the body travelled with: the tangent it had, laid onto the landing plane, then the
        // steering, then the body's own forward. The fall direction often runs along the landing normal and orients nothing.
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

    auto
        StepProceduralSurfaceMotionPaced(
            const FProceduralSurfaceMotionSettings& InSettings,
            const FVector& InSteerDirection,
            float InSpeed,
            FCk_Time InStep,
            FProceduralSurfaceRayCast InRayCast,
            const TOptional<FProceduralSurfaceFeetSupport>& InFeetSupport,
            TArrayView<const FProceduralSurfaceReachPaceAnchor> InAnchors,
            const TOptional<FTransform>& InPoseOffset,
            FTransform& InOutBody,
            FProceduralSurfaceMotionState& InOutState)
        -> FProceduralSurfaceReachPaceOutcome
    {
        auto Outcome = FProceduralSurfaceReachPaceOutcome{};
        const auto StartBody = InOutBody;
        const auto StartState = InOutState;
        const auto TryScale = [&](float InScale, FTransform& OutBody, FProceduralSurfaceMotionState& OutState) -> void
        {
            OutBody = StartBody;
            OutState = StartState;
            StepProceduralSurfaceMotion(InSettings, InSteerDirection, InSpeed, InStep, InRayCast, InFeetSupport,
                OutBody, OutState, InScale);
            Outcome.Set_Trials(Outcome.Get_Trials() + 1);
        };

        struct FReachLimit
        {
            double Sim = 0.0;
            double Posed = 0.0;
        };
        constexpr auto ReachTolerance = 1.0e-3;
        const auto FitsReach = [&](const FTransform& InCandidate, TArrayView<const FReachLimit> InLimits) -> bool
        {
            const auto Posed = InPoseOffset.IsSet() ? InPoseOffset.GetValue() * InCandidate : FTransform::Identity;
            for (auto Index = 0; Index < InAnchors.Num(); ++Index)
            {
                const auto& Anchor = InAnchors[Index];
                if (FVector::Dist(InCandidate.TransformPosition(Anchor.Get_HipLocal()), Anchor.Get_FootWorld())
                        > InLimits[Index].Sim + ReachTolerance
                    || (InPoseOffset.IsSet() && FVector::Dist(Posed.TransformPosition(Anchor.Get_HipLocal()), Anchor.Get_FootWorld())
                        > InLimits[Index].Posed + ReachTolerance))
                { return false; }
            }
            return true;
        };

        auto FullBody = FTransform{};
        auto FullState = FProceduralSurfaceMotionState{};
        TryScale(1.0f, FullBody, FullState);
        if (InAnchors.IsEmpty())
        {
            InOutBody = FullBody;
            InOutState = FullState;
            return Outcome;
        }

        auto Limits = TArray<FReachLimit, TInlineAllocator<64>>{};
        Limits.Reserve(InAnchors.Num());
        for (const auto& Anchor : InAnchors)
        { Limits.Add(FReachLimit{Anchor.Get_Reach(), Anchor.Get_Reach()}); }
        if (FitsReach(FullBody, TArrayView<const FReachLimit>{Limits}))
        {
            InOutBody = FullBody;
            InOutState = FullState;
            return Outcome;
        }

        auto ZeroBody = FTransform{};
        auto ZeroState = FProceduralSurfaceMotionState{};
        TryScale(0.0f, ZeroBody, ZeroState);
        const auto FullPosed = InPoseOffset.IsSet() ? InPoseOffset.GetValue() * FullBody : FTransform::Identity;
        const auto ZeroPosed = InPoseOffset.IsSet() ? InPoseOffset.GetValue() * ZeroBody : FTransform::Identity;
        auto AttemptedHipDistance = 0.0;
        for (const auto& Anchor : InAnchors)
        {
            AttemptedHipDistance = FMath::Max(AttemptedHipDistance,
                FVector::Dist(FullBody.TransformPosition(Anchor.Get_HipLocal()), ZeroBody.TransformPosition(Anchor.Get_HipLocal())));
            if (InPoseOffset.IsSet())
            {
                AttemptedHipDistance = FMath::Max(AttemptedHipDistance,
                    FVector::Dist(FullPosed.TransformPosition(Anchor.Get_HipLocal()), ZeroPosed.TransformPosition(Anchor.Get_HipLocal())));
            }
        }
        if (InStep > FCk_Time{})
        { Outcome.Set_AttemptedStanceSpeed(static_cast<float>(AttemptedHipDistance / InStep.Get_Seconds())); }
        auto PhysicalOverride = false;
        for (auto Index = 0; Index < InAnchors.Num(); ++Index)
        {
            const auto& Anchor = InAnchors[Index];
            const auto SimDistance = FVector::Dist(ZeroBody.TransformPosition(Anchor.Get_HipLocal()), Anchor.Get_FootWorld());
            const auto PosedDistance = InPoseOffset.IsSet()
                ? FVector::Dist(ZeroPosed.TransformPosition(Anchor.Get_HipLocal()), Anchor.Get_FootWorld()) : 0.0;
            PhysicalOverride |= SimDistance > Anchor.Get_Reach() + ReachTolerance
                || (InPoseOffset.IsSet() && PosedDistance > Anchor.Get_Reach() + ReachTolerance);
            Limits[Index].Sim = FMath::Max(static_cast<double>(Anchor.Get_Reach()), SimDistance);
            Limits[Index].Posed = FMath::Max(static_cast<double>(Anchor.Get_Reach()), PosedDistance);
        }
        Outcome.Set_PhysicalOverride(PhysicalOverride);

        auto AcceptedBody = ZeroBody;
        auto AcceptedState = ZeroState;
        auto AcceptedScale = 0.0f;
        if (FitsReach(FullBody, TArrayView<const FReachLimit>{Limits}))
        {
            AcceptedBody = FullBody;
            AcceptedState = FullState;
            AcceptedScale = 1.0f;
        }
        else
        {
            auto Lower = 0.0f;
            auto Upper = 1.0f;
            constexpr auto ReachPaceSearchIterations = 6;
            for (auto Search = 0; Search < ReachPaceSearchIterations; ++Search)
            {
                const auto Scale = (Lower + Upper) * 0.5f;
                auto TrialBody = FTransform{};
                auto TrialState = FProceduralSurfaceMotionState{};
                TryScale(Scale, TrialBody, TrialState);
                if (FitsReach(TrialBody, TArrayView<const FReachLimit>{Limits}))
                {
                    Lower = Scale;
                    AcceptedBody = TrialBody;
                    AcceptedState = TrialState;
                    AcceptedScale = Scale;
                }
                else
                { Upper = Scale; }
            }
        }

        // Every accepted pose and support state came from the same trial. Contact transitions need not be monotone in
        // voluntary scale; the bounded search only commits candidates it actually checked.
        InOutBody = AcceptedBody;
        InOutState = AcceptedState;
        if (AcceptedScale < 1.0f && NOT PhysicalOverride && InStep > FCk_Time{} && AttemptedHipDistance > 0.0)
        {
            for (auto Index = 0; Index < InAnchors.Num(); ++Index)
            {
                const auto& Anchor = InAnchors[Index];
                if (FVector::Dist(FullBody.TransformPosition(Anchor.Get_HipLocal()), Anchor.Get_FootWorld())
                        > Anchor.Get_Reach() + ReachTolerance
                    || (InPoseOffset.IsSet()
                        && FVector::Dist(FullPosed.TransformPosition(Anchor.Get_HipLocal()), Anchor.Get_FootWorld())
                            > Anchor.Get_Reach() + ReachTolerance))
                { Outcome.Get_RejectedAnchorIndices().Add(Index); }
            }
            if (NOT Outcome.Get_RejectedAnchorIndices().IsEmpty())
            { Outcome.Set_RejectedFullBody(TOptional<FTransform>{FullBody}); }
        }
        return Outcome.Set_Scale(AcceptedScale);
    }
}

// --------------------------------------------------------------------------------------------------------------------
