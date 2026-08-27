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
#include "CkCore/Diagnostics/CkDiagnosticVisibility.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "CkPmg/CkPmg_Log.h"
#include "CkPmg/CkPmg_Stats.h"
#include "CkPmg_Utils_DebugShapes.h"
#include "CkPmg_Utils_IconShapes.h"
#include "CkPmg_Utils_SymbolShapes.h"

#include <MaterialDomain.h>
#include <Materials/MaterialInstanceDynamic.h>
#include <ProceduralMeshComponent.h>

#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Scheduler/CkProcessorRegistration.h"

// --------------------------------------------------------------------------------------------------------------------

DECLARE_CYCLE_STAT(TEXT("Pmg::DebugDrawLines"), STAT_Pmg_DebugDrawLines, STATGROUP_CkPmg);
DECLARE_DWORD_COUNTER_STAT(TEXT("Pmg Debug Lines"), STAT_Pmg_DebugLines, STATGROUP_CkPmg);

// --------------------------------------------------------------------------------------------------------------------

CK_REGISTER_GROUP(ck::FGroup_Pmg_DebugShape_Setup);

CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_LineSet_Setup);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_UpdateTransform);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_ApplyRuntimeVisibility);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_BakeLines);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_CheckDuration);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_HandleRequests);
CK_REGISTER_PROCESSOR(ck::FProcessor_Pmg_DebugShape_CancelPendingRequests);
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
        FProcessor_Pmg_DebugShape_LineSet_Setup::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            FFragment_Pmg_DebugShape_Current& InCurrent)
        -> void
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        if (ck::Is_NOT_Valid(World))
        { return; }

        auto MeshComponent = UCk_Utils_Object_UE::Request_CreateNewObject<UProceduralMeshComponent>(
            UCk_Utils_Pmg_DebugShape_UE::Get_MeshComponentOuter(World, InHandle),
            UProceduralMeshComponent::StaticClass(),
            nullptr,
            FCk_ObjectPooling_PoolParams{}.Set_RecyclePolicy(ECk_ObjectPooling_RecyclePolicy::DestroyOnRelease),
            nullptr);
        if (ck::Is_NOT_Valid(MeshComponent))
        { return; }

        MeshComponent->SetWorldLocation(FVector::ZeroVector);
        MeshComponent->RegisterComponentWithWorld(World);
        if (MeshComponent->HasBegunPlay() == false)
        { MeshComponent->BeginPlay(); }
        MeshComponent->SetCastShadow(false);
        MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);

        auto* TranslucentMaterial = LoadObject<UMaterial>(
            nullptr,
            TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));
        if (ck::IsValid(TranslucentMaterial))
        {
            auto* DynamicMaterial = UMaterialInstanceDynamic::Create(TranslucentMaterial, MeshComponent);
            if (ck::IsValid(DynamicMaterial))
            {
                DynamicMaterial->SetVectorParameterValue(
                    FName(TEXT("Color")),
                    pmg_debug_shape::Get_FillColor(InCommon.Get_Color()));
                MeshComponent->SetMaterial(0, DynamicMaterial);
            }
        }
        else
        {
            ck::pmg::Warning(TEXT("Failed to load M_SimpleUnlitTranslucent for Pmg line set [{}]"), InHandle);
        }

        const auto ShouldBeVisible = InCommon.Get_RenderMode() != ECk_Pmg_RenderMode::Hidden &&
                                     NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode();
        MeshComponent->SetVisibility(ShouldBeVisible, true);
        MeshComponent->SetHiddenInGame(NOT ShouldBeVisible);

        if (InHandle.Has<FFragment_Transform>())
        { MeshComponent->SetWorldTransform(InHandle.Get<FFragment_Transform>().Get_Transform()); }

        InCurrent = FFragment_Pmg_DebugShape_Current{MeshComponent, FCk_Time{InDeltaT.Get_Seconds()}};
        InHandle.Remove<MarkedDirtyBy>();
    }

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
        // EndPlay resets _MeshComponent and a transform tick can race that window — skip, don't ensure.
        if (ck::Is_NOT_Valid(MeshComponent))
        { return; }

        MeshComponent->SetWorldTransform(InTransform.Get_Transform());
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_DebugShape_ApplyRuntimeVisibility::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Common& InCommon,
            const FFragment_Pmg_DebugShape_Current& InCurrent)
            -> void
    {
        auto* MeshComponent = InCurrent._MeshComponent.Get();
        if (ck::Is_NOT_Valid(MeshComponent))
        { return; }

        const auto IsLocallyVisible = InCommon.Get_RenderMode() != ECk_Pmg_RenderMode::Hidden;
        const auto ShouldBeVisible = IsLocallyVisible &&
                                     NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode();
        MeshComponent->SetVisibility(ShouldBeVisible, true);
        MeshComponent->SetHiddenInGame(NOT ShouldBeVisible);
    }

    // --------------------------------------------------------------------------------------------------------------------

    namespace pmg_bake_lines_helpers
    {
        constexpr int32 FilledSectionIndex = 0;
        constexpr int32 WireframeSectionIndex = 1;

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

            // Crossing with world-up keeps box cross-sections consistent across line orientations.
            const auto WorldUp = FVector::UpVector;
            auto Perp1 = FVector::CrossProduct(LineDir, WorldUp).GetSafeNormal();
            if (Perp1.IsNearlyZero())
            {
                Perp1 = FVector::ForwardVector;
            }
            const auto Perp2 = FVector::CrossProduct(LineDir, Perp1).GetSafeNormal();

            // Lifts the box clear of the filled section-0 mesh; every basic PMG shape is centred
            // on the entity local origin, which is what makes the midpoint direction "outward".
            const auto LineMid = (InLine._Start + InLine._End) * 0.5f;
            const auto Outward = LineMid.GetSafeNormal();
            const auto OutwardBias = Outward * (InLine._Thickness * 0.5f);

            const auto Half = InLine._Thickness * 0.5f;
            const auto P1 = Perp1 * Half;
            const auto P2 = Perp2 * Half;

            const auto BaseIndex = OutVertices.Num();

            OutVertices.Add(InLine._Start - P1 - P2 + OutwardBias);
            OutVertices.Add(InLine._Start + P1 - P2 + OutwardBias);
            OutVertices.Add(InLine._Start + P1 + P2 + OutwardBias);
            OutVertices.Add(InLine._Start - P1 + P2 + OutwardBias);
            OutVertices.Add(InLine._End   - P1 - P2 + OutwardBias);
            OutVertices.Add(InLine._End   + P1 - P2 + OutwardBias);
            OutVertices.Add(InLine._End   + P1 + P2 + OutwardBias);
            OutVertices.Add(InLine._End   - P1 + P2 + OutwardBias);

            // The material is unlit, so these normals are placeholders sized to the procmesh API.
            constexpr auto BoxVertexCount = 8;
            for (auto i = 0; i < BoxVertexCount; ++i)
            {
                OutNormals.Add(FVector::UpVector);
                OutUVs.Add(FVector2D::ZeroVector);
            }

            // Winding is CCW from the OUTSIDE of each face, so backface culling leaves a closed solid.
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

        // Consumed BEFORE the early-outs below, or a shape with no mesh or no lines re-fires forever.
        InHandle.Remove<MarkedDirtyBy>();

        auto MeshComponent = InCurrent._MeshComponent.Get();
        if (ck::Is_NOT_Valid(MeshComponent))
        { return; }

        MeshComponent->ClearMeshSection(pmg_bake_lines_helpers::WireframeSectionIndex);

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

        // Its OWN MID: sharing the fill's would inherit the fill alpha and camouflage the outline.
        auto* FilledMat = MeshComponent->GetMaterial(pmg_bake_lines_helpers::FilledSectionIndex);
        auto* FillMID = Cast<UMaterialInstanceDynamic>(FilledMat);
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

        const auto bShouldShow = InCommon.Get_RenderMode() != ECk_Pmg_RenderMode::Hidden &&
                                 NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode();
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

    namespace pmg_debug_shape_helpers
    {
        // The filled section's MID is created by FinalizeMeshComponent_Basic during Setup.
        auto Get_DynamicMaterial(
            UProceduralMeshComponent* InMesh)
            -> UMaterialInstanceDynamic*
        {
            if (ck::Is_NOT_Valid(InMesh))
            { return nullptr; }
            return Cast<UMaterialInstanceDynamic>(InMesh->GetMaterial(pmg_bake_lines_helpers::FilledSectionIndex));
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
                // Every DoHandleRequest overload below is void and has no rejection path, so reaching
                // the line after the call IS the success condition.
                auto Result = ECk_Request_OperationResult::Failed;
                const auto Guard = ck::MakeCompletionGuard(InRequest, InHandle, Result);

                DoHandleRequest(InHandle, InCommon, InCurrent, InRequest);

                Result = ECk_Request_OperationResult::Succeeded;
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

        InCommon._Color = NewColor;

        if (auto* MID = pmg_debug_shape_helpers::Get_DynamicMaterial(InCurrent._MeshComponent.Get()))
        {
            MID->SetVectorParameterValue(FName(TEXT("Color")), pmg_debug_shape::Get_FillColor(NewColor));
        }

        if (auto* MeshComponent = InCurrent._MeshComponent.Get();
            ck::IsValid(MeshComponent))
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
        const auto NewThickness = InRequest.Get_NewLineThickness();
        InCommon._LineThickness = NewThickness;
        if (InHandle.Has<FFragment_Pmg_DebugShape_Lines>())
        {
            for (auto& Line : InHandle.Get<FFragment_Pmg_DebugShape_Lines>()._Lines)
            { Line._Thickness = NewThickness; }
        }
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
        InCommon._Duration = InRequest.Get_NewDuration();
        pmg_debug_shape::UpdateDurationMembership(InHandle, InCommon._Duration);
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

        // Mirrors the visibility logic in FinalizeMeshComponent_Basic.
        if (auto* Mesh = InCurrent._MeshComponent.Get())
        {
            const auto ShouldBeVisible = NewMode != ECk_Pmg_RenderMode::Hidden &&
                                         NOT ck::diagnostic_visibility::Is_HiddenForStreamerMode();
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

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FProcessor_Pmg_DebugShape_CancelPendingRequests::
        ForEachEntity(
            TimeType InDeltaT,
            HandleType InHandle,
            const FFragment_Pmg_DebugShape_Requests& InRequestsComp)
        -> void
    {
        request::FireCancelledForPending(InHandle, InRequestsComp.Get_Requests());
    }
}
