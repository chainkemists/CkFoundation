#include "CkTransform_Fragment.h"

#include "CkTransform_Utils.h"

#include "CkEcs/Net/ReplicatedFragmentContainer/CkReplicatedFragmentContainer.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FFragment_Transform_MeshSocket::
        FFragment_Transform_MeshSocket(
            const UMeshComponent* InComponent,
            FName InSocket)
        : _Component(InComponent)
        , _Socket(InSocket)
    {
    }
}

// --------------------------------------------------------------------------------------------------------------------
// Container-based replication handlers for Transform

static struct FTransformRepHandlerRegistrar
{
    FTransformRepHandlerRegistrar()
    {
        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Location::StaticStruct(); },
            {
                .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& /*Old*/)
                {
                    auto HandleTransform = UCk_Utils_Transform_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(HandleTransform))
                    { return; }

                    const auto& Location = New.Get<FCk_RepData_Location>().Value;

                    if (UCk_Utils_TransformInterpolation_UE::Has(HandleTransform))
                    {
                        auto HandleTransformInterpolation = UCk_Utils_TransformInterpolation_UE::CastChecked(Entity);
                        const auto CurrentLoc = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(HandleTransform);
                        UCk_Utils_TransformInterpolation_UE::Request_SetInterpolationGoal_LocationOffset(
                            HandleTransformInterpolation, Location - CurrentLoc);
                        return;
                    }

                    UCk_Utils_Transform_UE::Request_SetLocation(HandleTransform, FCk_Request_Transform_SetLocation{Location});
                }
            });

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Rotation::StaticStruct(); },
            {
                .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& /*Old*/)
                {
                    auto HandleTransform = UCk_Utils_Transform_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(HandleTransform))
                    { return; }

                    const auto& Rotation = New.Get<FCk_RepData_Rotation>().Value;

                    if (UCk_Utils_TransformInterpolation_UE::Has(HandleTransform))
                    {
                        auto HandleTransformInterpolation = UCk_Utils_TransformInterpolation_UE::CastChecked(Entity);
                        const auto CurrentRot = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(HandleTransform);
                        UCk_Utils_TransformInterpolation_UE::Request_SetInterpolationGoal_RotationOffset(
                            HandleTransformInterpolation, Rotation.Rotator() - CurrentRot);
                        return;
                    }

                    UCk_Utils_Transform_UE::Request_SetRotation(HandleTransform, FCk_Request_Transform_SetRotation{Rotation.Rotator()});
                }
            });

        FCk_ReplicatedFragmentHandlerRegistry::RegisterLazy(
            []() -> UScriptStruct* { return FCk_RepData_Scale::StaticStruct(); },
            {
                .OnChange = [](FCk_Handle& Entity, const FInstancedStruct& New, const FInstancedStruct& /*Old*/)
                {
                    auto HandleTransform = UCk_Utils_Transform_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(HandleTransform))
                    { return; }

                    UCk_Utils_Transform_UE::Request_SetScale(HandleTransform,
                        FCk_Request_Transform_SetScale{New.Get<FCk_RepData_Scale>().Value}
                            .Set_LocalWorld(ECk_LocalWorld::World));
                }
            });
    }
} GTransformRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------