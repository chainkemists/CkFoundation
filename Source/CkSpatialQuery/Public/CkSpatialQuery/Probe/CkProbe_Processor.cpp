#include "CkProbe_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"

#include <edyn/collision/contact_signal.hpp>
#include <edyn/util/rigidbody.hpp>

// --------------------------------------------------------------------------------------------------------------------

struct EdynStruct
{
    CK_GENERATED_BODY(EdynStruct);

    entt::entity _Entity;

    CK_DEFINE_CONSTRUCTORS(EdynStruct, _Entity);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    FProcessor_Probe_Setup::
    FProcessor_Probe_Setup(
        const RegistryType& InPhysicsRegistry,
        const RegistryType& InGameRegistry)
        : TProcessor(InGameRegistry)
        , _PhysicsRegistry(InPhysicsRegistry)
    {
    }

    auto
        FProcessor_Probe_Setup::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Probe_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent)
            -> void
    {
        const auto& HalfExtents = InParams.Get_Params().Get_HalfExtents();
        const auto& EntityPosition = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(InHandle);

        auto ProbeDef = edyn::rigidbody_def{};
        ProbeDef.kind = edyn::rigidbody_kind::rb_kinematic;
        ProbeDef.material.reset();
        ProbeDef.shape = edyn::box_shape{HalfExtents.X, HalfExtents.Y, HalfExtents.Z};
        ProbeDef.position = edyn::vector3{EntityPosition.X, EntityPosition.Y, EntityPosition.Z};

        _PhysicsRegistry.Request_PerformOperationOnInternalRegistry([&](
            entt::registry& InRegistry)
            {
                auto RbEntity = edyn::make_rigidbody(InRegistry,
                    ProbeDef);

                InHandle.Add<EdynStruct>(RbEntity);

                edyn::on_contact_started(InRegistry).connect<&FProcessor_Probe_Setup::SensorContactStarted>(InRegistry);
            });
    }

    auto
        FProcessor_Probe_Setup::
        SensorContactStarted(
            entt::registry& registry,
            entt::entity entity)
            -> void
    {
        ck::spatialquery::Warning(TEXT("We have contact!"));
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Probe_HandleRequests::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        _TransientEntity.Clear<FTag_Probe_Updated>();

        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();
    }

    auto
        FProcessor_Probe_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent,
            FFragment_Probe_Requests& InRequestsComp) const
            -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](
            FFragment_Probe_Requests& InRequests)
            {
                algo::ForEachRequest(InRequests._Requests, ck::Visitor([&](
                    const auto& InRequest)
                    {
                        DoHandleRequest(InHandle, InParams, InCurrent, InRequest);
                    }));
            });
    }

    auto
        FProcessor_Probe_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent,
            const FCk_Request_Probe_ExampleRequest& InRequest)
            -> void
    {
        // Add request handling logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Probe_Teardown::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Probe_Params& InParams,
            FFragment_Probe_Current& InCurrent) const
            -> void
    {
        // Add teardown logic here
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Probe_Replicate::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent,
            const TObjectPtr<UCk_Fragment_Probe_Rep>& InRepComp) const
            -> void
    {
        UCk_Utils_Ecs_Net_UE::TryUpdateReplicatedFragment<UCk_Fragment_Probe_Rep>(InHandle, [&](
            UCk_Fragment_Probe_Rep* InLocalRepComp)
            {
                // Add replication logic here
            });
    }
}

// --------------------------------------------------------------------------------------------------------------------
