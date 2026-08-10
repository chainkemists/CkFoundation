#include "CkProbeTrace_Processor.h"

#include "CkProbe_Utils.h"
#include "CkProbeTrace_Utils.h"

#include "CkEcsExt/Transform/CkTransform_Utils.h"

#include "CkSpatialQuery/CkSpatialQuery_Log.h"
#include "CkSpatialQuery/Probe/CkProbeTrace_Context.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_PROCESSOR(ck::FProcessor_ProbeTrace_RayCast);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProbeTrace_ShapeCast);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProbeTrace_DebugDraw_RayCast);
CK_REGISTER_PROCESSOR(ck::FProcessor_ProbeTrace_DebugDraw_ShapeCast);

namespace ck_probe_trace_processor
{
    /// World contacts get their own begin-only bookkeeping because FCk_Probe_OverlapInfo is probe-typed
    /// and its loop fires reciprocal requests INTO the thing it hit — there is no probe on the other end
    /// of a wall. Fires once per contact episode; contacts absent this tick simply drop out (no end
    /// signal by design).
    template <typename T_Result>
    auto
        DoFire_NewWorldContacts(
            FCk_Handle_ProbeTrace InHandle,
            const TArray<T_Result>& InResults)
        -> void
    {
        auto& Contacts = InHandle.AddOrGet<ck::FFragment_ProbeTrace_WorldContacts>();

        auto CurrentEntities = TSet<FCk_Handle>{};
        auto HasAnonymousContact = false;

        for (const auto& Result : InResults)
        {
            if (Result.Get_HitKind() != ECk_ProbeTrace_HitKind::World)
            { continue; }

            const auto& WorldEntity = Result.Get_HitEntity();

            const auto IsNewContact = [&]() -> bool
            {
                if (ck::Is_NOT_Valid(WorldEntity))
                {
                    // Bodies with no owning entity are indistinguishable, so they share one contact slot.
                    const auto IsNew = NOT HasAnonymousContact && NOT Contacts._AnonymousContact;
                    HasAnonymousContact = true;
                    return IsNew;
                }

                // A Multi trace can report two sub-shapes of the same body — CurrentEntities catches
                // the within-tick repeat, Contacts._Entities the across-tick one.
                const auto IsNew = NOT CurrentEntities.Contains(WorldEntity) &&
                                   NOT Contacts._Entities.Contains(WorldEntity);
                CurrentEntities.Add(WorldEntity);
                return IsNew;
            }();

            if (NOT IsNewContact)
            { continue; }

            ck::UUtils_Signal_OnProbeTraceWorldHit::Broadcast(InHandle, ck::MakePayload(InHandle,
                FCk_ProbeTrace_Payload_OnWorldHit{WorldEntity, Result.Get_HitLocation(), Result.Get_SurfaceNormal()}));
        }

        Contacts._Entities = MoveTemp(CurrentEntities);
        Contacts._AnonymousContact = HasAnonymousContact;
    }
}

