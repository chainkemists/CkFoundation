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
    // An entity can own more than one Jolt body (e.g. a JoltBody AND a Probe), all sharing the entity id as
    // UserData — the index+seq check is the disambiguation. InOtherEntity is carried into the payload verbatim
    // and may be invalid (the other body's entity is dead) or a non-JoltBody attribution entity.
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
            // Jolt's RelativeNormalVelocity is negative when closing; negated here so _RelativeNormalSpeed is
            // POSITIVE at impact and consumers threshold "> X" naturally.
            case FCk_Jolt_ContactEvent::EType::Added:
            {
                const auto Payload = FCk_JoltBody_Payload_OnContact{
                    InOtherEntity, InContactPoints, InContactNormal, -InEvent.RelativeNormalVelocity, OtherIsSensor};

                ck::UUtils_Signal_OnJoltBodyContactAdded::Broadcast(JoltBody, ck::MakePayload(JoltBody, Payload));
                break;
            }
            case FCk_Jolt_ContactEvent::EType::Persisted:
            {
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

        // Body UserData is a raw (versioned) entity id baked in at body registration; a snapshot load wipes and
        // restores the registry, so a contact queued pre-load can resolve to an id that Get_ValidHandle would
        // ENSURE on. The non-ensuring liveness check first yields an invalid handle the guards below tolerate.
        const auto ResolveBodyEntity = [&](uint64 InUserData) -> FCk_Handle
        {
            // UserData 0 = NO entity; raw entity id 0 is ALWAYS the registry's live transient root.
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

            // Per-side: body1 gets the negated world-space normal + its own contact points, body2 the positive.
            DoRouteForSide(Body1Entity, Body2Entity, Event, Event.Body1IndexAndSeq,
                Event.ContactPointsOn1, -Event.WorldSpaceNormal, Event.IsSensor2);

            DoRouteForSide(Body2Entity, Body1Entity, Event, Event.Body2IndexAndSeq,
                Event.ContactPointsOn2, Event.WorldSpaceNormal, Event.IsSensor1);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
