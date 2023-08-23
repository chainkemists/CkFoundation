#include "CkAcceleration_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Fragment_Acceleration_ParamsData::
    FCk_Fragment_Acceleration_ParamsData(
        ECk_LocalWorld InCoordinates,
        FVector InStartingAcceleration)
    : _Coordinates(InCoordinates)
    , _StartingAcceleration(InStartingAcceleration)
{
}

// --------------------------------------------------------------------------------------------------------------------
