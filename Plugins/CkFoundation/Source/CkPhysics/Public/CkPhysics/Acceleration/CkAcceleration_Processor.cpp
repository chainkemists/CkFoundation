#include "CkAcceleration_Processor.h"

#include "CkEcsBasics/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FCk_Processor_Acceleration_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FCk_Fragment_Acceleration_Params& InParams,
            FCk_Fragment_Acceleration_Current& InCurrent) const
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
}

// --------------------------------------------------------------------------------------------------------------------