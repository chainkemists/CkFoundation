#include "CkPmg_Processor.h"
#include "CkPmg_Processor_AngularShapes.h"
#include "CkPmg_Processor_BasicShapes.h"
#include "CkPmg_Processor_DirectionalShapes.h"
#include "CkPmg_Processor_FlatShapes.h"
#include "CkPmg_Processor_IconShapes.h"
#include "CkPmg_Processor_SymbolShapes.h"

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
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_DrawLines);
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
        CK_ENSURE_IF_NOT(ck::IsValid(MeshComponent),
            TEXT("Pmg Donut [{}] has invalid mesh component"), InHandle)
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
        CK_ENSURE_IF_NOT(ck::IsValid(MeshComponent),
            TEXT("Pmg DebugShape [{}] has invalid mesh component"), InHandle)
        { return; }

        MeshComponent->SetWorldTransform(InTransform.Get_Transform());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_DebugShape_DrawLines::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            const FFragment_Pmg_DebugShape_Lines& InLines,
            const FFragment_Transform& InTransform)
            -> void
    {
        if (NOT InCommon.Get_DrawLines())
        { return; }

        // Honor RenderMode::Hidden — skip wireframe emission for shapes whose mesh is hidden.
        // Without this gate the line processor keeps drawing per-frame even when the procmesh
        // has been hidden via Request_SetRenderMode/Request_SetVisible, leaving a "ghost"
        // wireframe of an invisible body. Pairs with the visibility flip in
        // FProcessor_Pmg_DebugShape_HandleRequests::DoHandleRequest(SetRenderMode).
        if (InCommon.Get_RenderMode() == ECk_Pmg_RenderMode::Hidden)
        { return; }

        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        const auto& Xform = InTransform.Get_Transform();

        for (const auto& Line : InLines.Get_Lines())
        {
            const auto WorldStart = Xform.TransformPosition(Line._Start);
            const auto WorldEnd   = Xform.TransformPosition(Line._End);
            UCk_Utils_DebugDraw_UE::DrawDebugLine(
                World, WorldStart, WorldEnd, Line._Color, 0.0f, Line._Thickness);
        }
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
        // DrawLines processor reads thickness from Common._LineThickness each
        // tick; updating the cache is sufficient.
        InCommon._LineThickness = InRequest.Get_NewLineThickness();
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
        // DrawLines processor gates per tick on Common._DrawLines; updating the
        // cache flips the wireframe overlay on/off starting next tick.
        InCommon._DrawLines = InRequest.Get_NewDrawLines();
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
