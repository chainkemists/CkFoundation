#include "CkVelocity_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"
#include "CkEcsBasics/Transform/CkTransform_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Velocity_Setup::
        Tick(
            TimeType InDeltaT)
        -> void
    {
        TProcessor::Tick(InDeltaT);

        _Registry.Clear<MarkedDirtyBy>();
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
        const auto& Params = InParams.Get_Params();

        switch(const auto& Coordinates = Params.Get_Coordinates())
        {
            case ECk_LocalWorld::Local:
            {
                const auto DoGet_RotationFromEntityOrTargetEntity = [&]() -> FRotator
                {
                    if (UCk_Utils_Transform_UE::Has(InHandle))
                    {
                        return UCk_Utils_Transform_UE::Get_EntityCurrentRotation(InHandle);
                    }

                    CK_ENSURE_IF_NOT(UCk_Utils_Velocity_UE::VelocityTarget_Utils::Has(InHandle),
                        TEXT("Entity [{}] does NOT have Transform info nor does it have an VelocityTarget. "
                             "Unable to convert Velocity to LOCAL coordinates"),
                        InHandle)
                    { return {}; }

                    const auto AccelerationTarget = UCk_Utils_Velocity_UE::VelocityTarget_Utils::Get_StoredEntity(InHandle);

                    return UCk_Utils_Transform_UE::Get_EntityCurrentRotation(AccelerationTarget);
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
        InHandle.Get_Registry().View<FFragment_BulkVelocityModifier_Params, FTag_BulkVelocityModifier_GlobalScope>().ForEach(
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
            HandleType InHandle,
            const FFragment_BulkVelocityModifier_Params& InParams,
            FFragment_BulkVelocityModifier_Requests& InRequests) const
        -> void
    {
        const auto& TargetVelocityChannels = InParams.Get_Params().Get_TargetChannels();

        algo::ForEachRequest(InRequests._Requests, ck::Visitor(
        [&](const auto& InRequestVariant)
        {
            const auto& TargetEntity = InRequestVariant.Get_TargetEntity();

            // Entity may have been destroyed before we got a chance to process it
            if (ck::Is_NOT_Valid(TargetEntity))
            { return; }

            if (NOT UCk_Utils_VelocityChannel_UE::Get_IsAffectedByAnyOtherChannel(TargetEntity, TargetVelocityChannels))
            { return; }

            DoHandleRequest(InHandle, InParams, InRequestVariant);
        }));

        InHandle.Remove<MarkedDirtyBy>();
    }

    auto
        FProcessor_BulkVelocityModifier_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
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
            HandleType InHandle,
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
        UCk_Utils_Ecs_Net_UE::UpdateReplicatedFragment<UCk_Fragment_Velocity_Rep>(InHandle, [&](UCk_Fragment_Velocity_Rep* InRepComp)
        {
            InRepComp->_Velocity = InCurrent.Get_CurrentVelocity();
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------