#include "CkVelocity_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkCore/Math/Vector/CkVector_Utils.h"
#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"
#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkPhysics/CkPhysics_Log.h"
#include "CkPhysics/CkPhysics_Stats.h"
#include "CkPhysics/PredictedVelocity/CkPredictedVelocity_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Physics::BulkVelocity Setup Scan"), STAT_CkPhysics_BulkVelocitySetupScan, STATGROUP_CkPhysics);
DECLARE_CYCLE_STAT(TEXT("Physics::BulkVelocity AddTargets Scan"), STAT_CkPhysics_BulkVelocityAddTargetsScan, STATGROUP_CkPhysics);
DECLARE_DWORD_COUNTER_STAT(TEXT("Physics BulkVelocity Targets Matched"), STAT_CkPhysics_BulkVelocityTargetsMatched, STATGROUP_CkPhysics);

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_Velocity_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Velocity_Clamp);
CK_REGISTER_PROCESSOR(ck::FProcessor_VelocityModifier_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_VelocityModifier_EndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_BulkVelocityModifier_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_BulkVelocityModifier_AddNewTargets);
CK_REGISTER_PROCESSOR(ck::FProcessor_BulkVelocityModifier_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_BulkVelocityModifier_CancelPendingRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Velocity_Replicate);

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
                // Velocity replicates, so the fallback tracker is only needed on auth.
                else if (UCk_Utils_Net_UE::Get_EntityNetMode(InHandle) == ECk_Net_NetModeType::Host)
                {
                    UCk_Utils_PredictedVelocity_UE::Add(InHandle, {});
                }
            }
        }

        // The regular Velocity fragments are still added even when a MovementComponent makes them
        // inapplicable — they are what the gameplay debugger reads.

        // Everything above is Session state Setup owns. The velocity itself is NOT: Add already seeded
        // _CurrentVelocity from the params, and on a load the SAVED velocity is applied BEFORE Setup runs, so
        // re-deriving it here would overwrite a restored velocity with the starting one on every load. What is
        // genuinely deferred to Setup is the LOCAL->world conversion, which needs a Transform that may not
        // exist yet at Add time — and the entities that still owe it are the ones Add MARKED, not the ones
        // whose value happens to read back equal to the starting param. A rotated entity restored to a
        // velocity numerically equal to its local starting value is the case a value comparison converts twice.
        if (NOT InHandle.Try_Remove<FTag_Velocity_NeedsWorldConversion>())
        { return; }

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

            const auto VelocityTarget = UCk_Utils_Velocity_UE::VelocityTarget_Utils::Get_StoredEntity(InHandle);
            return UCk_Utils_Transform_UE::Get_EntityCurrentRotation(VelocityTarget);
        };

        const auto& Rotation = DoGet_RotationFromEntityOrTargetEntity();
        InCurrent._CurrentVelocity = Rotation.RotateVector(InParams.Get_Params().Get_StartingVelocity());
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
        FProcessor_VelocityModifier_EndPlay::
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

        {
            SCOPE_CYCLE_COUNTER(STAT_CkPhysics_BulkVelocitySetupScan);

            InHandle.View<FFragment_RecordOfVelocityChannels>().ForEach(
            [&](FCk_Entity InEntity, const FFragment_RecordOfVelocityChannels& InVelocityChannels)
            {
                const auto& TargetEntity = MakeHandle(InEntity, InHandle);

                if (NOT UCk_Utils_VelocityChannel_UE::Get_IsAffectedByAnyOtherChannel(TargetEntity, TargetVelocityChannels))
                { return; }

                INC_DWORD_STAT(STAT_CkPhysics_BulkVelocityTargetsMatched);

                UCk_Utils_BulkVelocityModifier_UE::DoRequest_AddTarget
                (
                    InHandle,
                    FCk_Request_BulkVelocityModifier_AddTarget{TargetEntity}
                );
            });
        }

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
        SCOPE_CYCLE_COUNTER(STAT_CkPhysics_BulkVelocityAddTargetsScan);

        InHandle.View<FFragment_BulkVelocityModifier_Params, FTag_BulkVelocityModifier_GlobalScope>().ForEach(
        [&](FCk_Entity InModifierEntity, const FFragment_BulkVelocityModifier_Params& InMultiTargetVelocityModifierParams)
        {
            if (NOT UCk_Utils_VelocityChannel_UE::Get_IsAffectedByAnyOtherChannel(InHandle, InMultiTargetVelocityModifierParams.Get_Params().Get_TargetChannels()))
            { return; }

            INC_DWORD_STAT(STAT_CkPhysics_BulkVelocityTargetsMatched);

            auto ModifierHandle = MakeHandle(InModifierEntity, InHandle);
            UCk_Utils_BulkVelocityModifier_UE::DoRequest_AddTarget
            (
                ModifierHandle,
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
            auto Result = ECk_Request_OperationResult::Failed;
            const auto Guard = MakeCompletionGuard(InRequest, InHandle, Result);

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

            Result = ECk_Request_OperationResult::Succeeded;
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
        auto TargetEntity = InRequest.Get_TargetEntity();

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
        auto TargetEntity = InRequest.Get_TargetEntity();

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
            const FFragment_ContainerRef_Velocity& InContainerRef) const
        -> void
    {
        const auto Produced = UCk_Utils_Net_UE::TryProduce<FCk_RepData_Velocity>(InHandle);
        if (Produced.IsSet())
        { UCk_Utils_Net_UE::TryUpdateContainerFragment<FCk_RepData_Velocity>(InHandle, *Produced); }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_BulkVelocityModifier_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_BulkVelocityModifier_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }

}

// --------------------------------------------------------------------------------------------------------------------