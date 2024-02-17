#include "CkCameraShake_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkCamera/CkCamera_Log.h"
#include "CkNet/CkNet_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_CameraShake_HandleRequests::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::Tick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_CameraShake_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_CameraShake_Params& InParams,
            FFragment_CameraShake_Requests& InRequestsComp) const
        -> void
    {
        algo::ForEachRequest(InRequestsComp._PlayRequests, ck::Visitor(
        [&](const auto& InRequest)
        {
            DoHandleRequest(InHandle, InParams, InRequest);
        }));
    }

    auto
        FProcessor_CameraShake_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CameraShake_Params& InParams,
            const FCk_Request_CameraShake_PlayOnTarget& InRequest) const
        -> void
    {
        const auto& TargetEntity = InRequest.Get_Target();

        CK_ENSURE_IF_NOT(ck::IsValid(TargetEntity), TEXT("Invalid Camera Shake Target!"))
        { return; }

        auto* TargetOwningActor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(TargetEntity);

        CK_ENSURE_IF_NOT(ck::IsValid(TargetOwningActor), TEXT("Camera Shake Targets an Entity [{}] that does NOT have an Actor!"), TargetEntity)
        { return; }

        const auto& TargetPlayerController = Cast<APlayerController>(TargetOwningActor->GetInstigatorController());

        if (ck::Is_NOT_Valid(TargetPlayerController))
        {
            camera::VeryVerbose(TEXT("Camera Shake Targets an Entity [{}] that is NO valid Player Controller!"), TargetEntity);
            return;
        }

        const auto& CameraManager = TargetPlayerController->PlayerCameraManager;

        if (ck::Is_NOT_Valid(CameraManager))
        {
            camera::VeryVerbose(TEXT("Camera Shake Targets an Entity [{}] with Player Controller [{}] but has NO Player Camera Manager!"), TargetEntity, TargetPlayerController);
            return;
        }

        const auto& Params = InParams.Get_Params();
        const auto& CameraShake = Params.Get_CameraShake();
        const auto& Scale = Params.Get_Scale();

        camera::VeryVerbose(TEXT("Playing CameraShake [{}] on Target Entity [{}] with Player Controller [{}]"), CameraShake, TargetEntity, TargetPlayerController);

        CameraManager->StartCameraShake(CameraShake, Scale);
    }

    auto
        FProcessor_CameraShake_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_CameraShake_Params& InParams,
            const FCk_Request_CameraShake_PlayAtLocation& InRequest) const
        -> void
    {
        const auto& WorldContextObject = InRequest.Get_WorldContextObject();

        CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(WorldContextObject)
        { return; }

        const auto& Params                 = InParams.Get_Params();
        const auto& CameraShake            = Params.Get_CameraShake();
        const auto& InnerRadius            = Params.Get_InnerRadius();
        const auto& OuterRadius            = Params.Get_OuterRadius();
        const auto& FallOff                = Params.Get_Falloff();
        const auto& OrientTowardsEpicenter = Params.Get_OrientTowardsEpicenter();
        const auto& ShakeLocation          = InRequest.Get_Location();

        camera::VeryVerbose(TEXT("Playing CameraShake [{}] at Location [{}]"), CameraShake, ShakeLocation);

        UGameplayStatics::PlayWorldCameraShake(WorldContextObject->GetWorld(), CameraShake, ShakeLocation, InnerRadius, OuterRadius, FallOff, OrientTowardsEpicenter);
    }
}

// --------------------------------------------------------------------------------------------------------------------
