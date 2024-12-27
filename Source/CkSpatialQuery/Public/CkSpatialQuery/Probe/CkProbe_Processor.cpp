#include "CkProbe_Processor.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkNet/CkNet_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"

#include "edyn/edyn.hpp"

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
        const RegistryType& InGameRegistry,
        const RegistryType& InPhysicsRegistry)
        : TProcessor(InGameRegistry)
        , _PhysicsRegistry(InPhysicsRegistry)
    {
        _PhysicsRegistry.Request_PerformOperationOnInternalRegistry([&](
            entt::registry& InRegistry)
            {
                edyn::detach(InRegistry);
                edyn::attach(InRegistry);
                InRegistry.clear();
            });
    }

    void
        print_entities(
            entt::registry& registry)
    {
        auto view = registry.view<const edyn::position, const edyn::linvel, const edyn::orientation>();
        view.each([](
            auto ent,
            const auto& pos,
            const auto& vel,
            const auto& ori)
            {
                ck::spatialquery::Warning(TEXT("pos ({}): {}, {}, {}\n"), entt::to_integral(ent), pos.x, pos.y, pos.z);
                ck::spatialquery::Warning(TEXT("vel ({}): {}, {}, {}\n"), entt::to_integral(ent), vel.x, vel.y, vel.z);
                ck::spatialquery::Warning(TEXT("ori ({}): {}, {}, {}\n"), entt::to_integral(ent), ori.x, ori.y, ori.z);
            });
    }

    auto
        FProcessor_Probe_Setup::
        DoTick(
            TimeType InDeltaT)
            -> void
    {
        TProcessor::DoTick(InDeltaT);

        _TransientEntity.Clear<MarkedDirtyBy>();

        _PhysicsRegistry.Request_PerformOperationOnInternalRegistry([&](
            entt::registry& InRegistry)
            {
                //InRegistry.clear<edyn::contact_manifold_events>();
                edyn::update(InRegistry, InDeltaT.Get_Seconds());
                print_entities(InRegistry);
            });
    }

    void
        OnCreateIsland(
            entt::registry& registry,
            entt::entity entity)
    {
        ck::spatialquery::Warning(TEXT("We have a new island!"));
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
        ProbeDef.kind = SetBodyToKinematic ? edyn::rigidbody_kind::rb_kinematic : edyn::rigidbody_kind::rb_dynamic;
        SetBodyToKinematic = !SetBodyToKinematic;
        //if (ProbeDef.kind == edyn::rigidbody_kind::rb_kinematic)
        //{
        //    ProbeDef.material = {};
        //}
        //else
        //{
            ProbeDef.presentation = true;
            ProbeDef.mass = 100;
            ProbeDef.material->friction = 0.8;
            ProbeDef.material->restitution = 0;
        //}
        ProbeDef.shape = edyn::box_shape{HalfExtents.X, HalfExtents.Y, HalfExtents.Z};
        ProbeDef.position = edyn::vector3{EntityPosition.X, EntityPosition.Y, EntityPosition.Z};

        _PhysicsRegistry.Request_PerformOperationOnInternalRegistry([&](
            entt::registry& InRegistry)
            {
                auto RbEntity = edyn::make_rigidbody(InRegistry,
                    ProbeDef);

                InCurrent._Entity = RbEntity;

                edyn::on_contact_started(InRegistry).connect<&FProcessor_Probe_Setup::SensorContactStarted>(InRegistry);
                InRegistry.on_construct<edyn::island_tag>().connect<&OnCreateIsland>();

                //edyn::rigidbody_apply_impulse(InRegistry, InCurrent._Entity, edyn::vector3{0, 100, 0},
                //    edyn::vector3_zero);
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
        const auto Contact = registry.get<edyn::contact_manifold>(entity);
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_Probe_UpdateTransform::
    FProcessor_Probe_UpdateTransform(
        const RegistryType& InGameRegistry,
        const RegistryType& InPhysicsRegistry)
        : TProcessor(InGameRegistry)
        , _PhysicsRegistry(InPhysicsRegistry)
    {
    }

    auto
        FProcessor_Probe_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Probe_Current& InCurrent)
            -> void
    {
        auto EntityTransform = UCk_Utils_Transform_TypeUnsafe_UE::Get_EntityCurrentLocation(InHandle);

        _PhysicsRegistry.Request_PerformOperationOnInternalRegistry([&](
            entt::registry& InRegistry)
            {
                //edyn::rigidbody_apply_impulse(InRegistry, InCurrent._Entity, edyn::vector3{0, 1000000, 0},
                //    edyn::vector3_zero);

                const auto Position = InRegistry.get<edyn::position>(InCurrent._Entity);
                const auto Orientation = InRegistry.get<edyn::orientation>(InCurrent._Entity);

                UCk_Utils_Transform_TypeUnsafe_UE::Request_SetLocation(InHandle,
                    FCk_Request_Transform_SetLocation{FVector{Position.x, Position.y, Position.z}});
                UCk_Utils_Transform_TypeUnsafe_UE::Request_SetRotation(InHandle,
                    FCk_Request_Transform_SetRotation{FRotator{FQuat{Orientation.x, Orientation.y, Orientation.z, Orientation.w}}});

                //edyn::update_kinematic_position(InRegistry, InCurrent._Entity,
                //    edyn::vector3{EntityTransform.X, EntityTransform.Y, EntityTransform.Z}, InDeltaT.Get_Seconds());

                //edyn::wake_up_entity(InRegistry, InCurrent._Entity);
            });
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
