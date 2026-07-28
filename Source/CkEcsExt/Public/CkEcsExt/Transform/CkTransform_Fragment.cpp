#include "CkTransform_Fragment.h"

#include "CkTransform_Utils.h"

#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.h"
#include "CkEcs/Persistence/CkPersistenceHandlerRegistry.inl.h" // Register_* entry-point bodies

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

static struct FTransformRepHandlerRegistrar
{
    FTransformRepHandlerRegistrar()
    {
        // Transform is composed pre-link, so the NotReady branches below are violation detectors, not an
        // expected wait. An unset Old means first application: snap directly (an interpolation offset
        // would glide the entity in from origin).

        FCk_PersistenceHandlerRegistry::Register_NetOnly<FCk_RepData_Location>({ .NetApply =
                [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                {
                    auto HandleTransform = UCk_Utils_Transform_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(HandleTransform))
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    const auto& Location = New.Get<FCk_RepData_Location>().Value;

                    if (Old.IsSet() && UCk_Utils_TransformInterpolation_UE::Has(HandleTransform))
                    {
                        auto HandleTransformInterpolation = UCk_Utils_TransformInterpolation_UE::CastChecked(Entity);
                        const auto CurrentLoc = UCk_Utils_Transform_UE::Get_EntityCurrentLocation(HandleTransform);
                        UCk_Utils_TransformInterpolation_UE::Request_SetInterpolationGoal_LocationOffset(
                            HandleTransformInterpolation, Location - CurrentLoc);
                        return ECk_Persistence_ApplyResult::Applied;
                    }

                    UCk_Utils_Transform_UE::Request_SetLocation(HandleTransform, FCk_Request_Transform_SetLocation{Location}, {});
                    return ECk_Persistence_ApplyResult::Applied;
                } });

        FCk_PersistenceHandlerRegistry::Register_NetOnly<FCk_RepData_Rotation>({ .NetApply =
                [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& Old) -> ECk_Persistence_ApplyResult
                {
                    auto HandleTransform = UCk_Utils_Transform_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(HandleTransform))
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    const auto& Rotation = New.Get<FCk_RepData_Rotation>().Value;

                    if (Old.IsSet() && UCk_Utils_TransformInterpolation_UE::Has(HandleTransform))
                    {
                        auto HandleTransformInterpolation = UCk_Utils_TransformInterpolation_UE::CastChecked(Entity);
                        const auto CurrentRot = UCk_Utils_Transform_UE::Get_EntityCurrentRotation(HandleTransform);
                        UCk_Utils_TransformInterpolation_UE::Request_SetInterpolationGoal_RotationOffset(
                            HandleTransformInterpolation, Rotation.Rotator() - CurrentRot);
                        return ECk_Persistence_ApplyResult::Applied;
                    }

                    UCk_Utils_Transform_UE::Request_SetRotation(HandleTransform, FCk_Request_Transform_SetRotation{Rotation.Rotator()}, {});
                    return ECk_Persistence_ApplyResult::Applied;
                } });

        FCk_PersistenceHandlerRegistry::Register_NetOnly<FCk_RepData_Scale>({ .NetApply =
                [](FCk_Handle& Entity, const FInstancedStruct& New, const TOptional<FInstancedStruct>& /*Old*/) -> ECk_Persistence_ApplyResult
                {
                    auto HandleTransform = UCk_Utils_Transform_UE::Cast(Entity);
                    if (ck::Is_NOT_Valid(HandleTransform))
                    { return ECk_Persistence_ApplyResult::NotReady; }

                    UCk_Utils_Transform_UE::Request_SetScale(HandleTransform,
                        FCk_Request_Transform_SetScale{New.Get<FCk_RepData_Scale>().Value}
                            .Set_LocalWorld(ECk_LocalWorld::World), {});
                    return ECk_Persistence_ApplyResult::Applied;
                } });
    }
} GTransformRepHandlerRegistrar;

// --------------------------------------------------------------------------------------------------------------------