namespace ck
{
    auto
        FProcessor_ProbeTrace_RayCast::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_RayCast& InRequest) const
        -> void
    {
        if (InHandle.Has<FTag_ProbeTrace_Disabled>())
        { return; }

        const auto TraceContext = FCk_ProbeTrace_Context::Get_ForEntity(InHandle);
        const auto TraceContextIsValid = ck::IsValid(TraceContext);

        CK_ENSURE_IF_NOT(TraceContextIsValid,
            TEXT("ProbeTrace context is NOT valid. Unable to start trace using Handle [{}]"), InHandle)
        {}

        if (NOT TraceContextIsValid)
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
            .Set_BackFaceModeTriangles(InRequest.Get_BackFaceModeTriangles())
            .Set_WorldHitPolicy(InRequest.Get_WorldHitPolicy())
            .Set_WorldFilter(InRequest.Get_WorldFilter())
            .Set_IgnoredEntities(InRequest.Get_IgnoredEntities());

        constexpr auto FireOverlaps = false;
        constexpr auto TryDebugDraw = false;

        const auto& Overlaps =
            InRequest.Get_TracePolicy() == ECk_ProbeTrace_Policy::Multi ?
            UCk_Utils_ProbeTrace_UE::Request_MultiLineTrace(InHandle, RayCastSettings, FireOverlaps, TryDebugDraw, TraceContext) :
            [&]
            {
                auto Result = UCk_Utils_ProbeTrace_UE::Request_SingleLineTrace(InHandle, RayCastSettings, FireOverlaps,
                    TryDebugDraw, TraceContext);

                if (ck::Is_NOT_Valid(Result))
                { return TArray<FCk_Probe_RayCast_Result>{}; }

                return TArray{{*Result}};
            }();

        if (InRequest.Get_WorldHitPolicy() != ECk_ProbeTrace_WorldHitPolicy::Ignore)
        { ck_probe_trace_processor::DoFire_NewWorldContacts(InHandle, Overlaps); }

        for (const auto& Overlap : Overlaps)
        {
            // The overlap bookkeeping below is probe-to-probe; world hits are handled above.
            if (Overlap.Get_HitKind() != ECk_ProbeTrace_HitKind::Probe)
            { continue; }

            auto OtherProbe = Overlap.Get_Probe();

            if (ExistingOverlaps.Contains(FCk_Probe_OverlapInfo{Overlap.Get_Probe()}))
            {
                UCk_Utils_Probe_UE::Request_OverlapUpdated(OtherProbe, FCk_Request_Probe_OverlapUpdated
                {
                    InHandle,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial().Get()
                }, {});

                auto Payload = FCk_Probe_Payload_OnOverlapUpdated
                {
                    OtherProbe,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial().Get()
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
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial().Get()
                }, {});

                auto Payload = FCk_Probe_Payload_OnBeginOverlap
                {
                    OtherProbe,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial().Get()
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
                UCk_Utils_Probe_UE::Request_EndOverlap(OtherProbe, FCk_Request_Probe_EndOverlap{InHandle}, {});
                UUtils_Signal_OnProbeTraceEndOverlap::Broadcast(InHandle, MakePayload(InHandle,
                    FCk_Probe_Payload_OnEndOverlap{OtherProbe}));
            }
        }

        ExistingOverlaps = NewOverlaps;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProbeTrace_ShapeCast::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_ShapeCast& InRequest) const
        -> void
    {
        if (InHandle.Has<FTag_ProbeTrace_Disabled>())
        { return; }

        const auto TraceContext = FCk_ProbeTrace_Context::Get_ForEntity(InHandle);
        const auto TraceContextIsValid = ck::IsValid(TraceContext);

        CK_ENSURE_IF_NOT(TraceContextIsValid,
            TEXT("ProbeTrace context is NOT valid. Unable to start shape trace using Handle [{}]"), InHandle)
        {}

        if (NOT TraceContextIsValid)
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
            .Set_BackFaceModeTriangles(InRequest.Get_BackFaceModeTriangles())
            .Set_WorldHitPolicy(InRequest.Get_WorldHitPolicy())
            .Set_WorldFilter(InRequest.Get_WorldFilter())
            .Set_IgnoredEntities(InRequest.Get_IgnoredEntities());

        constexpr auto FireOverlaps = false;
        constexpr auto TryDebugDraw = false;

        const auto& Overlaps =
            InRequest.Get_TracePolicy() == ECk_ProbeTrace_Policy::Multi ?
            UCk_Utils_ProbeTrace_UE::Request_MultiShapeTrace(InHandle, ShapeCastSettings, FireOverlaps, TryDebugDraw, TraceContext) :
            [&]
            {
                auto Result = UCk_Utils_ProbeTrace_UE::Request_SingleShapeTrace(InHandle, ShapeCastSettings, FireOverlaps,
                    TryDebugDraw, TraceContext);

                if (ck::Is_NOT_Valid(Result))
                { return TArray<FCk_ShapeCast_Result>{}; }

                return TArray{{*Result}};
            }();

        if (InRequest.Get_WorldHitPolicy() != ECk_ProbeTrace_WorldHitPolicy::Ignore)
        { ck_probe_trace_processor::DoFire_NewWorldContacts(InHandle, Overlaps); }

        for (const auto& Overlap : Overlaps)
        {
            // The overlap bookkeeping below is probe-to-probe; world hits are handled above.
            if (Overlap.Get_HitKind() != ECk_ProbeTrace_HitKind::Probe)
            { continue; }

            auto OtherProbe = Overlap.Get_Probe();

            if (ExistingOverlaps.Contains(FCk_Probe_OverlapInfo{Overlap.Get_Probe()}))
            {
                UCk_Utils_Probe_UE::Request_OverlapUpdated(OtherProbe, FCk_Request_Probe_OverlapUpdated
                {
                    InHandle,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial().Get()
                }, {});

                auto Payload = FCk_Probe_Payload_OnOverlapUpdated
                {
                    OtherProbe,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial().Get()
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
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial().Get()
                }, {});

                auto Payload = FCk_Probe_Payload_OnBeginOverlap
                {
                    OtherProbe,
                    TArray{{Overlap.Get_HitLocation()}},
                    Overlap.Get_NormalDirLen().GetSafeNormal(),
                    UCk_Utils_Probe_UE::Get_SurfaceInfo(OtherProbe).Get_PhysicalMaterial().Get()
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
                UCk_Utils_Probe_UE::Request_EndOverlap(OtherProbe, FCk_Request_Probe_EndOverlap{InHandle}, {});
                UUtils_Signal_OnProbeTraceEndOverlap::Broadcast(InHandle, MakePayload(InHandle,
                    FCk_Probe_Payload_OnEndOverlap{OtherProbe}));
            }
        }

        ExistingOverlaps = NewOverlaps;
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProbeTrace_DebugDraw_RayCast::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_RayCast& InRequest) const
        -> void
    {
        const auto TraceContext = FCk_ProbeTrace_Context::Get_ForEntity(InHandle);

        if (ck::Is_NOT_Valid(TraceContext) || ck::Is_NOT_Valid(InRequest.Get_StartPos()))
        { return; }

        const auto& Transform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InRequest.Get_StartPos());
        const auto& EndPos = Transform.TransformPosition(InRequest.Get_DirectionAndLength());

        const auto RayCastSettings = FCk_Probe_RayCast_Settings{Transform.GetLocation(), EndPos, InRequest.Get_Filter()}
            .Set_BackFaceModeConvex(InRequest.Get_BackFaceModeConvex())
            .Set_BackFaceModeTriangles(InRequest.Get_BackFaceModeTriangles())
            .Set_WorldHitPolicy(InRequest.Get_WorldHitPolicy())
            .Set_WorldFilter(InRequest.Get_WorldFilter())
            .Set_IgnoredEntities(InRequest.Get_IgnoredEntities());

        if (InHandle.Has<FTag_ProbeTrace_Disabled>())
        {
            UCk_Utils_ProbeTrace_UE::Request_DrawLineTrace(InHandle, RayCastSettings, {}, /*InIsDisabled=*/true);
            return;
        }

        constexpr auto FireOverlaps = false;
        constexpr auto TryDebugDraw = true;

        if (InRequest.Get_TracePolicy() == ECk_ProbeTrace_Policy::Multi)
        {
            UCk_Utils_ProbeTrace_UE::Request_MultiLineTrace(InHandle, RayCastSettings, FireOverlaps, TryDebugDraw, TraceContext);
        }
        else
        {
            UCk_Utils_ProbeTrace_UE::Request_SingleLineTrace(InHandle, RayCastSettings, FireOverlaps, TryDebugDraw, TraceContext);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_ProbeTrace_DebugDraw_ShapeCast::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_ProbeTrace_ShapeCast& InRequest) const
        -> void
    {
        const auto TraceContext = FCk_ProbeTrace_Context::Get_ForEntity(InHandle);

        if (ck::Is_NOT_Valid(TraceContext) || ck::Is_NOT_Valid(InRequest.Get_StartPos()))
        { return; }

        const auto& Transform = UCk_Utils_Transform_UE::Get_EntityCurrentTransform(InRequest.Get_StartPos());
        const auto& EndPos = Transform.TransformPosition(InRequest.Get_DirectionAndLength());

        const auto ShapeCastSettings = FCk_ShapeCast_Settings{Transform.GetLocation(), EndPos, InRequest.Get_Shape(), InRequest.Get_Filter()}
            .Set_BackFaceModeConvex(InRequest.Get_BackFaceModeConvex())
            .Set_BackFaceModeTriangles(InRequest.Get_BackFaceModeTriangles())
            .Set_WorldHitPolicy(InRequest.Get_WorldHitPolicy())
            .Set_WorldFilter(InRequest.Get_WorldFilter())
            .Set_IgnoredEntities(InRequest.Get_IgnoredEntities());

        if (InHandle.Has<FTag_ProbeTrace_Disabled>())
        {
            UCk_Utils_ProbeTrace_UE::Request_DrawShapeTrace(InHandle, ShapeCastSettings, {}, /*InIsDisabled=*/true);
            return;
        }

        constexpr auto FireOverlaps = false;
        constexpr auto TryDebugDraw = true;

        if (InRequest.Get_TracePolicy() == ECk_ProbeTrace_Policy::Multi)
        {
            UCk_Utils_ProbeTrace_UE::Request_MultiShapeTrace(InHandle, ShapeCastSettings, FireOverlaps, TryDebugDraw, TraceContext);
        }
        else
        {
            UCk_Utils_ProbeTrace_UE::Request_SingleShapeTrace(InHandle, ShapeCastSettings, FireOverlaps, TryDebugDraw, TraceContext);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
