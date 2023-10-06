#include "CkTransform_Fragment.h"

#include "CkTransform_Utils.h"

#include "CkEcsBasics/CkEcsBasics_Log.h"

#include "Net/UnrealNetwork.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_Transform_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(ThisType, _Location);
    DOREPLIFETIME(ThisType, _Rotation);
    DOREPLIFETIME(ThisType, _Scale);
}

auto
    UCk_Fragment_Transform_Rep::
    OnRep_Location() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        const auto& CurrentLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(Get_AssociatedEntity());
        UCk_Utils_Transform_UE::Request_SetInterpolationGoal_LocationOffset
        (
            Get_AssociatedEntity(),
            _Location - CurrentLocation
        );
    });
}

auto
    UCk_Fragment_Transform_Rep::
    OnRep_Rotation() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        const auto& CurrentRotation = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(Get_AssociatedEntity());
        UCk_Utils_Transform_UE::Request_SetInterpolationGoal_RotationOffset
        (
            Get_AssociatedEntity(),
            _Rotation.Rotator() - CurrentRotation
        );
    });
}

auto
    UCk_Fragment_Transform_Rep::
    OnRep_Scale() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        UCk_Utils_Transform_UE::Request_SetScale
        (
            Get_AssociatedEntity(),
            FCk_Request_Transform_SetScale{_Scale}.Set_RelativeAbsolute(ECk_RelativeAbsolute::Absolute)
        );
    });
}

// --------------------------------------------------------------------------------------------------------------------
