#include "CkPmg_Processor_TextShapes.h"

#include "CkPmg/CkPmg_FontGlyphCache.h"
#include "CkPmg/CkPmg_Log.h"
#include "CkPmg/CkPmg_Utils_DebugLines.h"

#include "CkCore/Math/Vector/CkVector_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

#include <Engine/FontFace.h>
#include <MaterialDomain.h>
#include <ProceduralMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pmg
{
    // Resolve raw font bytes: explicit override -> bundled Noto asset (Task 7 imports it) ->
    // engine Roboto from disk (always present, keeps the feature working before the bundled
    // CJK font is imported). The returned pointer is only used synchronously by EnsureFace,
    // which copies the bytes immediately.
    auto ResolveTextFontBytes(UFontFace* InOverride) -> const TArray<uint8>*
    {
        if (ck::IsValid(InOverride))
        {
            return &InOverride->GetFontFaceData()->GetData();
        }

        static TStrongObjectPtr<UFontFace> BundledFont;
        if (BundledFont.IsValid() == false)
        {
            if (auto* Loaded = LoadObject<UFontFace>(nullptr,
                TEXT("/CkFoundation/CkPmg/Fonts/NotoSansCJK_Medium.NotoSansCJK_Medium")))
            { BundledFont.Reset(Loaded); }
        }
        if (BundledFont.IsValid())
        {
            return &BundledFont->GetFontFaceData()->GetData();
        }

        static TArray<uint8> RobotoBytes;
        if (RobotoBytes.Num() == 0)
        {
            const FString Path = FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Regular.ttf");
            FFileHelper::LoadFileToArray(RobotoBytes, *Path);
        }
        return RobotoBytes.Num() > 0 ? &RobotoBytes : nullptr;
    }
}

namespace
{
    using namespace ck;

    auto SetupMeshComponent_Text(FCk_Handle InHandle) -> UProceduralMeshComponent*
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Could not get valid World for text entity [{}]"), InHandle)
        { return nullptr; }

        auto MeshComponent = NewObject<UProceduralMeshComponent>(
            World,
            UProceduralMeshComponent::StaticClass(),
            NAME_None,
            RF_Transient);

        CK_ENSURE_IF_NOT(ck::IsValid(MeshComponent), TEXT("Failed to create ProceduralMeshComponent for text entity [{}]"), InHandle)
        { return nullptr; }

        MeshComponent->SetWorldLocation(FVector::ZeroVector);
        MeshComponent->RegisterComponentWithWorld(World);

        if (MeshComponent->HasBegunPlay() == false)
        {
            MeshComponent->BeginPlay();
        }

        MeshComponent->SetVisibility(true);
        MeshComponent->SetHiddenInGame(false);
        MeshComponent->SetCastShadow(true);

