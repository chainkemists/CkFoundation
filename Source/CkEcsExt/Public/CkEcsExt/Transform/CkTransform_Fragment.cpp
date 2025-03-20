#include "CkTransform_Fragment.h"

#include "CkTransform_Utils.h"

#include <Net/UnrealNetwork.h>
#include <Net/Core/PushModel/PushModel.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_Transform_SkeletalMeshSocket::
        FFragment_Transform_SkeletalMeshSocket(
            const USkeletalMeshComponent* InComponent,
            const USkeletalMeshSocket* InSocket)
        : _Component(InComponent)
        , _Socket(InSocket)
    {
    }

    FFragment_Transform_StaticMeshSocket::
        FFragment_Transform_StaticMeshSocket(
            const UStaticMeshComponent* InComponent,
            const UStaticMeshSocket* InSocket)
        : _Component(InComponent)
        , _Socket(InSocket)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Fragment_Transform_Rep::
    GetLifetimeReplicatedProps(
        TArray<FLifetimeProperty>& OutLifetimeProps) const
    -> void
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    constexpr auto Params = FDoRepLifetimeParams{COND_None, REPNOTIFY_Always, true};

    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _Location, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _Rotation, Params);
    DOREPLIFETIME_WITH_PARAMS_FAST(ThisType, _Scale, Params);
}

auto
    UCk_Fragment_Transform_Rep::
    PostLink()
    -> void
{
    Super::PostLink();

    const auto& TransformHandle = UCk_Utils_Transform_UE::Cast(Get_AssociatedEntity());

    CK_ENSURE_IF_NOT(ck::IsValid(TransformHandle),
        TEXT("Entity [{}] with a Transform_Rep fragment does NOT have a transform fragment"),
        Get_AssociatedEntity())
    { return; }

    // Make sure the initial values match the transforms values since a rep-notify is sent on client spawned which needs to have the correct info
    const auto& EntityCurrentTransform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(TransformHandle);
    _Location = EntityCurrentTransform.GetLocation();
    _Rotation = EntityCurrentTransform.GetRotation();
    _Scale = EntityCurrentTransform.GetScale3D();
}

auto
    UCk_Fragment_Transform_Rep::
    OnRep_Location() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        auto HandleTransform = UCk_Utils_Transform_UE::CastChecked(_AssociatedEntity);
        const auto& CurrentLocation = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(HandleTransform);

        if (UCk_Utils_TransformInterpolation_UE::Has(HandleTransform))
        {
            auto HandleTransformInterpolation = UCk_Utils_TransformInterpolation_UE::CastChecked(_AssociatedEntity);
            UCk_Utils_TransformInterpolation_UE::Request_SetInterpolationGoal_LocationOffset
            (
                HandleTransformInterpolation,
                _Location - CurrentLocation
            );
            return;
        }

        UCk_Utils_Transform_UE::Request_SetLocation(
            HandleTransform, FCk_Request_Transform_SetLocation{_Location});
    });
}

auto
    UCk_Fragment_Transform_Rep::
    OnRep_Rotation() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        auto HandleTransform = UCk_Utils_Transform_UE::CastChecked(_AssociatedEntity);
        const auto& CurrentRotation = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(HandleTransform);

        if (UCk_Utils_TransformInterpolation_UE::Has(HandleTransform))
        {
            auto HandleTransformInterpolation = UCk_Utils_TransformInterpolation_UE::CastChecked(_AssociatedEntity);
            UCk_Utils_TransformInterpolation_UE::Request_SetInterpolationGoal_RotationOffset
            (
                HandleTransformInterpolation,
                _Rotation.Rotator() - CurrentRotation
            );

            return;
        }

        UCk_Utils_Transform_UE::Request_SetRotation(HandleTransform, FCk_Request_Transform_SetRotation{_Rotation.Rotator()});
    });
}

auto
    UCk_Fragment_Transform_Rep::
    OnRep_Scale() -> void
{
    CK_REP_OBJ_EXECUTE_IF_VALID([&]()
    {
        auto HandleTransform = UCk_Utils_Transform_UE::CastChecked(_AssociatedEntity);
        UCk_Utils_Transform_UE::Request_SetScale
        (
            HandleTransform,
            FCk_Request_Transform_SetScale{_Scale}.Set_LocalWorld(ECk_LocalWorld::World)
        );
    });
}

auto
    UCk_Fragment_Transform_Rep::
    Set_Location(
        const FVector& OutLocation)
    -> void
{
    _Location = OutLocation;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _Location, this);
}

auto
    UCk_Fragment_Transform_Rep::
    Set_Rotation(
        const FQuat& OutRotation)
    -> void
{
    _Rotation = OutRotation;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _Rotation, this);
}

auto
    UCk_Fragment_Transform_Rep::
    Set_Scale(
        const FVector& OutScale)
    -> void
{
    _Scale = OutScale;
    MARK_PROPERTY_DIRTY_FROM_NAME(ThisType, _Scale, this);
}

// --------------------------------------------------------------------------------------------------------------------