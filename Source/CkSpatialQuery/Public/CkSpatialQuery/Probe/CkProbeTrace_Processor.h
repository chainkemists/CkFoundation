#pragma once

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable once CppInconsistentNaming
namespace JPH { class PhysicsSystem; }

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKSPATIALQUERY_API FProcessor_ProbeTrace_RayCast : public ck_exp::TProcessor<
        FProcessor_ProbeTrace_RayCast,
        FCk_Handle_ProbeTrace,
        ck::TReadOnly<FFragment_ProbeTrace_RayCast>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Overlap;
        using MarkedDirtyBy = FFragment_Probe_Requests;

        FProcessor_ProbeTrace_RayCast(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_RayCast& InRequest) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_ProbeTrace_ShapeCast : public ck_exp::TProcessor<
        FProcessor_ProbeTrace_ShapeCast,
        FCk_Handle_ProbeTrace,
        ck::TReadOnly<FFragment_ProbeTrace_ShapeCast>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Overlap;
        using MarkedDirtyBy = FFragment_Probe_Requests;

        FProcessor_ProbeTrace_ShapeCast(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_ShapeCast& InRequest) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_ProbeTrace_DebugDraw_RayCast : public ck_exp::TProcessor<
        FProcessor_ProbeTrace_DebugDraw_RayCast,
        FCk_Handle_ProbeTrace,
        FTag_ProbeTrace_DebugDraw,
        ck::TReadOnly<FFragment_ProbeTrace_RayCast>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Overlap;

        FProcessor_ProbeTrace_DebugDraw_RayCast(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_RayCast& InRequest) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_ProbeTrace_DebugDraw_ShapeCast : public ck_exp::TProcessor<
        FProcessor_ProbeTrace_DebugDraw_ShapeCast,
        FCk_Handle_ProbeTrace,
        FTag_ProbeTrace_DebugDraw,
        ck::TReadOnly<FFragment_ProbeTrace_ShapeCast>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Overlap;

        FProcessor_ProbeTrace_DebugDraw_ShapeCast(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem);

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_ShapeCast& InRequest) const -> void;

    private:
        TWeakPtr<JPH::PhysicsSystem> _PhysicsSystem;
    };
}

// --------------------------------------------------------------------------------------------------------------------
