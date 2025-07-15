#include "CkVelocity_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"
#include "CkEcs/Transform/CkTransform_Utils.h"

#include "CkPhysics/PredictedVelocity/CkPredictedVelocity_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Utils.h"

#include "GameFramework/Character.h"
#include "GameFramework/PawnMovementComponent.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Velocity_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Velocity_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Params& InParams,
            FFragment_Velocity_Current& InCurrent) const
        -> void
    {
        if (UCk_Utils_OwningActor_UE::Has(InHandle))
        {
            if (const auto Actor = UCk_Utils_OwningActor_UE::Get_EntityOwningActor(InHandle);
                ck::IsValid(Actor))
            {
                if (const auto MovementComponent = Actor->GetComponentByClass<UMovementComponent>();
                    ck::IsValid(MovementComponent))
                {
                    InHandle.Add<ck::FFragment_MovementComponent>(MovementComponent);
                }
                // If there isn't a movement component, use predicted velocity feature to track velocity
                // Since velocity is replicated, we only need predicted velocity on auth
                else if (UCk_Utils_Net_UE::Get_EntityNetMode(InHandle) == ECk_Net_NetModeType::Host)
                {
                    UCk_Utils_PredictedVelocity_UE::Add(InHandle, {});
                }
            }
        }

        // We continue adding the regular Velocity Fragments even if they are not applicable anymore IF we
        // have the MovementComponent. This is mainly for us to be able to debug (and add gameplay debugger breadcrumbs)

        const auto& Params = InParams.Get_Params();

        switch(const auto& Coordinates = Params.Get_Coordinates())
        {
            case ECk_LocalWorld::Local:
            {
                const auto DoGet_RotationFromEntityOrTargetEntity = [&]() -> FRotator
                {
                    if (UCk_Utils_Transform_UE::Has(InHandle))
                    {
                        const auto HandleTransform = UCk_Utils_Transform_UE::CastChecked(InHandle);
                        return UCk_Utils_Transform_UE::Get_EntityCurrentRotation(HandleTransform);
                    }

                    CK_ENSURE_IF_NOT(UCk_Utils_Velocity_UE::VelocityTarget_Utils::Has(InHandle),
                        TEXT("Entity [{}] does NOT have Transform info nor does it have an VelocityTarget. "
                             "Unable to convert Velocity to LOCAL coordinates"),
                        InHandle)
                    { return {}; }

                    const auto AccelerationTarget = UCk_Utils_Velocity_UE::VelocityTarget_Utils::Get_StoredEntity(InHandle);

                    const auto HandleTransform = UCk_Utils_Transform_UE::CastChecked(AccelerationTarget);
                    return UCk_Utils_Transform_UE::Get_EntityCurrentRotation(HandleTransform);
                };

                const auto& Rotation = DoGet_RotationFromEntityOrTargetEntity();
                InCurrent._CurrentVelocity = Rotation.RotateVector(Params.Get_StartingVelocity());
                break;
            }
            case ECk_LocalWorld::World:
            {
                InCurrent._CurrentVelocity = Params.Get_StartingVelocity();
                break;
            }
            default:
            {
                CK_INVALID_ENUM(Coordinates);
                break;
            }
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Velocity_Clamp::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Velocity_Current& InCurrent,
            const FFragment_Velocity_MinMax& InMinMax) const
        -> void
    {
        const auto& CurrentVelocity = InCurrent.Get_CurrentVelocity();
        const auto& ClampMin = InMinMax.Get_MinSpeed().Get(0.0f);
        const auto& ClampMax = InMinMax.Get_MaxSpeed().Get(CurrentVelocity.Length());

        const auto ClampRange = FCk_FloatRange{ClampMin, ClampMax};

        InCurrent._CurrentVelocity = UCk_Utils_Vector3_UE::Get_ClampedLength(CurrentVelocity, ClampRange);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VelocityModifier_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InVelocity,
            const FFragment_Velocity_Target& InTarget) const
        -> void
    {
        auto TargetEntity  = InTarget.Get_Entity();
        auto& TargetVelocity = TargetEntity.Get<FFragment_Velocity_Current>();

        TargetVelocity._CurrentVelocity += InVelocity.Get_CurrentVelocity();

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_VelocityModifier_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InVelocity,
            const FFragment_Velocity_Target& InTarget) const
        -> void
    {
        auto TargetEntity = InTarget.Get_Entity();
        auto& TargetVelocity = TargetEntity.Get<FFragment_Velocity_Current>();

        TargetVelocity._CurrentVelocity -= InVelocity._CurrentVelocity;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_BulkVelocityModifier_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BulkVelocityModifier_Params& InParams) const
        -> void
    {
        const auto& TargetVelocityChannels = InParams.Get_Params().Get_TargetChannels();

        InHandle.View<FFragment_RecordOfVelocityChannels>().ForEach(
        [&](FCk_Entity InEntity, const FFragment_RecordOfVelocityChannels& InVelocityChannels)
        {
            const auto& TargetEntity = MakeHandle(InEntity, InHandle);

            if (NOT UCk_Utils_VelocityChannel_UE::Get_IsAffectedByAnyOtherChannel(TargetEntity, TargetVelocityChannels))
            { return; }

            UCk_Utils_BulkVelocityModifier_UE::DoRequest_AddTarget
            (
                InHandle,
                FCk_Request_BulkVelocityModifier_AddTarget{TargetEntity}
            );
        });

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_BulkVelocityModifier_AddNewTargets::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Params& InParams,
            const FFragment_RecordOfVelocityChannels& InVelocityChannels) const
        -> void
    {
        InHandle.View<FFragment_BulkVelocityModifier_Params, FTag_BulkVelocityModifier_GlobalScope>().ForEach(
        [&](FCk_Entity InModifierEntity, const FFragment_BulkVelocityModifier_Params& InMultiTargetVelocityModifierParams)
        {
            if (NOT UCk_Utils_VelocityChannel_UE::Get_IsAffectedByAnyOtherChannel(InHandle, InMultiTargetVelocityModifierParams.Get_Params().Get_TargetChannels()))
            { return; }

            UCk_Utils_BulkVelocityModifier_UE::DoRequest_AddTarget
            (
                MakeHandle(InModifierEntity, InHandle),
                FCk_Request_BulkVelocityModifier_AddTarget{InHandle}
            );
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_BulkVelocityModifier_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType& InHandle,
            const FFragment_BulkVelocityModifier_Params& InParams,
            FFragment_BulkVelocityModifier_Requests& InRequests) const
        -> void
    {
        const auto& TargetVelocityChannels = InParams.Get_Params().Get_TargetChannels();

        algo::ForEachRequest(InRequests._Requests, ck::Visitor(
        [&](const auto& InRequest)
        {
            const auto& TargetEntity = InRequest.Get_TargetEntity();

            // Entity may have been destroyed before we got a chance to process it
            if (ck::Is_NOT_Valid(TargetEntity))
            { return; }

            if (NOT UCk_Utils_VelocityChannel_UE::Get_IsAffectedByAnyOtherChannel(TargetEntity, TargetVelocityChannels))
            { return; }

            DoHandleRequest(InHandle, InParams, InRequest);

            if (InRequest.Get_IsRequestHandleValid())
            {
                InRequest.GetAndDestroyRequestHandle();
            }
        }));

        InHandle.Remove<MarkedDirtyBy>();
    }

    auto
        FProcessor_BulkVelocityModifier_HandleRequests::
        DoHandleRequest(
            const HandleType& InHandle,
            const FFragment_BulkVelocityModifier_Params& InParams,
            const FCk_Request_BulkVelocityModifier_AddTarget& InRequest)
        -> void
    {
        const auto& TargetEntity = InRequest.Get_TargetEntity();

        UCk_Utils_VelocityModifier_UE::Add
        (
            TargetEntity,
            UCk_Utils_GameplayLabel_UE::Get_Label(InHandle),
            FCk_Fragment_VelocityModifier_ParamsData
            {
                InParams.Get_Params().Get_VelocityParams()
            }
        );
    }

    auto
        FProcessor_BulkVelocityModifier_HandleRequests::
        DoHandleRequest(
            const HandleType& InHandle,
            const FFragment_BulkVelocityModifier_Params& InParams,
            const FCk_Request_BulkVelocityModifier_RemoveTarget& InRequest)
        -> void
    {
        const auto& TargetEntity = InRequest.Get_TargetEntity();

        UCk_Utils_VelocityModifier_UE::Remove
        (
            TargetEntity,
            UCk_Utils_GameplayLabel_UE::Get_Label(InHandle)
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Velocity_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Velocity_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Velocity_Rep>& InVelRepComp) const
        -> void
    {
        // TODO: Remove usage of UpdateReplicatedFragment once the processor is tagged to only run on Server
        UCk_Utils_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_Velocity_Rep>(InHandle, [&](UCk_Fragment_Velocity_Rep* InRepComp)
        {
            InRepComp->Set_Velocity(InCurrent.Get_CurrentVelocity());
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------