#include "CkVelocity_Processor.h"

#include "CkEcsBasics/Transform/CkTransform_Utils.h"
#include "CkPhysics/Velocity/CkVelocity_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FCk_Processor_Velocity_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Velocity_Params& InParams,
            FCk_Fragment_Velocity_Current& InCurrent) const
        -> void
    {
        const auto& params = InParams.Get_Params();

        switch(const auto& coordinates = params.Get_Coordinates())
        {
            case ECk_LocalWorld::Local:
            {
                const auto& rotation = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(InHandle);
                InCurrent._CurrentVelocity = rotation.RotateVector(params.Get_StartingVelocity());
                break;
            }
            case ECk_LocalWorld::World:
            {
                InCurrent._CurrentVelocity = params.Get_StartingVelocity();
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
        FCk_Processor_VelocityModifier_SingleTarget_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Velocity_Current& InVelocity,
            const FCk_Fragment_Velocity_Target& InTarget) const
        -> void
    {
        auto targetEntity  = InTarget.Get_Entity();
        auto& targetVelocity = targetEntity.Get<FCk_Fragment_Velocity_Current>();

        targetVelocity._CurrentVelocity += InVelocity.Get_CurrentVelocity();

        InHandle.Remove<MarkedDirtyBy>();
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_VelocityModifier_SingleTarget_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Velocity_Current& InVelocity,
            const FCk_Fragment_Velocity_Target& InTarget) const
        -> void
    {
        auto targetEntity  = UCk_Utils_Velocity_UE::VelocityTarget_Utils::Get_StoredEntity(InHandle);
        auto& targetVelocity = targetEntity.Get<FCk_Fragment_Velocity_Current>();

        targetVelocity._CurrentVelocity -= InVelocity._CurrentVelocity;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_Processor_Velocity_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Velocity_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Velocity_Rep>& InRepComp) const
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