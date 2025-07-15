#include "CkAcceleration_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkEcs/Transform/CkTransform_Utils.h"
#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Acceleration_Setup::
        DoTick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Acceleration_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Params& InParams,
            FFragment_Acceleration_Current& InCurrent) const
        -> void
    {
        const auto& Params = InParams.Get_Params();

        switch(const auto& Coordinates = Params.Get_Coordinates())
        {
            case ECk_LocalWorld::Local:
            {
                const auto DoGetRotationFromEntityOrTargetEntity = [&]() -> FRotator
                {
                    if (UCk_Utils_Transform_UE::Has(InHandle))
                    {
                        const auto HandleTransform = UCk_Utils_Transform_UE::CastChecked(InHandle);
                        return UCk_Utils_Transform_UE::Get_EntityCurrentRotation(HandleTransform);
                    }

                    CK_ENSURE_IF_NOT(UCk_Utils_Acceleration_UE::AccelerationTarget_Utils::Has(InHandle),
                        TEXT("Entity [{}] does NOT have Transform info nor does it have an AccelerationTarget. "
                             "Unable to convert Acceleration to LOCAL coordinates"),
                        InHandle)
                    { return {}; }

                    const auto AccelerationTarget = UCk_Utils_Acceleration_UE::AccelerationTarget_Utils::Get_StoredEntity(InHandle);

                    const auto HandleTransform = UCk_Utils_Transform_UE::CastChecked(AccelerationTarget);
                    return UCk_Utils_Transform_UE::Get_EntityCurrentRotation(HandleTransform);
                };

                const auto& Rotation = DoGetRotationFromEntityOrTargetEntity();
                InCurrent._CurrentAcceleration = Rotation.RotateVector(Params.Get_StartingAcceleration());
                break;
            }
            case ECk_LocalWorld::World:
            {
                InCurrent._CurrentAcceleration = Params.Get_StartingAcceleration();
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
        FProcessor_AccelerationModifier_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Current& InAcceleration,
            const FFragment_Acceleration_Target& InTarget) const
        -> void
    {
        auto TargetEntity  = InTarget.Get_Entity();
        auto& TargetAcceleration = TargetEntity.Get<FFragment_Acceleration_Current>();

        TargetAcceleration._CurrentAcceleration += InAcceleration.Get_CurrentAcceleration();

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AccelerationModifier_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Current& InAcceleration,
            const FFragment_Acceleration_Target& InTarget) const
        -> void
    {
        auto TargetEntity = InTarget.Get_Entity();
        auto& TargetAcceleration = TargetEntity.Get<FFragment_Acceleration_Current>();

        TargetAcceleration._CurrentAcceleration -= InAcceleration._CurrentAcceleration;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_BulkAccelerationModifier_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BulkAccelerationModifier_Params& InParams) const
        -> void
    {
        const auto& TargetAccelerationChannels = InParams.Get_Params().Get_TargetChannels();

        InHandle.View<FFragment_RecordOfAccelerationChannels>().ForEach(
        [&](FCk_Entity InEntity, const FFragment_RecordOfAccelerationChannels& InAccelerationChannels)
        {
            const auto& TargetEntity = MakeHandle(InEntity, InHandle);

            if (NOT UCk_Utils_AccelerationChannel_UE::Get_IsAffectedByAnyOtherChannel(TargetEntity, TargetAccelerationChannels))
            { return; }

            UCk_Utils_BulkAccelerationModifier_UE::DoRequest_AddTarget
            (
                InHandle,
                FCk_Request_BulkAccelerationModifier_AddTarget{TargetEntity}
            );
        });

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_BulkAccelerationModifier_AddNewTargets::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Params& InParams,
            const FFragment_RecordOfAccelerationChannels& InAccelerationChannels) const
        -> void
    {
        InHandle.View<FFragment_BulkAccelerationModifier_Params, FTag_BulkAccelerationModifier_GlobalScope>().ForEach(
        [&](FCk_Entity InModifierEntity, const FFragment_BulkAccelerationModifier_Params& InMultiTargetAccelerationModifierParams)
        {
            if (NOT UCk_Utils_AccelerationChannel_UE::Get_IsAffectedByAnyOtherChannel(InHandle, InMultiTargetAccelerationModifierParams.Get_Params().Get_TargetChannels()))
            { return; }

            UCk_Utils_BulkAccelerationModifier_UE::DoRequest_AddTarget
            (
                MakeHandle(InModifierEntity, InHandle),
                FCk_Request_BulkAccelerationModifier_AddTarget{InHandle}
            );
        });
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_BulkAccelerationModifier_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BulkAccelerationModifier_Params& InParams,
            FFragment_BulkAccelerationModifier_Requests& InRequests) const
        -> void
    {
        const auto& TargetAccelerationChannels = InParams.Get_Params().Get_TargetChannels();

        algo::ForEachRequest(InRequests._Requests, ck::Visitor(
        [&](const auto& InRequest)
        {
            const auto& TargetEntity = InRequest.Get_TargetEntity();

            // Entity may have been destroyed before we got a chance to process it
            if (ck::Is_NOT_Valid(TargetEntity))
            { return; }

            if (NOT UCk_Utils_AccelerationChannel_UE::Get_IsAffectedByAnyOtherChannel(TargetEntity, TargetAccelerationChannels))
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
        FProcessor_BulkAccelerationModifier_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_BulkAccelerationModifier_Params& InParams,
            const FCk_Request_BulkAccelerationModifier_AddTarget& InRequest)
        -> void
    {
        const auto& TargetEntity = InRequest.Get_TargetEntity();

        UCk_Utils_AccelerationModifier_UE::Add
        (
            TargetEntity,
            UCk_Utils_GameplayLabel_UE::Get_Label(InHandle),
            FCk_Fragment_AccelerationModifier_ParamsData
            {
                InParams.Get_Params().Get_AccelerationParams()
            }
        );
    }

    auto
        FProcessor_BulkAccelerationModifier_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_BulkAccelerationModifier_Params& InParams,
            const FCk_Request_BulkAccelerationModifier_RemoveTarget& InRequest)
        -> void
    {
        const auto& TargetEntity = InRequest.Get_TargetEntity();

        UCk_Utils_AccelerationModifier_UE::Remove
        (
            TargetEntity,
            UCk_Utils_GameplayLabel_UE::Get_Label(InHandle)
        );
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Acceleration_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Acceleration_Rep>& InVelRepComp) const
        -> void
    {
        // TODO: Remove usage of UpdateReplicatedFragment once the processor is tagged to only run on Server
        UCk_Utils_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_Acceleration_Rep>(InHandle, [&](UCk_Fragment_Acceleration_Rep* InRepComp)
        {
            InRepComp->Set_Acceleration(InCurrent.Get_CurrentAcceleration());
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------