#include "CkAcceleration_Processor.h"

#include "CkEcsBasics/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Fragment.h"
#include "CkPhysics/Acceleration/CkAcceleration_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Acceleration_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Params& InParams,
            FFragment_Acceleration_Current& InCurrent) const
        -> void
    {
        const auto& params = InParams.Get_Params();

        switch(const auto& coordinates = params.Get_Coordinates())
        {
            case ECk_LocalWorld::Local:
            {
                const auto& rotation = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(InHandle);
                InCurrent._CurrentAcceleration = rotation.RotateVector(params.Get_StartingAcceleration());
                break;
            }
            case ECk_LocalWorld::World:
            {
                InCurrent._CurrentAcceleration = params.Get_StartingAcceleration();
                break;
            }
            default:
            {
                CK_INVALID_ENUM(coordinates);
                break;
            }
        }

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AccelerationModifier_SingleTarget_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Current& InAcceleration,
            const FFragment_Acceleration_Target& InTarget) const
        -> void
    {
        auto targetEntity  = InTarget.Get_Entity();
        auto& targetAcceleration = targetEntity.Get<FFragment_Acceleration_Current>();

        targetAcceleration._CurrentAcceleration += InAcceleration.Get_CurrentAcceleration();

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_AccelerationModifier_SingleTarget_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Current& InAcceleration,
            const FFragment_Acceleration_Target& InTarget) const
        -> void
    {
        auto targetEntity = InTarget.Get_Entity();
        auto& targetAcceleration = targetEntity.Get<FFragment_Acceleration_Current>();

        targetAcceleration._CurrentAcceleration -= InAcceleration._CurrentAcceleration;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Acceleration_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Acceleration_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Acceleration_Rep>& InAccelRepComp) const
        -> void
    {
        // TODO: Remove usage of UpdateReplicatedFragment once the processor is tagged to only run on Server
        UCk_Utils_Ecs_Net_UE::UpdateReplicatedFragment<UCk_Fragment_Acceleration_Rep>(InHandle, [&](UCk_Fragment_Acceleration_Rep* InRepComp)
        {
            InRepComp->_Acceleration = InCurrent.Get_CurrentAcceleration();
        });
    }

}
// --------------------------------------------------------------------------------------------------------------------