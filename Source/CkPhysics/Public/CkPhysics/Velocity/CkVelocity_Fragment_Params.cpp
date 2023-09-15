#include "CkVelocity_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Velocity_ParamsData::
    FCk_Fragment_Velocity_ParamsData(
        ECk_LocalWorld InCoordinates,
        FVector InStartingVelocity)
    : _Coordinates(InCoordinates)
    , _StartingVelocity(InStartingVelocity)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_VelocityModifier_SingleTarget_ParamsData::
    FCk_Fragment_VelocityModifier_SingleTarget_ParamsData(
        FCk_Fragment_Velocity_ParamsData InVelocityParams)
    : _VelocityParams(InVelocityParams)
{
}

// --------------------------------------------------------------------------------------------------------------------
