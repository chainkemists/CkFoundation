#include "CkPmg_Processor_TextShapes.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkPmg/CkPmg_FontGlyphCache.h"
#include "CkPmg/CkPmg_Log.h"
#include "CkPmg/CkPmg_Utils_DebugLines.h"

#include "CkCore/Math/Vector/CkVector_Utils.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"

#include "Interfaces/IPluginManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/StrongObjectPtr.h"

#include <Engine/FontFace.h>
#include <MaterialDomain.h>
#include <ProceduralMeshComponent.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::pmg
{
    // Resolve raw font bytes: explicit override -> bundled Noto asset ->
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

    namespace
    {
        // Loads raw font bytes from a file under the CkFoundation plugin's CkPmg Resources dir.
        // Tried once (the tried flag avoids re-hitting the filesystem every frame when absent).
        const TArray<uint8>* LoadResourceFontBytes_Cached(const TCHAR* InFileName, TArray<uint8>& InOutBytes, bool& InOutTried)
        {
            if (InOutTried == false)
            {
                InOutTried = true;
                if (const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("CkFoundation")))
                {
                    const FString Path = Plugin->GetBaseDir() / TEXT("Source/CkPmg/Resources") / InFileName;
                    FFileHelper::LoadFileToArray(InOutBytes, *Path);
                }
            }
            return InOutBytes.Num() > 0 ? &InOutBytes : nullptr;
        }
    }

    // Ordered glyph-fallback chain: primary text font, then bundled Noto Emoji, then Noto Sans Symbols 2.
    // The fallbacks ship as raw .ttf under Source/CkPmg/Resources (staged for packaged builds).
    auto ResolveTextFontChain(UFontFace* InOverride, TArray<const TArray<uint8>*>& OutChain) -> void
    {
        if (const auto* Primary = ResolveTextFontBytes(InOverride)) { OutChain.Add(Primary); }

        static TArray<uint8> EmojiBytes;   static bool EmojiTried = false;
        if (const auto* Emoji = LoadResourceFontBytes_Cached(TEXT("NotoEmoji-Medium.ttf"), EmojiBytes, EmojiTried))
        { OutChain.Add(Emoji); }

        static TArray<uint8> SymbolsBytes; static bool SymbolsTried = false;
        if (const auto* Symbols = LoadResourceFontBytes_Cached(TEXT("NotoSansSymbols2-Regular.ttf"), SymbolsBytes, SymbolsTried))
        { OutChain.Add(Symbols); }
    }
}

