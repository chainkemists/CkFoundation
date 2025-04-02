#include "CkRaySense_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/TransientEntity/CkTransientEntity_Utils.h"

#include "CkRaySense/CkRaySense_Log.h"

#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_RaySense_LineTrace_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) const
        -> void
    {
        auto World = UCk_Utils_TransientEntity_UE::Get_World(InHandle);

        const auto& PrevTransform = InTransform_Prev.Get_Transform();
        const auto& CurrTransform = InTransform.Get_Transform();

        auto HitResult = FHitResult{};
        const auto Hit = World->LineTraceSingleByChannel(HitResult,
            PrevTransform.GetLocation(), CurrTransform.GetLocation(),
            InParams.Get_CollisionChannel());

        if (NOT Hit)
        { return; }

        auto Result = FCk_RaySense_HitResult{HitResult.ImpactPoint, HitResult.ImpactNormal}
        .Set_MaybeHitActor(HitResult.GetActor())
        .Set_MaybeHitComponent(HitResult.GetComponent())
        .Set_MaybeHitHandle(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(HitResult.GetActor()) ?
            UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(HitResult.GetActor()) : FCk_Handle{});

        UUtils_Signal_OnRaySenseTraceHit::Broadcast(InHandle, MakePayload(InHandle, Result));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RaySense_BoxSweep_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeBox_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) const
        -> void
    {
        auto World = UCk_Utils_TransientEntity_UE::Get_World(InHandle);

        const auto& PrevTransform = InTransform_Prev.Get_Transform();
        const auto& CurrTransform = InTransform.Get_Transform();

        auto Shape = FCollisionShape::MakeBox(InShape.Get_Dimensions().Get_HalfExtents());

        auto HitResult = FHitResult{};

        const auto Hit = InParams.Get_CollisionQuality() == ECk_RaySense_CollisionQuality::Sweep
                         ? World->SweepSingleByChannel(HitResult, PrevTransform.GetLocation(),
                             CurrTransform.GetLocation(), CurrTransform.GetRotation(), InParams.Get_CollisionChannel(),
                             Shape)
                         : World->OverlapAnyTestByChannel(CurrTransform.GetLocation(), CurrTransform.GetRotation(),
                             InParams.Get_CollisionChannel(), Shape);

        if (NOT Hit)
        { return; }

        auto Result = FCk_RaySense_HitResult{HitResult.ImpactPoint, HitResult.ImpactNormal}
        .Set_MaybeHitActor(HitResult.GetActor())
        .Set_MaybeHitComponent(HitResult.GetComponent())
        .Set_MaybeHitHandle(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(HitResult.GetActor()) ?
            UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(HitResult.GetActor()) : FCk_Handle{});

        UUtils_Signal_OnRaySenseTraceHit::Broadcast(InHandle, MakePayload(InHandle, Result));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RaySense_SphereSweep_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeSphere_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) const
        -> void
    {
        auto World = UCk_Utils_TransientEntity_UE::Get_World(InHandle);

        const auto& PrevTransform = InTransform_Prev.Get_Transform();
        const auto& CurrTransform = InTransform.Get_Transform();

        auto Shape = FCollisionShape::MakeSphere(InShape.Get_Dimensions().Get_Radius());

        auto HitResult = FHitResult{};

        const auto Hit = InParams.Get_CollisionQuality() == ECk_RaySense_CollisionQuality::Sweep
                         ? World->SweepSingleByChannel(HitResult, PrevTransform.GetLocation(),
                             CurrTransform.GetLocation(), CurrTransform.GetRotation(), InParams.Get_CollisionChannel(),
                             Shape)
                         : World->OverlapAnyTestByChannel(CurrTransform.GetLocation(), CurrTransform.GetRotation(),
                             InParams.Get_CollisionChannel(), Shape);

        if (NOT Hit)
        { return; }

        auto Result = FCk_RaySense_HitResult{HitResult.ImpactPoint, HitResult.ImpactNormal}
        .Set_MaybeHitActor(HitResult.GetActor())
        .Set_MaybeHitComponent(HitResult.GetComponent())
        .Set_MaybeHitHandle(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(HitResult.GetActor()) ?
            UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(HitResult.GetActor()) : FCk_Handle{});

        UUtils_Signal_OnRaySenseTraceHit::Broadcast(InHandle, MakePayload(InHandle, Result));
    }

    auto
        FProcessor_RaySense_CapsuleSweep_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCapsule_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) const
        -> void
    {
        auto World = UCk_Utils_TransientEntity_UE::Get_World(InHandle);

        const auto& PrevTransform = InTransform_Prev.Get_Transform();
        const auto& CurrTransform = InTransform.Get_Transform();

        auto Shape = FCollisionShape::MakeCapsule(InShape.Get_Dimensions().Get_Radius(), InShape.Get_Dimensions().Get_HalfHeight());

        auto HitResult = FHitResult{};

        const auto Hit = InParams.Get_CollisionQuality() == ECk_RaySense_CollisionQuality::Sweep
                         ? World->SweepSingleByChannel(HitResult, PrevTransform.GetLocation(),
                             CurrTransform.GetLocation(), CurrTransform.GetRotation(), InParams.Get_CollisionChannel(),
                             Shape)
                         : World->OverlapAnyTestByChannel(CurrTransform.GetLocation(), CurrTransform.GetRotation(),
                             InParams.Get_CollisionChannel(), Shape);

        if (NOT Hit)
        { return; }

        auto Result = FCk_RaySense_HitResult{HitResult.ImpactPoint, HitResult.ImpactNormal}
        .Set_MaybeHitActor(HitResult.GetActor())
        .Set_MaybeHitComponent(HitResult.GetComponent())
        .Set_MaybeHitHandle(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(HitResult.GetActor()) ?
            UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(HitResult.GetActor()) : FCk_Handle{});

        UUtils_Signal_OnRaySenseTraceHit::Broadcast(InHandle, MakePayload(InHandle, Result));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RaySense_CylinderSweep_Update::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ShapeCylinder_Current& InShape,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_Transform_Previous& InTransform_Prev,
            const FFragment_Transform& InTransform) const
        -> void
    {
        CK_TRIGGER_ENSURE(TEXT("Cylinder shape is NOT supported by Unreal. It is only supported by Jolt. Collisions for "
            "[{}] will NOT work"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RaySense_HandleRequests::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        _TransientEntity.Clear<FTag_RaySense_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_RaySense_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RaySense_Current& InCurrent,
            FFragment_RaySense_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](FFragment_RaySense_Requests& InRequests)
        {
            algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](const auto& InRequest)
            {
                DoHandleRequest(InHandle, InCurrent, InRequest);
            }));
        });
    }

    auto
        FProcessor_RaySense_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_RaySense_Current& InCurrent,
            const FCk_Request_RaySense_ExampleRequest& InRequest)
        -> void
    {
        // Add request handling logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RaySense_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RaySense_Current& InCurrent) const
        -> void
    {
        // Add teardown logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_RaySense_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_RaySense_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_RaySense_Rep>& InRepComp) const
        -> void
    {
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_RaySense_Rep>(InHandle, [&](UCk_Fragment_RaySense_Rep* InLocalRepComp)
        {
            // Add replication logic here
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------