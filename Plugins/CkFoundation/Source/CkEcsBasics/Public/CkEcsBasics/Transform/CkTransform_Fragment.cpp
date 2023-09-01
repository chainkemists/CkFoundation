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
    OnRep_Transform() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        const auto& CurrentLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(Get_AssociatedEntity());

        ck::ecs_basics::Log(TEXT("[UPDATE] Just got a new Goal Value [{}]. Delta Goal is [{}]"), _Location, _Location - CurrentLocation);

        UCk_Utils_Transform_UE::Request_SetInterpolationGoal_Offset
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
        UCk_Utils_Transform_UE::Request_SetRotation
        (
            Get_AssociatedEntity(),
            FCk_Request_Transform_SetRotation{FRotator{_Rotation}, ECk_RelativeAbsolute::Absolute}
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
            FCk_Request_Transform_SetScale{_Scale, ECk_RelativeAbsolute::Absolute}
        );
    });
}

// --------------------------------------------------------------------------------------------------------------------
