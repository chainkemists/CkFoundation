#include "CkProbeTrace_Processor.h"

#include "CkProbe_Utils.h"
#include "CkProbeTrace_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/Subsystem/CkSpatialQuery_Subsystem.h"

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

#define CK_PROBE_FACTORY(ProcessorType) \
    CK_REGISTER_PROCESSOR_WITH_FACTORY(ProcessorType, \
        [](const FCk_Registry& InRegistry) -> ck::concepts::FTickableType \
        { \
            const auto& PhysicsSystem = InRegistry.GetContext<TWeakPtr<JPH::PhysicsSystem>>(); \
            return ProcessorType{InRegistry, PhysicsSystem}; \
        })

CK_PROBE_FACTORY(ck::FProcessor_ProbeTrace_RayCast);
CK_PROBE_FACTORY(ck::FProcessor_ProbeTrace_ShapeCast);
CK_PROBE_FACTORY(ck::FProcessor_ProbeTrace_DebugDraw_RayCast);
CK_PROBE_FACTORY(ck::FProcessor_ProbeTrace_DebugDraw_ShapeCast);

#undef CK_PROBE_FACTORY

namespace ck
{
    FProcessor_ProbeTrace_RayCast::
        FProcessor_ProbeTrace_RayCast(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem)
    {
    }

