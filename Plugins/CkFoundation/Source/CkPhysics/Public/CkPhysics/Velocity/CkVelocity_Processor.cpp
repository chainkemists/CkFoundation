#include "CkVelocity_Processor.h"

#include "CkEcsBasics/Transform/CkTransform_Utils.h"

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
}

// --------------------------------------------------------------------------------------------------------------------