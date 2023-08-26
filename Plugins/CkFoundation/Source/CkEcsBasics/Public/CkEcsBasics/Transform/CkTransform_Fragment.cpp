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
    if (NOT Get_AssociatedEntity().IsValid())
    { return; }

    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    const auto& CurrentLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(Get_AssociatedEntity());

    ck::ecs_basics::Log(TEXT("[UPDATE] Just got a new Goal Value [{}]. Delta Goal is [{}]"), _Location, _Location - CurrentLocation);

    UCk_Utils_Transform_UE::Request_SetInterpolationGoal_Location
    (
        Get_AssociatedEntity(),
        FCk_Fragment_Transform_NewGoal_Location{}.Set_InterpolationOffset(_Location - CurrentLocation)
    );
}

auto
    UCk_Fragment_Transform_Rep::
    OnRep_Rotation() -> void
{
    if (NOT Get_AssociatedEntity().IsValid())
    { return; }

    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    UCk_Utils_Transform_UE::Request_SetRotation
    (
        Get_AssociatedEntity(),
        FCk_Request_Transform_SetRotation{FRotator{_Rotation}, ECk_RelativeAbsolute::Absolute}
    );
}

auto
    UCk_Fragment_Transform_Rep::
    OnRep_Scale() -> void
{
    if (NOT Get_AssociatedEntity().IsValid())
    { return; }

    CK_ENSURE_VALID_UNREAL_WORLD_IF_NOT(this)
    { return; }

    if (GetWorld()->IsNetMode(NM_DedicatedServer))
    { return; }

    UCk_Utils_Transform_UE::Request_SetScale
    (
        Get_AssociatedEntity(),
        FCk_Request_Transform_SetScale{_Scale, ECk_RelativeAbsolute::Absolute}
    );
}

// --------------------------------------------------------------------------------------------------------------------
