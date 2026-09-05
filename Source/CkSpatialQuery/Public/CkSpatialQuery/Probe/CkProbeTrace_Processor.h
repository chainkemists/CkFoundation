#pragma once

#include "CkSpatialQuery/Probe/CkProbe_Fragment.h"

#include "CkJolt/World/CkJoltWorld_Processor.h"

#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_ProbeTrace_RayCast;

    // --------------------------------------------------------------------------------------------------------------------

    // LatestCompleted reads only the completed prior batch. It deliberately runs before every scheduled
    // Jolt Transform writer and stores values for the Overlap-group reconciliation below.
    class CKSPATIALQUERY_API FProcessor_ProbeTrace_EarlyRayCast : public ck_exp::TProcessor<
        FProcessor_ProbeTrace_EarlyRayCast,
        FCk_Handle_ProbeTrace,
        ck::TReadOnly<FFragment_ProbeTrace_RayCast>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_WaitForAsync>;
        using RunBefore = TDepList<FProcessor_JoltWorld_TransformWriters>;
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

        using TProcessor::TProcessor;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_RayCast& InRequest) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_ProbeTrace_EarlyShapeCast : public ck_exp::TProcessor<
        FProcessor_ProbeTrace_EarlyShapeCast,
        FCk_Handle_ProbeTrace,
        ck::TReadOnly<FFragment_ProbeTrace_ShapeCast>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Transform;
        using RunAfter = TDepList<FProcessor_JoltWorld_WaitForAsync>;
        using RunBefore = TDepList<FProcessor_JoltWorld_TransformWriters>;
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

        using TProcessor::TProcessor;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_ShapeCast& InRequest) const -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Runs once per world before persistent Ray/Shape reconciliation. It blocks only when at least one
    // CurrentSolved trace exists, leaving LatestCompleted traces on the no-wait fast path.
    class CKSPATIALQUERY_API FProcessor_ProbeTrace_EnsureCurrentSolved : public TProcessorBase<FProcessor_ProbeTrace_EnsureCurrentSolved>
    {
    public:
        using Group = FGroup_Overlap;
        using RunAfter = TDepList<FProcessor_Probe_HandleRequests>;
        using RunBefore = TDepList<FProcessor_ProbeTrace_RayCast>;
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

    private:
        using Super = TProcessorBase;
        friend class Super;

    public:
        explicit FProcessor_ProbeTrace_EnsureCurrentSolved(const RegistryType& InRegistry);

    public:
        auto DoTick(TimeType InDeltaT) -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKSPATIALQUERY_API FProcessor_ProbeTrace_RayCast : public ck_exp::TProcessor<
        FProcessor_ProbeTrace_RayCast,
        FCk_Handle_ProbeTrace,
        ck::TReadOnly<FFragment_ProbeTrace_RayCast>,
        CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Overlap;
        using MarkedDirtyBy = FFragment_Probe_Requests;
        // The shared MarkedDirtyBy is only a trigger here (the view is FFragment_ProbeTrace_*), so
        // running after HandleRequests is safe.
        using RunAfter = TDepList<FProcessor_Probe_HandleRequests, FProcessor_ProbeTrace_EnsureCurrentSolved>;
        // SkipPump: sticky view + non-idempotent body — an overlapping trace re-adds the shared dirty
        // marker every pass, so pumping never quiesces and re-casts against Jolt each time.
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

        using TProcessor::TProcessor;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_RayCast& InRequest) const -> void;
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
        using RunAfter = TDepList<FProcessor_Probe_HandleRequests, FProcessor_ProbeTrace_RayCast>;
        // SkipPump for the same reason as RayCast above.
        static constexpr auto PumpPolicy = ECk_ProcessorPumpPolicy::SkipPump;
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

        using TProcessor::TProcessor;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_ShapeCast& InRequest) const -> void;
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
        using RunAfter = TDepList<FProcessor_ProbeTrace_EnsureCurrentSolved>;
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

        using TProcessor::TProcessor;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_RayCast& InRequest) const -> void;
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
        using RunAfter = TDepList<FProcessor_ProbeTrace_EnsureCurrentSolved>;
        static constexpr auto WorldTypeRequirement = ECk_ProcessorWorldTypeRequirement::RuntimeOnly;

        using TProcessor::TProcessor;

        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_ShapeCast& InRequest) const -> void;
    };
}

// --------------------------------------------------------------------------------------------------------------------