    auto
        FProcessor_ProbeTrace_RayCast::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_RayCast& InRequest) const
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(_PhysicsSystem),
            TEXT("PhysicsSystem is NOT valid. Unable to start trace using Handle [{}]"), InHandle)
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            return;
        }

        if (ck::Is_NOT_Valid(InRequest.Get_StartPos()))
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            return;
        }

        auto& ExistingOverlaps = InHandle.AddOrGet<TSet<FCk_Probe_OverlapInfo>>();
        auto NewOverlaps = TSet<FCk_Probe_OverlapInfo>{};

        const auto& Transform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InRequest.Get_StartPos());
        const auto& EndPos = Transform.TransformPosition(InRequest.Get_DirectionAndLength());

        const auto RayCastSettings = FCk_Probe_RayCast_Settings{Transform.GetLocation(), EndPos, InRequest.Get_Filter()}
            .Set_BackFaceModeConvex(InRequest.Get_BackFaceModeConvex())
            .Set_BackFaceModeTriangles(InRequest.Get_BackFaceModeTriangles());

        constexpr auto FireOverlaps = false;
        constexpr auto TryDebugDraw = false;

        const auto& Overlaps =
            InRequest.Get_TracePolicy() == ECk_ProbeTrace_Policy::Multi ?
            UCk_Utils_ProbeTrace_UE::Request_MultiLineTrace(InHandle, RayCastSettings, FireOverlaps, TryDebugDraw, *_PhysicsSystem.Pin()) :
            [&]
            {
                auto Result = UCk_Utils_ProbeTrace_UE::Request_SingleLineTrace(InHandle, RayCastSettings, FireOverlaps,
                    TryDebugDraw, *_PhysicsSystem.Pin());

                if (ck::Is_NOT_Valid(Result))
                { return TArray<FCk_Probe_RayCast_Result>{}; }

                return TArray{{*Result}};
            }();

        for (const auto& Overlap : Overlaps)
        {
            auto OtherProbe = Overlap.Get_Probe();

            if (ExistingOverlaps.Contains(FCk_Probe_OverlapInfo{Overlap.Get_Probe()}))
            {
                UCk_Utils_Probe_UE::Request_OverlapUpdated(OtherProbe, FCk_Request_Probe_OverlapUpdated
                {
                    InHandle,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial()
                });

                auto Payload = FCk_Probe_Payload_OnOverlapUpdated
                {
                    OtherProbe,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial()
                };

                UUtils_Signal_OnProbeTraceOverlapUpdated::Broadcast(InHandle, MakePayload(InHandle, Payload));
            }
            else
            {
                UCk_Utils_Probe_UE::Request_BeginOverlap(OtherProbe, FCk_Request_Probe_BeginOverlap
                {
                    InHandle,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial()
                });

                auto Payload = FCk_Probe_Payload_OnBeginOverlap
                {
                    OtherProbe,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial()
                };

                UUtils_Signal_OnProbeTraceBeginOverlap::Broadcast(InHandle, MakePayload(InHandle, Payload));
            }

            NewOverlaps.Add(FCk_Probe_OverlapInfo{OtherProbe});
        }

        for (const auto& ExistingOverlap : ExistingOverlaps)
        {
            if (NewOverlaps.Contains(ExistingOverlap))
            { continue; }

            if (auto OtherProbe = UCk_Utils_Probe_UE::Cast(ExistingOverlap.Get_OtherEntity());
                ck::IsValid(OtherProbe))
            {
                UCk_Utils_Probe_UE::Request_EndOverlap(OtherProbe, FCk_Request_Probe_EndOverlap{InHandle});
                UUtils_Signal_OnProbeTraceEndOverlap::Broadcast(InHandle, MakePayload(InHandle,
                    FCk_Probe_Payload_OnEndOverlap{OtherProbe}));
            }
        }

        ExistingOverlaps = NewOverlaps;
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_ProbeTrace_ShapeCast::
        FProcessor_ProbeTrace_ShapeCast(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem)
    {
    }

    auto
        FProcessor_ProbeTrace_ShapeCast::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_ShapeCast& InRequest) const
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(_PhysicsSystem),
            TEXT("PhysicsSystem is NOT valid. Unable to start shape trace using Handle [{}]"), InHandle)
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            return;
        }

        if (ck::Is_NOT_Valid(InRequest.Get_StartPos()))
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
            return;
        }

        auto& ExistingOverlaps = InHandle.AddOrGet<TSet<FCk_Probe_OverlapInfo>>();
        auto NewOverlaps = TSet<FCk_Probe_OverlapInfo>{};

        const auto& Transform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InRequest.Get_StartPos());
        const auto& EndPos = Transform.TransformPosition(InRequest.Get_DirectionAndLength());

        const auto ShapeCastSettings = FCk_ShapeCast_Settings{Transform.GetLocation(), EndPos, InRequest.Get_Shape(), InRequest.Get_Filter()}
            .Set_BackFaceModeConvex(InRequest.Get_BackFaceModeConvex())
            .Set_BackFaceModeTriangles(InRequest.Get_BackFaceModeTriangles());

        constexpr auto FireOverlaps = false;
        constexpr auto TryDebugDraw = false;

        const auto& Overlaps =
            InRequest.Get_TracePolicy() == ECk_ProbeTrace_Policy::Multi ?
            UCk_Utils_ProbeTrace_UE::Request_MultiShapeTrace(InHandle, ShapeCastSettings, FireOverlaps, TryDebugDraw, *_PhysicsSystem.Pin()) :
            [&]
            {
                auto Result = UCk_Utils_ProbeTrace_UE::Request_SingleShapeTrace(InHandle, ShapeCastSettings, FireOverlaps,
                    TryDebugDraw, *_PhysicsSystem.Pin());

                if (ck::Is_NOT_Valid(Result))
                { return TArray<FCk_ShapeCast_Result>{}; }

                return TArray{{*Result}};
            }();

        for (const auto& Overlap : Overlaps)
        {
            auto OtherProbe = Overlap.Get_Probe();

            if (ExistingOverlaps.Contains(FCk_Probe_OverlapInfo{Overlap.Get_Probe()}))
            {
                UCk_Utils_Probe_UE::Request_OverlapUpdated(OtherProbe, FCk_Request_Probe_OverlapUpdated
                {
                    InHandle,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial()
                });

                auto Payload = FCk_Probe_Payload_OnOverlapUpdated
                {
                    OtherProbe,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial()
                };

                UUtils_Signal_OnProbeTraceOverlapUpdated::Broadcast(InHandle, MakePayload(InHandle, Payload));
            }
            else
            {
                UCk_Utils_Probe_UE::Request_BeginOverlap(OtherProbe, FCk_Request_Probe_BeginOverlap
                {
                    InHandle,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial()
                });

                auto Payload = FCk_Probe_Payload_OnBeginOverlap
                {
                    OtherProbe,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial()
                };

                UUtils_Signal_OnProbeTraceBeginOverlap::Broadcast(InHandle, MakePayload(InHandle, Payload));
            }

            NewOverlaps.Add(FCk_Probe_OverlapInfo{OtherProbe});
        }

        for (const auto& ExistingOverlap : ExistingOverlaps)
        {
            if (NewOverlaps.Contains(ExistingOverlap))
            { continue; }

            if (auto OtherProbe = UCk_Utils_Probe_UE::Cast(ExistingOverlap.Get_OtherEntity());
                ck::IsValid(OtherProbe))
            {
                UCk_Utils_Probe_UE::Request_EndOverlap(OtherProbe, FCk_Request_Probe_EndOverlap{InHandle});
                UUtils_Signal_OnProbeTraceEndOverlap::Broadcast(InHandle, MakePayload(InHandle,
                    FCk_Probe_Payload_OnEndOverlap{OtherProbe}));
            }
        }

        ExistingOverlaps = NewOverlaps;
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_ProbeTrace_DebugDraw_RayCast::
        FProcessor_ProbeTrace_DebugDraw_RayCast(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem)
    {
    }

    auto
        FProcessor_ProbeTrace_DebugDraw_RayCast::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_RayCast& InRequest) const
        -> void
    {
        if (ck::Is_NOT_Valid(_PhysicsSystem) || ck::Is_NOT_Valid(InRequest.Get_StartPos()))
        { return; }

        const auto& Transform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InRequest.Get_StartPos());
        const auto& EndPos = Transform.TransformPosition(InRequest.Get_DirectionAndLength());

        const auto RayCastSettings = FCk_Probe_RayCast_Settings{Transform.GetLocation(), EndPos, InRequest.Get_Filter()}
            .Set_BackFaceModeConvex(InRequest.Get_BackFaceModeConvex())
            .Set_BackFaceModeTriangles(InRequest.Get_BackFaceModeTriangles());

        constexpr auto FireOverlaps = false;
        constexpr auto TryDebugDraw = true;

        if (InRequest.Get_TracePolicy() == ECk_ProbeTrace_Policy::Multi)
        {
            UCk_Utils_ProbeTrace_UE::Request_MultiLineTrace(InHandle, RayCastSettings, FireOverlaps, TryDebugDraw, *_PhysicsSystem.Pin());
        }
        else
        {
            UCk_Utils_ProbeTrace_UE::Request_SingleLineTrace(InHandle, RayCastSettings, FireOverlaps, TryDebugDraw, *_PhysicsSystem.Pin());
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    FProcessor_ProbeTrace_DebugDraw_ShapeCast::
        FProcessor_ProbeTrace_DebugDraw_ShapeCast(
            const RegistryType& InRegistry,
            const TWeakPtr<JPH::PhysicsSystem>& InPhysicsSystem)
        : TProcessor(InRegistry)
        , _PhysicsSystem(InPhysicsSystem)
    {
    }

    auto
        FProcessor_ProbeTrace_DebugDraw_ShapeCast::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_ShapeCast& InRequest) const
        -> void
    {
        if (ck::Is_NOT_Valid(_PhysicsSystem) || ck::Is_NOT_Valid(InRequest.Get_StartPos()))
        { return; }

        const auto& Transform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InRequest.Get_StartPos());
        const auto& EndPos = Transform.TransformPosition(InRequest.Get_DirectionAndLength());

        const auto ShapeCastSettings = FCk_ShapeCast_Settings{Transform.GetLocation(), EndPos, InRequest.Get_Shape(), InRequest.Get_Filter()}
            .Set_BackFaceModeConvex(InRequest.Get_BackFaceModeConvex())
            .Set_BackFaceModeTriangles(InRequest.Get_BackFaceModeTriangles());

        constexpr auto FireOverlaps = false;
        constexpr auto TryDebugDraw = true;

        if (InRequest.Get_TracePolicy() == ECk_ProbeTrace_Policy::Multi)
        {
            UCk_Utils_ProbeTrace_UE::Request_MultiShapeTrace(InHandle, ShapeCastSettings, FireOverlaps, TryDebugDraw, *_PhysicsSystem.Pin());
        }
        else
        {
            UCk_Utils_ProbeTrace_UE::Request_SingleShapeTrace(InHandle, ShapeCastSettings, FireOverlaps, TryDebugDraw, *_PhysicsSystem.Pin());
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
