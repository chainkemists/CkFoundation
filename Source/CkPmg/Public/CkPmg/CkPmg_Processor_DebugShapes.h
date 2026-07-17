#pragma once

#include "CkPmg_Fragment.h"
#include "CkPmg_ProcessorGroups.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment.h"
#include "CkEcs/Processor/CkProcessor.h"
#include "CkEcs/Scheduler/CkProcessorGroups.h"

#include "CkEcsExt/Transform/CkTransform_Fragment.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class CKPMG_API FProcessor_Pmg_DebugShape_UpdateTransform : public ck_exp::TProcessor<
            FProcessor_Pmg_DebugShape_UpdateTransform,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Current>,
            ck::TReadOnly<FFragment_Transform>,
            FTag_Transform_Updated,
            TExclude<FTag_Pmg_DebugShape_NeedsSetup>,
            TExclude<FTag_Pmg_DebugShape_Composite>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FGroup_Pmg_DebugShape_Setup>;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FFragment_Transform& InTransform)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // One-shot wireframe bake. Triangulates each cached local-space line
    // segment (populated by Setup processors via ck::pmg::Append_DebugLine_World)
    // as a flat rectangle and appends it as a second mesh section on the
    // entity's procmesh. The wireframe section shares the filled mesh's
    // UMaterialInstanceDynamic, so Request_SetColor mutates both at once.
    //
    // Gated by FTag_Pmg_DebugShape_LinesNeedBaking — stamped by every
    // Append_Debug*_World call, removed after bake. The processor never
    // iterates an entity again until something appends new lines (e.g. a
    // re-setup) or SetDrawLines toggles wireframes back on.
    //
    // Replaces the prior per-tick FProcessor_Pmg_DebugShape_DrawLines, which
    // re-emitted every line every frame via UE's TransientLineBatchComponent
    // and tanked perf in scenes with many wireframe shapes.
    class CKPMG_API FProcessor_Pmg_DebugShape_BakeLines : public ck_exp::TProcessor<
            FProcessor_Pmg_DebugShape_BakeLines,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Common>,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Lines>,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Current>,
            FTag_Pmg_DebugShape_LinesNeedBaking,
            TExclude<FTag_Pmg_DebugShape_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using MarkedDirtyBy = FTag_Pmg_DebugShape_LinesNeedBaking;
        using RunAfter = TDepList<FProcessor_Pmg_DebugShape_UpdateTransform>;
        using TProcessor::TProcessor;

    public:
        // Note: Current is TReadOnly because this processor only invokes
        // non-const methods on the underlying UProceduralMeshComponent
        // (CreateMeshSection / SetMaterial / SetMeshSectionVisible) — those
        // mutate the mesh-component object but don't write to the fragment
        // itself. Modeling this as TReadOnly avoids the framework's write-
        // conflict warning against HandleRequests / CheckDuration, which DO
        // write the fragment.
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            const FFragment_Pmg_DebugShape_Lines& InLines,
            const FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_DebugShape_CheckDuration : public ck_exp::TProcessor<
            FProcessor_Pmg_DebugShape_CheckDuration,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadOnly<FFragment_Pmg_DebugShape_Common>,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Current>,
            TExclude<FTag_Pmg_DebugShape_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        // UpdateTransform/CheckDuration/HandleRequests all write FFragment_Pmg_DebugShape_Current;
        // chain them in declaration order to make the deterministic write-ordering explicit.
        using RunAfter = TDepList<FGroup_Pmg_DebugShape_Setup, FProcessor_Pmg_DebugShape_UpdateTransform>;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------


    // --------------------------------------------------------------------------------------------------------------------

    class CKPMG_API FProcessor_Pmg_DebugShape_EndPlay : public ck_exp::TProcessor<
            FProcessor_Pmg_DebugShape_EndPlay,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Current>,
            CK_IF_END_PLAY>
    {
    public:
        using Group = FGroup_EndPlay;
        using TProcessor::TProcessor;

    public:
        static auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void;
    };

    // --------------------------------------------------------------------------------------------------------------------

    // Drains FFragment_Pmg_DebugShape_Requests and applies each request to the
    // shape's Common cache + live procmesh side effects (MID color parameter,
    // visibility from RenderMode, collision toggle). Runs after Setup so the
    // procmesh exists before we try to mutate it. Removes the requests fragment
    // when drained (CopyAndRemove), so this processor only spends cycles on
    // shapes that actually have pending mutations.
    class CKPMG_API FProcessor_Pmg_DebugShape_HandleRequests : public ck_exp::TProcessor<
            FProcessor_Pmg_DebugShape_HandleRequests,
            FCk_Handle_Pmg_DebugShape,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Common>,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Current>,
            ck::TReadWrite<FFragment_Pmg_DebugShape_Requests>,
            TExclude<FTag_Pmg_DebugShape_NeedsSetup>,
            CK_IGNORE_PENDING_KILL>
    {
    public:
        using Group = FGroup_Gameplay_Rendering;
        using RunAfter = TDepList<FGroup_Pmg_DebugShape_Setup, FProcessor_Pmg_DebugShape_CheckDuration>;
        using MarkedDirtyBy = FFragment_Pmg_DebugShape_Requests;

    public:
        using TProcessor::TProcessor;

    public:
        auto
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            FFragment_Pmg_DebugShape_Requests& InRequestsComp) const
            -> void;

    private:
        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetColor& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetLineThickness& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetDrawLines& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetDuration& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetRenderMode& InRequest)
            -> void;

        static auto
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetEnableCollision& InRequest)
            -> void;
    };
}
