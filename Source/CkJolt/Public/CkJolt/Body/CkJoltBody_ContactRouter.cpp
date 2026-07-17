#include "CkJoltBody_ContactRouter.h"

#include "CkCore/Payload/CkPayload.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Registry/CkRegistry.h"

#include "CkJolt/Body/CkJoltBody_Fragment.h"
#include "CkJolt/Body/CkJoltBody_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::jolt_body
{
    // Fires the contact signal for ONE side (the "self" body) of a contact event, if self is a JoltBody whose
    // OWN body id matches this side's index+seq. An entity can own more than one Jolt body (e.g. a JoltBody
    // AND a Probe), all sharing the entity id as UserData — the index+seq check is the disambiguation.
    // InOtherEntity may be an invalid handle (the other body has no live entity, e.g. a baked static-world
    // body); it is carried into the payload verbatim.
    auto
        DoRouteForSide(
            const FCk_Handle& InSelfEntity,
            const FCk_Handle& InOtherEntity,
            const FCk_Jolt_ContactEvent& InEvent,
            uint32 InSelfBodyIndexAndSeq,
            const TArray<FVector>& InContactPoints,
            const FVector& InContactNormal,
            bool InOtherIsSensor)
        -> void
    {
        if (ck::Is_NOT_Valid(InSelfEntity) || NOT InSelfEntity.Has<ck::FFragment_JoltBody_Current>())
        { return; }

        if (InSelfEntity.Get<ck::FFragment_JoltBody_Current>().Get_BodyId().GetIndexAndSequenceNumber() != InSelfBodyIndexAndSeq)
        { return; }

        auto SelfHandle = InSelfEntity;
        auto JoltBody = UCk_Utils_JoltBody_UE::Cast(SelfHandle);

        const auto OtherIsSensor = InOtherIsSensor ? ECk_EnableDisable::Enable : ECk_EnableDisable::Disable;

        switch (InEvent.Type)
        {
            // The event's RelativeNormalVelocity keeps Jolt's convention (negative = closing); the payload
            // negates it so _RelativeNormalSpeed is POSITIVE at impact — consumers threshold "> X" naturally.
            case FCk_Jolt_ContactEvent::EType::Added:
            {
                const auto Payload = FCk_JoltBody_Payload_OnContact{
                    InOtherEntity, InContactPoints, InContactNormal, -InEvent.RelativeNormalVelocity, OtherIsSensor};

                ck::UUtils_Signal_OnJoltBodyContactAdded::Broadcast(JoltBody, ck::MakePayload(JoltBody, Payload));
                break;
            }
            case FCk_Jolt_ContactEvent::EType::Persisted:
            {
                // Persisted events only fire for bodies that opted in — mirrors the Probe PersistContacts gate.
                if (NOT JoltBody.Has<ck::FTag_JoltBody_PersistContacts>())
                { break; }

                const auto Payload = FCk_JoltBody_Payload_OnContact{
                    InOtherEntity, InContactPoints, InContactNormal, -InEvent.RelativeNormalVelocity, OtherIsSensor};

                ck::UUtils_Signal_OnJoltBodyContactPersisted::Broadcast(JoltBody, ck::MakePayload(JoltBody, Payload));
                break;
            }
            case FCk_Jolt_ContactEvent::EType::Removed:
            {
                const auto Payload = FCk_JoltBody_Payload_OnContactRemoved{InOtherEntity};

                ck::UUtils_Signal_OnJoltBodyContactRemoved::Broadcast(JoltBody, ck::MakePayload(JoltBody, Payload));
                break;
            }
        }
    }

    auto
        RouteContactEvents(
            const FCk_Handle& InTransientEntity,
            const TArray<FCk_Jolt_ContactEvent>& InEvents)
        -> void
    {
        if (InEvents.IsEmpty())
        { return; }

        const auto RegView = InTransientEntity.Get_RegistryView();

        // Body UserData is a raw (versioned) entity id baked in at body registration. A snapshot load wipes/
        // restores the registry, so a contact queued pre-load can resolve to an id that is dead in the fresh
        // registry — Get_ValidHandle ENSURES on a stale id. Do a non-ensuring registry-liveness check first
        // (mirrors the SpatialQuery bridge and SleepStateMirror); a dead id yields an invalid handle the
        // guards below already tolerate.
        const auto ResolveBodyEntity = [&](uint64 InUserData) -> FCk_Handle
        {
            // UserData 0 = NO entity. Bodies that never SetUserData (baked static-world bodies) carry Jolt's
            // default 0 — and raw entity id 0 (index 0, version 0) is ALWAYS the registry's transient root
            // (first create()), a live entity. Without this guard every dynamic-vs-baked-floor contact would
            // resolve _OtherEntity to the transient root instead of an invalid handle.
            if (InUserData == 0)
            { return {}; }

            const auto Entity = FCk_Entity{FCk_Entity::IdType{static_cast<FCk_Entity::IdType>(InUserData)}};
            if (NOT RegView.IsValid(Entity))
            { return {}; }
            return InTransientEntity.Get_ValidHandle(Entity.Get_ID());
        };

        for (const auto& Event : InEvents)
        {
            const auto Body1Entity = ResolveBodyEntity(Event.Body1UserData);
            const auto Body2Entity = ResolveBodyEntity(Event.Body2UserData);

            // Normal sign and contact points are per-side (mirrors the SpatialQuery bridge): body1 gets the
            // negated world-space normal + its own contact points; body2 gets the positive normal + its own.
            DoRouteForSide(Body1Entity, Body2Entity, Event, Event.Body1IndexAndSeq,
                Event.ContactPointsOn1, -Event.WorldSpaceNormal, Event.IsSensor2);

            DoRouteForSide(Body2Entity, Body1Entity, Event, Event.Body2IndexAndSeq,
                Event.ContactPointsOn2, Event.WorldSpaceNormal, Event.IsSensor1);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
