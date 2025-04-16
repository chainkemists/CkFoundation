#include "CkRaySense_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/TransientEntity/CkTransientEntity_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkRaySense/CkRaySense_Log.h"

#include "CkNet/CkNet_Utils.h"

#include "CkRaySense/CkRaySense_Utils.h"

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
        auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);

        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("Could NOT get the World for entity [{}]. RaySense will NOT work"), InHandle)
        { return; }

        const auto& PrevTransform = InTransform_Prev.Get_Transform();
        const auto& CurrTransform = InTransform.Get_Transform();

        auto HitResult = FHitResult{};
        const auto Hit = World->LineTraceSingleByChannel(HitResult,
            PrevTransform.GetLocation(), CurrTransform.GetLocation(),
            InParams.Get_CollisionChannel());

        if (NOT Hit)
        { return; }

        if (UCk_Utils_RaySense_UE::DoGet_ShouldIgnoreTraceHit(InHandle, HitResult))
        { return; }

        auto Result = FCk_RaySense_HitResult{HitResult.ImpactPoint, HitResult.ImpactNormal}
        .Set_ImpactPhysMat(HitResult.PhysMaterial.Get())
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

        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("Could NOT get the World for entity [{}]. RaySense will NOT work"), InHandle)
        { return; }

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

        if (UCk_Utils_RaySense_UE::DoGet_ShouldIgnoreTraceHit(InHandle, HitResult))
        { return; }

        auto Result = FCk_RaySense_HitResult{HitResult.ImpactPoint, HitResult.ImpactNormal}
        .Set_ImpactPhysMat(HitResult.PhysMaterial.Get())
        .Set_MaybeHitActor(HitResult.GetActor())
        .Set_MaybeHitComponent(HitResult.GetComponent())
        .Set_MaybeHitHandle(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(HitResult.GetActor()) ?
            UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(HitResult.GetActor()) : FCk_Handle{});

        switch (InParams.Get_CollisionResponse())
        {
            case ECk_RaySense_CollisionResponse_Policy::Overlap: break;
            case ECk_RaySense_CollisionResponse_Policy::Collide:
            {
                UCk_Utils_Transform_TypeUnsafe_UE::Request_SetLocation(InHandle,
                    FCk_Request_Transform_SetLocation{Result.Get_ImpactPoint()});
                break;
            }
        }

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

        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("Could NOT get the World for entity [{}]. RaySense will NOT work"), InHandle)
        { return; }

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

        if (UCk_Utils_RaySense_UE::DoGet_ShouldIgnoreTraceHit(InHandle, HitResult))
        { return; }

        auto Result = FCk_RaySense_HitResult{HitResult.ImpactPoint, HitResult.ImpactNormal}
        .Set_ImpactPhysMat(HitResult.PhysMaterial.Get())
        .Set_MaybeHitActor(HitResult.GetActor())
        .Set_MaybeHitComponent(HitResult.GetComponent())
        .Set_MaybeHitHandle(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(HitResult.GetActor()) ?
            UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(HitResult.GetActor()) : FCk_Handle{});

        switch (InParams.Get_CollisionResponse())
        {
            case ECk_RaySense_CollisionResponse_Policy::Overlap: break;
            case ECk_RaySense_CollisionResponse_Policy::Collide:
            {
                UCk_Utils_Transform_TypeUnsafe_UE::Request_SetLocation(InHandle,
                    FCk_Request_Transform_SetLocation{Result.Get_ImpactPoint()});
                break;
            }
        }

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

        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("Could NOT get the World for entity [{}]. RaySense will NOT work"), InHandle)
        { return; }

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

        if (UCk_Utils_RaySense_UE::DoGet_ShouldIgnoreTraceHit(InHandle, HitResult))
        { return; }

        auto Result = FCk_RaySense_HitResult{HitResult.ImpactPoint, HitResult.ImpactNormal}
        .Set_ImpactPhysMat(HitResult.PhysMaterial.Get())
        .Set_MaybeHitActor(HitResult.GetActor())
        .Set_MaybeHitComponent(HitResult.GetComponent())
        .Set_MaybeHitHandle(UCk_Utils_OwningActor_UE::Get_IsActorEcsReady(HitResult.GetActor()) ?
            UCk_Utils_OwningActor_UE::Get_ActorEntityHandle(HitResult.GetActor()) : FCk_Handle{});

        switch (InParams.Get_CollisionResponse())
        {
            case ECk_RaySense_CollisionResponse_Policy::Overlap: break;
            case ECk_RaySense_CollisionResponse_Policy::Collide:
            {
                UCk_Utils_Transform_TypeUnsafe_UE::Request_SetLocation(InHandle,
                    FCk_Request_Transform_SetLocation{Result.Get_ImpactPoint()});
                break;
            }
        }

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
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_RaySense_Params& InParams,
            FFragment_RaySense_Current& InCurrent,
            const FFragment_RaySense_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](
            FFragment_RaySense_Requests& InRequests)
            {
                algo::ForEachRequest(InRequests._Requests, Visitor([&](
                    const auto& InRequest)
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
            const FCk_Request_RaySense_EnableDisable& InRequest)
        -> void
    {
        switch(InRequest.Get_EnableDisable())
        {
            case ECk_EnableDisable::Enable:
            {
                InHandle.Try_Remove<FTag_RaySense_Disabled>();
                break;
            }
            case ECk_EnableDisable::Disable:
            {
                InHandle.AddOrGet<FTag_RaySense_Disabled>();
                break;
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------