#include "CkTransform_Fragment_Params.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Transform_SetLocation::
    FCk_Request_Transform_SetLocation(
        FVector InNewLocation,
        ECk_RelativeAbsolute InRelativeAbsolute)
    : _NewLocation(InNewLocation)
    , _RelativeAbsolute(InRelativeAbsolute)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Transform_AddLocationOffset::
    FCk_Request_Transform_AddLocationOffset(
        FVector InDeltaLocation,
        ECk_LocalWorld InLocalWorld)
    : _DeltaLocation(InDeltaLocation)
    , _LocalWorld(InLocalWorld)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Transform_SetRotation::
    FCk_Request_Transform_SetRotation(
        FRotator InNewLocation,
        ECk_RelativeAbsolute InRelativeAbsolute)
    : _NewRotation(InNewLocation)
    , _RelativeAbsolute(InRelativeAbsolute)

{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Transform_AddRotationOffset::
    FCk_Request_Transform_AddRotationOffset(
        FRotator InDeltaRotation,
        ECk_LocalWorld InLocalWorld)
    : _DeltaRotation(InDeltaRotation)
    , _LocalWorld(InLocalWorld)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Transform_SetScale::
    FCk_Request_Transform_SetScale(
        FVector InNewScale,
        ECk_RelativeAbsolute InRelativeAbsolute)
    : _NewScale(InNewScale)
    , _RelativeAbsolute(InRelativeAbsolute)
{
}

// --------------------------------------------------------------------------------------------------------------------

FCk_Request_Transform_SetTransform::
    FCk_Request_Transform_SetTransform(
        FTransform InNewTransform,
        ECk_RelativeAbsolute InRelativeAbsolute)
    : _NewTransform(InNewTransform)
    , _RelativeAbsolute(InRelativeAbsolute)
{
}

// --------------------------------------------------------------------------------------------------------------------