namespace
{
    auto SetupMeshComponent_Text(FCk_Handle InHandle) -> UProceduralMeshComponent*
    {
        const auto World = UCk_Utils_EntityLifetime_UE::Get_WorldForEntity(InHandle);
        CK_ENSURE_IF_NOT(ck::IsValid(World), TEXT("Could not get valid World for text entity [{}]"), InHandle)
        { return nullptr; }

        auto MeshComponent = UCk_Utils_Object_UE::Request_CreateNewObject<UProceduralMeshComponent>(
            World, UProceduralMeshComponent::StaticClass(), nullptr,
            FCk_ObjectPooling_PoolParams{}.Set_RecyclePolicy(ECk_ObjectPooling_RecyclePolicy::DestroyOnRelease), nullptr);

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

        InCurrent = ck::FFragment_Pmg_DebugShape_Current{InMeshComponent, FCk_Time{InDeltaT}};
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
        const TArray<int32>& InFaceChain,
        float InSize,
        float InLineSpacing,
        ECk_Pmg_TextAlign InAlign,
        int32 InMaxGlyphs,
        TArray<FPlacedGlyph_Text>& OutPlaced)
        -> void
    {
        auto& Cache = ck::pmg::FFontGlyphCache::Get();
        const int32 PrimaryFace = InFaceChain[0]; // chain is guaranteed non-empty by the caller
        const float LineH = Cache.Get_LineHeightEm(PrimaryFace) * InSize * InLineSpacing;

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

            // Font fallback: first face in the chain that has this codepoint; else the primary (.notdef).
            int32 ChosenFace = PrimaryFace;
            for (int32 Fk : InFaceChain)
            {
                if (Cache.FaceHasCodepoint(Fk, Codepoint)) { ChosenFace = Fk; break; }
            }

            const auto& G = Cache.GetOrBuildGlyph(ChosenFace, Codepoint);
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
        // Ensure a procmesh exists FIRST (reuse on rebuild) so Current is always valid once
        // NeedsSetup is cleared. Otherwise a no-font early-out would clear NeedsSetup while leaving
        // an invalid mesh, which FProcessor_Pmg_DebugShape_UpdateTransform then ensures on.
        auto* Mesh = InCurrent._MeshComponent.Get();
        if (ck::Is_NOT_Valid(Mesh))
        {
            Mesh = SetupMeshComponent_Text(InHandle);
            if (ck::Is_NOT_Valid(Mesh)) { return; } // no world yet; keep NeedsSetup, retry next tick
        }
        Mesh->ClearAllMeshSections();

        TArray<const TArray<uint8>*> FontChain;
        ck::pmg::ResolveTextFontChain(InParams.Get_FontOverride().Get(), FontChain);

        auto& Cache = ck::pmg::FFontGlyphCache::Get();
        TArray<int32> FaceChain;
        for (const TArray<uint8>* Bytes : FontChain)
        {
            if (Bytes == nullptr || Bytes->Num() == 0) { continue; }
            const int32 Fk = Cache.EnsureFace(*Bytes);
            if (Fk != INDEX_NONE) { FaceChain.Add(Fk); }
        }
        if (FaceChain.Num() == 0)
        {
            // No usable font (e.g. dedicated server): leave the shape inert with a valid empty mesh.
            FinalizeMeshComponent_Text(Mesh, InHandle, InCommon, InCurrent, InDeltaT.Get_Seconds());
            return;
        }

        TArray<FPlacedGlyph_Text> Placed;
        LayoutText(InParams.Get_Text(), FaceChain, InParams.Get_Size(), InParams.Get_LineSpacing(),
            InParams.Get_Align(), InParams.Get_MaxGlyphs(), Placed);

        // Orientation applied to BOTH the filled mesh and the wireframe so they stay aligned.
        // Keyed on _Axis (default YZ = upright, facing the play camera; see Get_PlaneAxisRotation).
        const FQuat OrientQuat = UCk_Utils_Vector3_UE::Get_PlaneAxisRotation(InParams.Get_Axis());

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

            // Double-sided fill: the constrained-Delaunay winding isn't guaranteed to face the
            // camera and the glyph quad is flat, so add reversed triangles — otherwise the fill is
            // backface-culled and only the wireframe outline shows.
            const int32 ForwardTriCount = Triangles.Num();
            for (int32 t = 0; t + 2 < ForwardTriCount; t += 3)
            {
                // Copy to locals first — Triangles.Add(Triangles[i]) aliases into the array and
                // asserts when the Add reallocates (Array.h "adding element from same array").
                const int32 I0 = Triangles[t];
                const int32 I1 = Triangles[t + 1];
                const int32 I2 = Triangles[t + 2];
                Triangles.Add(I0);
                Triangles.Add(I2);
                Triangles.Add(I1);
            }

            if (Vertices.Num() > 0)
            {
                for (FVector& V : Vertices) { V = OrientQuat.RotateVector(V); }
                for (FVector& N : Normals)  { N = OrientQuat.RotateVector(N); }
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

            auto Center = FVector::ZeroVector;
            auto EntityRotation = FQuat::Identity;
            if (InHandle.Has<FFragment_Transform>())
            {
                const auto& Xform = InHandle.Get<FFragment_Transform>().Get_Transform();
                Center = Xform.GetLocation();
                EntityRotation = Xform.GetRotation();
            }
            const auto FinalRotation = EntityRotation * OrientQuat;

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