        return MeshComponent;
    }

    auto FinalizeMeshComponent_Text(
        UProceduralMeshComponent* InMeshComponent,
        FCk_Handle InHandle,
        const ck::FFragment_Pmg_DebugShape_Common& InCommon,
        ck::FFragment_Pmg_DebugShape_Current& InCurrent,
        float InDeltaT)
        -> void
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InMeshComponent), TEXT("Invalid mesh component"))
        { return; }

        const auto ShouldBeVisible = InCommon.Get_RenderMode() != ECk_Pmg_RenderMode::Hidden;
        InMeshComponent->SetVisibility(ShouldBeVisible, true);
        InMeshComponent->SetHiddenInGame(!ShouldBeVisible);
        InMeshComponent->bHiddenInGame = !ShouldBeVisible;
        InMeshComponent->SetRenderInMainPass(true);
        InMeshComponent->SetRenderInDepthPass(true);

        static auto TranslucentMaterial = LoadObject<UMaterial>(
            nullptr,
            TEXT("/Engine/EngineDebugMaterials/M_SimpleUnlitTranslucent.M_SimpleUnlitTranslucent"));

        if (ck::IsValid(TranslucentMaterial))
        {
            auto DynamicMaterial = UMaterialInstanceDynamic::Create(TranslucentMaterial, InMeshComponent);
            if (ck::IsValid(DynamicMaterial))
            {
                DynamicMaterial->SetVectorParameterValue(FName("Color"), InCommon.Get_Color());
                InMeshComponent->SetMaterial(0, DynamicMaterial);
            }
        }
        else
        {
            ck::pmg::Warning(TEXT("Failed to load M_SimpleUnlitTranslucent for Pmg text DebugShape [{}]"), InHandle);
        }

        InMeshComponent->SetCollisionEnabled(
            InCommon.Get_EnableCollision() ? ECollisionEnabled::QueryAndPhysics : ECollisionEnabled::NoCollision);

        InMeshComponent->UpdateBounds();
        InMeshComponent->MarkRenderStateDirty();

        InCurrent = ck::FFragment_Pmg_DebugShape_Current{TStrongObjectPtr{InMeshComponent}, FCk_Time{InDeltaT}};
        InHandle.Remove<ck::FTag_Pmg_DebugShape_NeedsSetup>();

        if (InHandle.Has<ck::FFragment_Transform>())
        {
            const auto& Transform = InHandle.Get<ck::FFragment_Transform>();
            const auto& CurrentTransform = Transform.Get_Transform();
            InMeshComponent->SetWorldTransform(CurrentTransform);
        }
    }

    struct FPlacedGlyph_Text { const ck::pmg::FCachedGlyph* Glyph; float PenX; float PenY; };

    // Glyphs are authored in the local XY plane (glyph x -> +X right, glyph y -> +Y up, Z=0),
    // matching the magnifying-glass authoring so the filled mesh (ApplyPlaneAxisRotation) and
    // the wireframe (axis quat below) align. Rows descend in -Y.
    auto LayoutText(
        const FString& InText,
        int32 InFaceKey,
        float InSize,
        float InLineSpacing,
        ECk_Pmg_TextAlign InAlign,
        int32 InMaxGlyphs,
        TArray<FPlacedGlyph_Text>& OutPlaced)
        -> void
    {
        auto& Cache = ck::pmg::FFontGlyphCache::Get();
        const float LineH = Cache.Get_LineHeightEm(InFaceKey) * InSize * InLineSpacing;

        TArray<TArray<const ck::pmg::FCachedGlyph*>> Lines; Lines.AddDefaulted();
        TArray<float> LineWidths; LineWidths.Add(0.0f);

        int32 Count = 0;
        for (int32 i = 0; i < InText.Len() && Count < InMaxGlyphs; )
        {
            uint32 Codepoint = static_cast<uint32>(InText[i]);
            if (Codepoint >= 0xD800 && Codepoint <= 0xDBFF && i + 1 < InText.Len())
            {
                const uint32 Lo = static_cast<uint32>(InText[i + 1]);
                if (Lo >= 0xDC00 && Lo <= 0xDFFF)
                { Codepoint = 0x10000 + ((Codepoint - 0xD800) << 10) + (Lo - 0xDC00); ++i; }
            }
            ++i; ++Count;

            if (Codepoint == static_cast<uint32>('\n')) { Lines.AddDefaulted(); LineWidths.Add(0.0f); continue; }

            const auto& G = Cache.GetOrBuildGlyph(InFaceKey, Codepoint);
            Lines.Last().Add(&G);
            LineWidths.Last() += G.AdvanceEm * InSize;
        }

        for (int32 LineIdx = 0; LineIdx < Lines.Num(); ++LineIdx)
        {
            const float W = LineWidths[LineIdx];
            float PenX = InAlign == ECk_Pmg_TextAlign::Center ? -W * 0.5f
                       : InAlign == ECk_Pmg_TextAlign::Right  ? -W : 0.0f;
            const float PenY = -LineIdx * LineH;
            for (const ck::pmg::FCachedGlyph* G : Lines[LineIdx])
            {
                OutPlaced.Add(FPlacedGlyph_Text{G, PenX, PenY});
                PenX += G->AdvanceEm * InSize;
            }
        }
    }
}

