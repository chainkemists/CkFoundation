#include "CkPmg_Processor_DebugShapes.h"

#include "CkCore/Object/CkObject_Utils.h"
#include "CkPmg_Processor_AngularShapes.h"
#include "CkPmg_Processor_BasicShapes.h"
#include "CkPmg_Processor_DirectionalShapes.h"
#include "CkPmg_Processor_FlatShapes.h"
#include "CkPmg_Processor_IconShapes.h"
#include "CkPmg_Processor_SymbolShapes.h"
#include "CkPmg_Processor_TextShapes.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Debug/CkDebugDraw_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkPmg/CkPmg_Log.h"
#include "CkPmg/CkPmg_Stats.h"
#include "CkPmg_Utils_DebugShapes.h"
#include "CkPmg_Utils_IconShapes.h"
#include "CkPmg_Utils_SymbolShapes.h"

#include <MaterialDomain.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <ProceduralMeshComponent.h>

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Pmg::DebugDrawLines"), STAT_Pmg_DebugDrawLines, STATGROUP_CkPmg);
DECLARE_DWORD_COUNTER_STAT(TEXT("Pmg Debug Lines"), STAT_Pmg_DebugLines, STATGROUP_CkPmg);

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_GROUP(ck::FGroup_Pmg_DebugShape_Setup);

CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_UpdateTransform);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_BakeLines);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_CheckDuration);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_EndPlay);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Sphere_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Box_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Cone_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Cylinder_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Capsule_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Pyramid_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Hemisphere_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Torus_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Circle_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Triangle_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Plane_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Ring_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Cross_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Star_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Checkmark_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Diamond_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Warning_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Prohibition_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_NoEntry_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_MagnifyingGlass_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_QuestionMark_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_ExclamationMark_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Flag_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_InfoCircle_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Pin_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Text_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Wedge_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Arc_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_WedgeCone_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Arrow_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Pivot_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DashedLine_Setup);

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_DebugShape_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FFragment_Transform& InTransform)
            -> void
    {
        auto MeshComponent = InCurrent._MeshComponent.Get();
        // Tolerate a torn-down mesh: the EndPlay processor resets _MeshComponent during teardown,
        // and a transform tick can race that window. Nothing to update if the procmesh is gone —
        // skip gracefully (matches FProcessor_Pmg_DebugShape_BakeLines' guard) rather than ensure.
        if (ck::Is_NOT_Valid(MeshComponent))
        { return; }

        MeshComponent->SetWorldTransform(InTransform.Get_Transform());
    }

    // --------------------------------------------------------------------------------------------------------------------

    namespace pmg_bake_lines_helpers
    {
        // Section index of the baked wireframe boxes on the procmesh.
        // Section 0 is the filled shape (see CkPmg_Processor_BasicShapes.cpp
        // FinalizeMeshComponent_Basic). Section 1 hosts the wireframe overlay
        // and gets its own UMID (spawned lazily by BakeLines from the same
        // parent material as slot 0) with alpha forced to 1, so wireframes
        // stay opaque regardless of fill transparency. Request_SetColor
        // mirrors RGB updates to both MIDs.
        constexpr int32 WireframeSectionIndex = 1;

        // Triangulate a single FCk_Pmg_DebugLine as a stretched box (8 verts,
        // 12 triangles). Cross-section is Thickness × Thickness, length is the
        // line length. The box is in entity-local space — the procmesh's
        // SetWorldTransform handles world placement. Stretched-box geometry
        // (vs the earlier flat-quad approach) closes corner gaps where
        // adjacent lines meet, because each box extends Thickness/2 in both
        // perpendicular directions at every endpoint, overlapping the
        // neighbouring box's endpoint at the join.
        auto
            BuildBoxForLine(
                const FCk_Pmg_DebugLine& InLine,
                TArray<FVector>& OutVertices,
                TArray<int32>& OutTriangles,
                TArray<FVector>& OutNormals,
                TArray<FVector2D>& OutUVs)
            -> void
        {
            const auto LineDir = (InLine._End - InLine._Start).GetSafeNormal();
            if (LineDir.IsNearlyZero())
            { return; }

            // Build an orthonormal basis perpendicular to the line. Perp1
            // prefers a horizontal axis (CrossProduct with WorldUp) so most
            // line orientations get visually consistent "width vs height" of
            // the box cross-section. Vertical lines (Perp1 collapses to zero)
            // fall back to world-X.
            const auto WorldUp = FVector::UpVector;
            auto Perp1 = FVector::CrossProduct(LineDir, WorldUp).GetSafeNormal();
            if (Perp1.IsNearlyZero())
            {
                Perp1 = FVector::ForwardVector;
            }
            const auto Perp2 = FVector::CrossProduct(LineDir, Perp1).GetSafeNormal();

            // Bias the box slightly outward from the entity local origin so it
            // doesn't sit fully embedded in the filled Section-0 mesh (which
            // causes Z-fighting / visual camouflage). All basic PMG shapes are
            // centered at the entity local origin, so the line midpoint's
            // direction from origin is a usable "outward" vector. Lines that
            // happen to pass through the origin fall back to no bias.
            const auto LineMid = (InLine._Start + InLine._End) * 0.5f;
            const auto Outward = LineMid.GetSafeNormal();
            const auto OutwardBias = Outward * (InLine._Thickness * 0.5f);

            const auto Half = InLine._Thickness * 0.5f;
            const auto P1 = Perp1 * Half;
            const auto P2 = Perp2 * Half;

            const auto BaseIndex = OutVertices.Num();

            // 8 corners of the stretched box. Convention at each end:
            // (0,4): -P1 -P2  (1,5): +P1 -P2  (2,6): +P1 +P2  (3,7): -P1 +P2
            // Verts 0-3 are at Start; 4-7 are at End.
            OutVertices.Add(InLine._Start - P1 - P2 + OutwardBias);
            OutVertices.Add(InLine._Start + P1 - P2 + OutwardBias);
            OutVertices.Add(InLine._Start + P1 + P2 + OutwardBias);
            OutVertices.Add(InLine._Start - P1 + P2 + OutwardBias);
            OutVertices.Add(InLine._End   - P1 - P2 + OutwardBias);
            OutVertices.Add(InLine._End   + P1 - P2 + OutwardBias);
            OutVertices.Add(InLine._End   + P1 + P2 + OutwardBias);
            OutVertices.Add(InLine._End   - P1 + P2 + OutwardBias);

            // Material is unlit so per-face normals don't affect shading;
            // populate with a placeholder so the array sizes match the
            // procmesh API expectations.
            for (auto i = 0; i < 8; ++i)
            {
                OutNormals.Add(FVector::UpVector);
                OutUVs.Add(FVector2D::ZeroVector);
            }

            // 6 faces × 2 triangles. Winding is CCW from the OUTSIDE of each
            // face so default backface culling renders the box as a closed
            // solid. No double-winding needed (the box is closed; the inside
            // is never directly viewed at typical camera angles).
            const auto AddQuad =
                [&](int32 A, int32 B, int32 C, int32 D)
                {
                    OutTriangles.Add(BaseIndex + A);
                    OutTriangles.Add(BaseIndex + B);
                    OutTriangles.Add(BaseIndex + C);
                    OutTriangles.Add(BaseIndex + A);
                    OutTriangles.Add(BaseIndex + C);
                    OutTriangles.Add(BaseIndex + D);
                };

            AddQuad(0, 3, 2, 1);    // Start cap, outward = -LineDir
            AddQuad(4, 5, 6, 7);    // End cap,   outward = +LineDir
            AddQuad(0, 1, 5, 4);    // -Perp2 face
            AddQuad(3, 7, 6, 2);    // +Perp2 face
            AddQuad(0, 4, 7, 3);    // -Perp1 face
            AddQuad(1, 2, 6, 5);    // +Perp1 face
        }
    }

    auto
        FProcessor_Pmg_DebugShape_BakeLines::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            const FFragment_Pmg_DebugShape_Lines& InLines,
            const FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void
    {
        SCOPE_CYCLE_COUNTER(STAT_Pmg_DebugDrawLines);
        INC_DWORD_STAT_BY(STAT_Pmg_DebugLines, InLines.Get_Lines().Num());

        // Consume the gate immediately — even if we early-out below (no
        // valid mesh, no lines), the entity shouldn't keep re-firing this
        // processor every tick. A fresh stamp (via Append_Debug*_World) will
        // re-stamp the tag and we'll re-bake then.
        InHandle.Remove<MarkedDirtyBy>();

        auto MeshComponent = InCurrent._MeshComponent.Get();
        if (ck::Is_NOT_Valid(MeshComponent, ck::IsValid_Policy_NullptrOnly{}))
        { return; }

        // Always clear any prior wireframe section before re-baking so the
        // geometry stays in sync with the current _Lines content.
        MeshComponent->ClearMeshSection(pmg_bake_lines_helpers::WireframeSectionIndex);

        // DrawLines=false or empty cache → leave the section cleared.
        if (NOT InCommon.Get_DrawLines() || InLines.Get_Lines().IsEmpty())
        { return; }

        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        for (const auto& Line : InLines.Get_Lines())
        {
            pmg_bake_lines_helpers::BuildBoxForLine(
                Line, Vertices, Triangles, Normals, UVs);
        }

        if (Vertices.IsEmpty())
        { return; }

        constexpr auto bCreateCollision = false;
        MeshComponent->CreateMeshSection_LinearColor(
            pmg_bake_lines_helpers::WireframeSectionIndex,
            Vertices, Triangles, Normals, UVs,
            TArray<FLinearColor>{}, TArray<FProcMeshTangent>{},
            bCreateCollision);

        // The wireframe needs its own MID — sharing slot 0's MID makes the
        // wireframe inherit the fill's alpha, which produces a translucent
        // outline that visually camouflages with the fill. Instead, spawn a
        // sibling MID from the same parent material on first bake and force
        // its alpha to 1 so wireframes stay opaque regardless of fill
        // transparency. Request_SetColor mirrors RGB updates to both MIDs.
        auto* Slot0Mat = MeshComponent->GetMaterial(0);
        auto* FillMID = Cast<UMaterialInstanceDynamic>(Slot0Mat);
        auto* WireframeMID = Cast<UMaterialInstanceDynamic>(
            MeshComponent->GetMaterial(pmg_bake_lines_helpers::WireframeSectionIndex));

        if (WireframeMID == nullptr && FillMID != nullptr && ck::IsValid(FillMID->Parent))
        {
            WireframeMID = UMaterialInstanceDynamic::Create(FillMID->Parent, MeshComponent);
            MeshComponent->SetMaterial(pmg_bake_lines_helpers::WireframeSectionIndex, WireframeMID);
        }

        if (WireframeMID != nullptr)
        {
            auto WireframeColor = InCommon.Get_Color();
            WireframeColor.A = 1.0f;
            WireframeMID->SetVectorParameterValue(FName(TEXT("Color")), WireframeColor);
        }

        // Mirror the filled mesh's visibility on Section 1 so an entity that
        // was set to Hidden before lines were ever appended doesn't suddenly
        // show wireframes when the bake runs. The handle for runtime
        // visibility flips lives in DoHandleRequest(SetRenderMode), which
        // toggles the whole component (Sections 0 + 1 ride together).
        const auto bShouldShow = InCommon.Get_RenderMode() != ECk_Pmg_RenderMode::Hidden;
        MeshComponent->SetMeshSectionVisible(pmg_bake_lines_helpers::WireframeSectionIndex, bShouldShow);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_DebugShape_CheckDuration::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void
    {
        const auto& Duration = InCommon.Get_Duration();

        if (Duration.Get_Seconds() < 0.0f)
        { return; }

        auto Now = UCk_Utils_Time_UE::Get_WorldTime(FCk_Utils_Time_GetWorldTime_Params(UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle))).Get_WorldTime().Get_Time();
        const auto ElapsedTime = Now - InCurrent._SpawnTime;

        if (ElapsedTime.Get_Seconds() >= Duration.Get_Seconds())
        {
            UCk_Utils_EntityLifetime_UE::Request_DestroyEntity(InHandle);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_DebugShape_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void
    {
        ck::pmg::Verbose(TEXT("Tearing down Pmg DebugShape [{}]"), InHandle);

        auto MeshComponent = InCurrent._MeshComponent.Get();
        if (ck::IsValid(MeshComponent))
        {
            // unpin before DestroyComponent (destroy garbage-marks the object, failing release's validity check)
            UCk_Utils_Object_UE::TryReleaseToPool(MeshComponent);
            MeshComponent->DestroyComponent();
        }

        InCurrent._MeshComponent.Reset();
    }

    // --------------------------------------------------------------------------------------------------------------------
    // FProcessor_Pmg_DebugShape_HandleRequests
    // --------------------------------------------------------------------------------------------------------------------

    namespace pmg_debug_shape_helpers
    {
        // Pulls the procmesh's element-0 dynamic material instance, if one exists.
        // FinalizeMeshComponent_Basic in CkPmg_Processor_BasicShapes.cpp creates a
        // UMaterialInstanceDynamic on the procmesh's slot 0 during Setup, so this
        // is the canonical color hook for live mutations.
        auto Get_DynamicMaterial(
            UProceduralMeshComponent* InMesh)
            -> UMaterialInstanceDynamic*
        {
            if (ck::Is_NOT_Valid(InMesh, ck::IsValid_Policy_NullptrOnly{}))
            { return nullptr; }
            return Cast<UMaterialInstanceDynamic>(InMesh->GetMaterial(0));
        }
    }

    auto
        FProcessor_Pmg_DebugShape_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            FFragment_Pmg_DebugShape_Requests& InRequestsComp) const
        -> void
    {
        InHandle.CopyAndRemove(InRequestsComp, [&](const FFragment_Pmg_DebugShape_Requests& InRequests)
        {
            ck::algo::ForEachRequest(InRequests._Requests, ck::Visitor(
            [&](const auto& InRequest) -> void
            {
                DoHandleRequest(InHandle, InCommon, InCurrent, InRequest);
            }), ck::policy::DontResetContainer{});
        });
    }

    auto
        FProcessor_Pmg_DebugShape_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetColor& InRequest)
        -> void
    {
        const auto NewColor = InRequest.Get_NewColor();

        // Cache on Common so DrawLines (per-tick wireframe re-emit) picks the
        // new color too — the wireframe processor reads from Common._Color.
        InCommon._Color = NewColor;

        // Push to the procmesh's MID Color parameter — this changes the filled
        // mesh's appearance immediately. No mesh rebuild required.
        if (auto* MID = pmg_debug_shape_helpers::Get_DynamicMaterial(InCurrent._MeshComponent.Get()))
        {
            MID->SetVectorParameterValue(FName(TEXT("Color")), NewColor);
        }

        // Mirror the RGB to the wireframe MID at slot 1 (created lazily by
        // BakeLines). Force alpha to 1 so the wireframe stays opaque even
        // when the fill is translucent — otherwise the outline visually
        // blends into the fill.
        if (auto* MeshComponent = InCurrent._MeshComponent.Get();
            ck::IsValid(MeshComponent, ck::IsValid_Policy_NullptrOnly{}))
        {
            if (auto* WireframeMID = Cast<UMaterialInstanceDynamic>(
                    MeshComponent->GetMaterial(pmg_bake_lines_helpers::WireframeSectionIndex)))
            {
                auto WireframeColor = NewColor;
                WireframeColor.A = 1.0f;
                WireframeMID->SetVectorParameterValue(FName(TEXT("Color")), WireframeColor);
            }
        }
    }

    auto
        FProcessor_Pmg_DebugShape_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetLineThickness& InRequest)
        -> void
    {
        InCommon._LineThickness = InRequest.Get_NewLineThickness();
        // Per-line thickness is baked into the rectangle geometry, so any
        // change here requires re-running BakeLines. The Append_Debug*_World
        // helpers also write thickness into each FCk_Pmg_DebugLine — Common's
        // cached value is the "uniform override" semantically, but the bake
        // currently reads per-line. Stamp the tag so a future change-on-rebake
        // path can pick this up.
        InHandle.AddOrGet<FTag_Pmg_DebugShape_LinesNeedBaking>();
    }

    auto
        FProcessor_Pmg_DebugShape_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetDrawLines& InRequest)
        -> void
    {
        InCommon._DrawLines = InRequest.Get_NewDrawLines();
        // BakeLines respects Common._DrawLines: when re-run it either rebuilds
        // Section 1 or clears it. Re-stamping the tag triggers exactly that.
        InHandle.AddOrGet<FTag_Pmg_DebugShape_LinesNeedBaking>();
    }

    auto
        FProcessor_Pmg_DebugShape_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetDuration& InRequest)
        -> void
    {
        // CheckDuration processor compares against Common._Duration each tick.
        // Bumping it grants the shape additional time before auto-destroy;
        // setting it negative makes the shape persistent.
        InCommon._Duration = InRequest.Get_NewDuration();
    }

    auto
        FProcessor_Pmg_DebugShape_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetRenderMode& InRequest)
        -> void
    {
        const auto NewMode = InRequest.Get_NewRenderMode();
        InCommon._RenderMode = NewMode;

        // Mirror the visibility logic from FinalizeMeshComponent_Basic so this
        // immediately reflects on the live procmesh — no waiting for re-setup.
        if (auto* Mesh = InCurrent._MeshComponent.Get())
        {
            const auto ShouldBeVisible = NewMode != ECk_Pmg_RenderMode::Hidden;
            Mesh->SetVisibility(ShouldBeVisible, true);
            Mesh->SetHiddenInGame(NOT ShouldBeVisible);
        }
    }

    auto
        FProcessor_Pmg_DebugShape_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent,
            const FCk_Request_Pmg_DebugShape_SetEnableCollision& InRequest)
        -> void
    {
        const auto NewEnable = InRequest.Get_NewEnableCollision();
        InCommon._EnableCollision = NewEnable;

        if (auto* Mesh = InCurrent._MeshComponent.Get())
        {
            Mesh->SetCollisionEnabled(NewEnable
                ? ECollisionEnabled::QueryAndPhysics
                : ECollisionEnabled::NoCollision);
        }
    }
}
