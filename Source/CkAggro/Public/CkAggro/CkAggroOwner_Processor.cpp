#include "CkAggroOwner_Processor.h"

#include "CkAggro/CkAggro_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Aggro_DistanceScore::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_AggroOwner_Params& InParams) const
        -> void
    {
        const auto OwnerLocation = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(InHandle);

        ck::FUtils_RecordOfAggros::ForEach_ValidEntry(InHandle, [&](FCk_Handle_Aggro InAggro)
        {
            const auto AggroTarget = UCk_Utils_Aggro_UE::Get_AggroTarget(InAggro);

            if (ck::Is_NOT_Valid(AggroTarget))
            { UCk_Utils_Aggro_UE::Request_Exclude(InAggro); }

            const auto CurrentLocation = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(AggroTarget);
            const auto DistanceSquared = FVector::DistSquared(OwnerLocation, CurrentLocation);
            InAggro.Get<FFragment_Aggro_Current>() = FFragment_Aggro_Current{DistanceSquared};

            if (DistanceSquared > FMath::Square(InParams.Get_Params().Get_AggroRange()))
            { UCk_Utils_Aggro_UE::Request_Exclude(InAggro); }

            return ECk_Record_ForEachIterationResult::Continue;
        });

        FUtils_RecordOfAggros::Sort(InHandle, [&](const FCk_Handle_Aggro& A, const FCk_Handle_Aggro& B)
        {
            if (A.Has<FTag_Aggro_Excluded>())
            { return false; }

            return A.Get<FFragment_Aggro_Current>().Get_Score() < B.Get<FFragment_Aggro_Current>().Get_Score();
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Aggro_LineOfSightScore::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_AggroOwner_Current& InCurrent) const
        -> void
    {
        // assuming that the record is already sorted
        // assuming that the incoming handle has an Actor (TODO: remove this assumption)

        const auto OwningActorEntity = UCk_Utils_OwningActor_UE::TryGet_Entity_OwningActor_InOwnershipChain(InHandle);
        const auto Actor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(OwningActorEntity);
        const auto OwnerLocation = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(InHandle);

        InCurrent = FFragment_AggroOwner_Current{};

        ck::FUtils_RecordOfAggros::ForEach_ValidEntry(InHandle, [&](FCk_Handle_Aggro InAggro)
        {
            if (InAggro.Has<FTag_Aggro_Excluded>())
            { return ECk_Record_ForEachIterationResult::Continue; }

            // assuming that each Aggro Entity Target has an Actor (TODO: remove this assumption)

            const auto OtherActorEntity = UCk_Utils_OwningActor_UE::TryGet_Entity_OwningActor_InOwnershipChain(UCk_Utils_Aggro_UE::Get_AggroTarget(InAggro));
            const auto OtherActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(OtherActorEntity);

            const auto TargetLocation = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(OtherActorEntity);

            auto CollisionParams = FCollisionQueryParams{SCENE_QUERY_STAT(LineOfSight), true, Actor};
            CollisionParams.AddIgnoredActor(OtherActor);

            auto Response = FCollisionResponseParams{};
            auto HitResult = FHitResult{}; // TODO: for debugging only, to remove
            auto Hit = Actor->GetWorld()->LineTraceSingleByChannel(HitResult, OwnerLocation, TargetLocation, ECC_Visibility, CollisionParams, Response);

            if (Hit)
            { return ECk_Record_ForEachIterationResult::Continue; }

            InCurrent = FFragment_AggroOwner_Current{InAggro};
            return ECk_Record_ForEachIterationResult::Break;
        });
    }
}