namespace ck
{
    auto FProcessor_Pmg_Text_Setup::ForEachEntity(
        TimeType InDeltaT,
        HandleType InHandle,
        const FFragment_Pmg_Text_Params& InParams,
        const FFragment_Pmg_DebugShape_Common& InCommon,
        FFragment_Pmg_DebugShape_Current& InCurrent)
        -> void
    {
        const auto* FontBytes = ck::pmg::ResolveTextFontBytes(InParams.Get_FontOverride().Get());
        if (FontBytes == nullptr || FontBytes->Num() == 0)
        {
            // No font available (e.g. dedicated server). Clear the gate so we don't spin.
            InHandle.Remove<FTag_Pmg_DebugShape_NeedsSetup>();
            return;
        }

        auto& Cache = ck::pmg::FFontGlyphCache::Get();
        const int32 FaceKey = Cache.EnsureFace(*FontBytes);
        if (FaceKey == INDEX_NONE) { InHandle.Remove<FTag_Pmg_DebugShape_NeedsSetup>(); return; }

        TArray<FPlacedGlyph_Text> Placed;
        LayoutText(InParams.Get_Text(), FaceKey, InParams.Get_Size(), InParams.Get_LineSpacing(),
            InParams.Get_Align(), InParams.Get_MaxGlyphs(), Placed);

        // Reuse the existing procmesh on rebuild (SetText); else create a fresh one.
        auto* Mesh = InCurrent._MeshComponent.Get();
        if (ck::IsValid(Mesh)) { Mesh->ClearAllMeshSections(); }
        else
        {
            Mesh = SetupMeshComponent_Text(InHandle);
            if (ck::Is_NOT_Valid(Mesh)) { return; } // ensure already fired; leave NeedsSetup, retry next tick
        }

        // ---- Filled tier (local XY plane, Z=0) ----
        if (InParams.Get_DrawFilled())
        {
            TArray<FVector> Vertices; TArray<int32> Triangles; TArray<FVector> Normals; TArray<FVector2D> UVs;
            for (const FPlacedGlyph_Text& P : Placed)
            {
                if (P.Glyph == nullptr || P.Glyph->TessTris.Num() == 0) { continue; }
                const int32 Base = Vertices.Num();
                for (const FVector2D& V : P.Glyph->TessVerts)
                {
                    Vertices.Add(FVector(P.PenX + V.X * InParams.Get_Size(), P.PenY + V.Y * InParams.Get_Size(), 0.0f));
                    Normals.Add(FVector::ForwardVector);
                    UVs.Add(V);
                }
                for (const FIntVector& T : P.Glyph->TessTris)
                { Triangles.Add(Base + T.X); Triangles.Add(Base + T.Y); Triangles.Add(Base + T.Z); }
            }
            if (Vertices.Num() > 0)
            {
                UCk_Utils_Vector3_UE::ApplyPlaneAxisRotation(Vertices, Normals, InParams.Get_Axis());
                Mesh->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UVs,
                    TArray<FLinearColor>{}, TArray<FProcMeshTangent>{}, true);
            }
        }

        FinalizeMeshComponent_Text(Mesh, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());

        // ---- Wireframe tier ----
        if (InCommon.Get_DrawLines())
        {
            if (InHandle.Has<FFragment_Pmg_DebugShape_Lines>())
            { InHandle.Get<FFragment_Pmg_DebugShape_Lines>()._Lines.Empty(); }

            auto AxisRotation = FQuat::Identity;
            switch (InParams.Get_Axis())
            {
                case ECk_Plane_Axis::XY: AxisRotation = FQuat::Identity; break;
                case ECk_Plane_Axis::XZ: AxisRotation = FQuat(FVector::ForwardVector, PI * 0.5f); break;
                case ECk_Plane_Axis::YZ: AxisRotation = FQuat(FVector::RightVector, -PI * 0.5f); break;
            }

            auto Center = FVector::ZeroVector;
            auto EntityRotation = FQuat::Identity;
            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Xform = InHandle.Get<FFragment_Transform>().Get_Transform();
                Center = Xform.GetLocation();
                EntityRotation = Xform.GetRotation();
            }
            const auto FinalRotation = EntityRotation * AxisRotation;

            auto LineColor = InCommon.Get_Color(); LineColor.A = 1.0f;
            const float Size = InParams.Get_Size();

            for (const FPlacedGlyph_Text& P : Placed)
            {
                if (P.Glyph == nullptr) { continue; }
                for (const TArray<FVector2D>& Contour : P.Glyph->Contours)
                {
                    if (Contour.Num() < 2) { continue; }
                    TArray<FVector> World; World.Reserve(Contour.Num());
                    for (const FVector2D& V : Contour)
                    {
                        const FVector Local(P.PenX + V.X * Size, P.PenY + V.Y * Size, 0.0f);
                        World.Add(Center + FinalRotation.RotateVector(Local));
                    }
                    ck::pmg::Append_DebugPolygon_World(InHandle, World, LineColor, InCommon.Get_LineThickness());
                }
            }
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
