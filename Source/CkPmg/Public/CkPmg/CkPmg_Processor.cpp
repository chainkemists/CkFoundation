#include "CkPmg_Processor.h"
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
#include "CkPmg_Utils.h"
#include "CkPmg_Utils_IconShapes.h"
#include "CkPmg_Utils_SymbolShapes.h"

#include <MaterialDomain.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <ProceduralMeshComponent.h>

#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_GROUP(ck::FGroup_Pmg_DebugShape_Setup);

CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Donut_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Donut_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Donut_UpdateTransform);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_Donut_EndPlay);
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

namespace ck_pmg
{
    auto
        DoGenerateDonutMesh(
            UProceduralMeshComponent* InMeshComponent,
            float InInnerRadius,
            float InOuterRadius,
            int32 InSegments,
            float InFillAngle,
            ECk_Pmg_RenderMode InRenderMode)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InMeshComponent), TEXT("Invalid mesh component"))
        { return; }

        CK_ENSURE_IF_NOT(InInnerRadius < InOuterRadius, TEXT("Inner radius must be less than outer radius"))
        { return; }

        CK_ENSURE_IF_NOT(InSegments >= 3, TEXT("Segments must be at least 3"))
        { return; }

        auto Vertices = TArray<FVector>{};
        auto Triangles = TArray<int32>{};
        auto Normals = TArray<FVector>{};
        auto UVs = TArray<FVector2D>{};

        const auto FillRadians = FMath::DegreesToRadians(FMath::Clamp(InFillAngle, 0.0f, 360.0f));
        const auto SegmentsToGenerate = InFillAngle >= 360.0f ? InSegments : InSegments + 1;

        // Generate vertices for top face
        for (auto i = 0; i < SegmentsToGenerate; ++i)
        {
            const auto Angle = (FillRadians * i) / InSegments;
            const auto CosAngle = FMath::Cos(Angle);
            const auto SinAngle = FMath::Sin(Angle);

            Vertices.Add(FVector{
                CosAngle * InOuterRadius,
                SinAngle * InOuterRadius,
                0.0f
            });

            Vertices.Add(FVector{
                CosAngle * InInnerRadius,
                SinAngle * InInnerRadius,
                0.0f
            });

            const auto UVAngle = static_cast<float>(i) / InSegments;
            UVs.Add(FVector2D{UVAngle, 1.0f});
            UVs.Add(FVector2D{UVAngle, 0.0f});

            Normals.Add(FVector::UpVector);
            Normals.Add(FVector::UpVector);
        }

        const auto TopVertexCount = Vertices.Num();

        // Generate vertices for bottom face (same positions, reversed normals) if double-sided
        if (InRenderMode == ECk_Pmg_RenderMode::DoubleSided)
        {
            for (auto i = 0; i < SegmentsToGenerate; ++i)
            {
                const auto Angle = (FillRadians * i) / InSegments;
                const auto CosAngle = FMath::Cos(Angle);
                const auto SinAngle = FMath::Sin(Angle);

                Vertices.Add(FVector{
                    CosAngle * InOuterRadius,
                    SinAngle * InOuterRadius,
                    0.0f
                });

                Vertices.Add(FVector{
                    CosAngle * InInnerRadius,
                    SinAngle * InInnerRadius,
                    0.0f
                });

                const auto UVAngle = static_cast<float>(i) / InSegments;
                UVs.Add(FVector2D{UVAngle, 1.0f});
                UVs.Add(FVector2D{UVAngle, 0.0f});

                Normals.Add(FVector::DownVector);
                Normals.Add(FVector::DownVector);
            }
        }

        const auto TriangleCount = InFillAngle >= 360.0f ? InSegments : InSegments;

        // Generate triangles for top face
        for (auto i = 0; i < TriangleCount; ++i)
        {
            const auto Current = i * 2;
            const auto Next = ((i + 1) % SegmentsToGenerate) * 2;

            Triangles.Add(Current);
            Triangles.Add(Next);
            Triangles.Add(Current + 1);

            Triangles.Add(Next);
            Triangles.Add(Next + 1);
            Triangles.Add(Current + 1);
        }

        // Generate triangles for bottom face (reversed winding) if double-sided
        if (InRenderMode == ECk_Pmg_RenderMode::DoubleSided)
        {
            for (auto i = 0; i < TriangleCount; ++i)
            {
                const auto Current = TopVertexCount + i * 2;
                const auto Next = TopVertexCount + ((i + 1) % SegmentsToGenerate) * 2;

                Triangles.Add(Current);
                Triangles.Add(Current + 1);
                Triangles.Add(Next);

                Triangles.Add(Next);
                Triangles.Add(Current + 1);
                Triangles.Add(Next + 1);
            }
        }

        ck::pmg::Verbose(TEXT("Generated donut mesh: Vertices=[{}], Triangles=[{}], Segments=[{}]"),
            Vertices.Num(), Triangles.Num(), InSegments);

        InMeshComponent->CreateMeshSection_LinearColor(
            0,
            Vertices,
            Triangles,
            Normals,
            UVs,
            TArray<FLinearColor>{},
            TArray<FProcMeshTangent>{},
            true
        );
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FProcessor_Pmg_Donut_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_Donut_Params& InParams,
            FFragment_Pmg_Donut_Current& InCurrent)
            -> void
    {
        InHandle.Remove<MarkedDirtyBy>();

        ck::pmg::Verbose(TEXT("Setting up Pmg Donut [{}]"), InHandle);

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World),
            TEXT("Pmg Donut [{}] could not get valid World"), InHandle)
        { return; }

        auto MeshComponent = NewObject<UProceduralMeshComponent>(
            World,
            UProceduralMeshComponent::StaticClass(),
            NAME_None,
            RF_Transient);

        CK_ENSURE_IF_NOT(ck::IsValid(MeshComponent),
            TEXT("Pmg Donut [{}] failed to create ProceduralMeshComponent"), InHandle)
        { return; }

        MeshComponent->SetWorldLocation(FVector::ZeroVector);
        MeshComponent->RegisterComponentWithWorld(World);

        if (MeshComponent->HasBegunPlay() == false)
        {
            MeshComponent->BeginPlay();
        }

        MeshComponent->SetVisibility(true);
        MeshComponent->SetHiddenInGame(false);
        MeshComponent->SetCastShadow(true);

        InCurrent._InnerRadius = InParams.Get_Params().Get_InnerRadius();
        InCurrent._OuterRadius = InParams.Get_Params().Get_OuterRadius();
        InCurrent._Segments = InParams.Get_Params().Get_Segments();
        InCurrent._FillAngle = InParams.Get_Params().Get_FillAngle();
        InCurrent._Material = InParams.Get_Params().Get_Material();
        InCurrent._EnableCollision = InParams.Get_Params().Get_EnableCollision();
        InCurrent._RenderMode = InParams.Get_Params().Get_RenderMode();

        ck_pmg::DoGenerateDonutMesh(
            MeshComponent,
            InCurrent._InnerRadius,
            InCurrent._OuterRadius,
            InCurrent._Segments,
            InCurrent._FillAngle,
            InCurrent._RenderMode);

        const auto ShouldBeVisible = InCurrent._RenderMode != ECk_Pmg_RenderMode::Hidden;
        MeshComponent->SetVisibility(ShouldBeVisible, true);
        MeshComponent->SetHiddenInGame(!ShouldBeVisible);
        MeshComponent->bHiddenInGame = !ShouldBeVisible;
        MeshComponent->SetRenderInMainPass(true);
        MeshComponent->SetRenderInDepthPass(true);

        if (ck::IsValid(InCurrent._Material))
        {
            MeshComponent->SetMaterial(0, InCurrent._Material);
        }
        else
        {
            ck::pmg::Warning(TEXT("No material specified for Pmg Donut [{}] - using default material"), InHandle);
            auto DefaultMaterial = UMaterial::GetDefaultMaterial(MD_Surface);
            if (ck::IsValid(DefaultMaterial))
            {
                MeshComponent->SetMaterial(0, DefaultMaterial);
            }
        }

        MeshComponent->SetCollisionEnabled(
            InCurrent._EnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

        MeshComponent->UpdateBounds();
        MeshComponent->MarkRenderStateDirty();

        InCurrent._MeshComponent = TStrongObjectPtr{MeshComponent};

        if (InHandle.Has<ck::FFragment_Transform>())
        {
            const auto& Transform = InHandle.Get<ck::FFragment_Transform>();
            const auto& CurrentTransform = Transform.Get_Transform();
            MeshComponent->SetWorldTransform(CurrentTransform);
        }
        else
        {
            ck::pmg::Warning(TEXT("Pmg Donut [{}] has NO Transform fragment - mesh will be at world origin (0,0,0)"), InHandle);
        }

        ck::pmg::Verbose(TEXT("Pmg Donut [{}] setup complete"), InHandle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_Donut_HandleRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_Donut_Current& InCurrent,
            const FFragment_Pmg_Donut_UpdateParams& InRequest) const
            -> void
    {
        DoHandleRequest(InHandle, InCurrent, InRequest);
        InHandle.Remove<MarkedDirtyBy>();
    }

    auto
        FProcessor_Pmg_Donut_HandleRequests::
        DoHandleRequest(
            HandleType InHandle,
            FFragment_Pmg_Donut_Current& InCurrent,
            const FCk_Request_Pmg_Donut_UpdateParams& InRequest)
            -> void
    {
        ck::pmg::Verbose(TEXT("Handling update params request for Pmg Donut [{}]"), InHandle);

        auto MeshComponent = InCurrent._MeshComponent.Get();
        CK_ENSURE_IF_NOT(ck::IsValid(MeshComponent),
            TEXT("Pmg Donut [{}] has invalid mesh component"), InHandle)
        { return; }

        auto NeedsRegeneration = false;

        if (InRequest.Get_InnerRadius().IsSet())
        {
            InCurrent._InnerRadius = InRequest.Get_InnerRadius().GetValue();
            NeedsRegeneration = true;
        }

        if (InRequest.Get_OuterRadius().IsSet())
        {
            InCurrent._OuterRadius = InRequest.Get_OuterRadius().GetValue();
            NeedsRegeneration = true;
        }

        if (InRequest.Get_Segments().IsSet())
        {
            InCurrent._Segments = InRequest.Get_Segments().GetValue();
            NeedsRegeneration = true;
        }

        if (InRequest.Get_FillAngle().IsSet())
        {
            InCurrent._FillAngle = InRequest.Get_FillAngle().GetValue();
            NeedsRegeneration = true;
        }

        if (InRequest.Get_Material().IsSet())
        {
            InCurrent._Material = InRequest.Get_Material().GetValue();
            if (ck::IsValid(InCurrent._Material))
            {
                MeshComponent->SetMaterial(0, InCurrent._Material);
            }
        }

        if (InRequest.Get_EnableCollision().IsSet())
        {
            InCurrent._EnableCollision = InRequest.Get_EnableCollision().GetValue();
            MeshComponent->SetCollisionEnabled(
                InCurrent._EnableCollision ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);
        }

        if (InRequest.Get_RenderMode().IsSet())
        {
            const auto OldRenderMode = InCurrent._RenderMode;
            InCurrent._RenderMode = InRequest.Get_RenderMode().GetValue();

            const auto ShouldBeVisible = InCurrent._RenderMode != ECk_Pmg_RenderMode::Hidden;
            MeshComponent->SetVisibility(ShouldBeVisible, true);
            MeshComponent->SetHiddenInGame(!ShouldBeVisible);

            const auto OldIsDoubleSided = OldRenderMode == ECk_Pmg_RenderMode::DoubleSided;
            const auto NewIsDoubleSided = InCurrent._RenderMode == ECk_Pmg_RenderMode::DoubleSided;
            if (OldIsDoubleSided != NewIsDoubleSided)
            {
                NeedsRegeneration = true;
            }
        }

        if (NeedsRegeneration)
        {
            CK_ENSURE_IF_NOT(InCurrent._InnerRadius < InCurrent._OuterRadius,
                TEXT("Inner radius must be less than outer radius for Pmg Donut [{}]"), InHandle)
            { return; }

            MeshComponent->ClearAllMeshSections();
            ck_pmg::DoGenerateDonutMesh(
                MeshComponent,
                InCurrent._InnerRadius,
                InCurrent._OuterRadius,
                InCurrent._Segments,
                InCurrent._FillAngle,
                InCurrent._RenderMode);

            ck::pmg::Verbose(TEXT("Pmg Donut [{}] mesh regenerated"), InHandle);
        }
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_Donut_UpdateTransform::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_Donut_Current& InCurrent,
            const FFragment_Transform& InTransform)
            -> void
    {
        auto MeshComponent = InCurrent._MeshComponent.Get();
        // Tolerate a torn-down mesh (reset by EndPlay during teardown) — skip gracefully.
        if (ck::Is_NOT_Valid(MeshComponent))
        { return; }

        const auto& CurrentTransform = InTransform.Get_Transform();
        MeshComponent->SetWorldTransform(CurrentTransform);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_Donut_EndPlay::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            FFragment_Pmg_Donut_Current& InCurrent)
            -> void
    {
        ck::pmg::Verbose(TEXT("Tearing down Pmg Donut [{}]"), InHandle);

        auto MeshComponent = InCurrent._MeshComponent.Get();
        if (ck::IsValid(MeshComponent))
        {
            MeshComponent->DestroyComponent();
        }

        InCurrent._MeshComponent.Reset();
    }
}

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
