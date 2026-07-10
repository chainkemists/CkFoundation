// `Ck_Vat_DebugVerifyBake` — bakes a transient Mannequin collection and numerically verifies every
// CPU-checkable link of the bone-mode pipeline. Born 2026-07-10 hunting the gym mangling (it
// convicted the MID's platform-data-derived BoneCount/TotalRows — check [E]); kept as the standing
// bake-data verifier: when VAT visuals look wrong, run this FIRST — a full PASS means the bug is
// in the material/VF layer, not the data.
//   [A] collection/texture shape        [B] texture texels vs freshly-sampled pose transforms
//   [C] built-mesh buffers vs source    [D] full GPU-decode reconstruction vs direct CPU skinning
//   [E] render-state MID uniforms vs the collection (dims, texture bindings)
// Every line is prefixed [VAT-VERIFY]; each check ends PASS/FAIL.

#include "CkVatEditor/CkVatBaker.h"
#include "CkVatEditor/CkVat_BakerSubsystem.h"

#include "CkVat/CkVat_Log.h"
#include "CkVat/CkVat_Subsystem.h"
#include "CkVat/Collection/CkVatCollection_Data.h"

#include "CkAnimation/AnimBake/CkAnimBake.h"

#include "CkCore/Validation/CkIsValid.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/Skeleton.h"
#include "Editor.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GPUSkinPublicDefs.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Math/Float16Color.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#include "StaticMeshResources.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_vat_bake_verify
{

struct FExpectedSkin
{
    int32 RenderBones[4] = { 0, 0, 0, 0 };
    float Weights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
    FVector3f Position = FVector3f::ZeroVector;
};

auto Run() -> void
{
    const auto Fail = [](const FString& InWhat) -> void
    {
        ck::vat::Error(TEXT("[VAT-VERIFY] FAIL: {}"), InWhat);
    };
    const auto Pass = [](const FString& InWhat) -> void
    {
        ck::vat::Display(TEXT("[VAT-VERIFY] PASS: {}"), InWhat);
    };

    // ---- load content ----
    auto* Skeleton = LoadObject<USkeleton>(nullptr, TEXT("/CkTests/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"));
    auto* Mesh = LoadObject<USkeletalMesh>(nullptr, TEXT("/CkTests/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    auto* Idle = LoadObject<UAnimSequenceBase>(nullptr, TEXT("/CkTests/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
    if (Skeleton == nullptr || Mesh == nullptr || Idle == nullptr)
    { Fail(TEXT("content load (skeleton/mesh/idle)")); return; }

    // ---- bake ----
    auto* Baker = GEditor->GetEditorSubsystem<UCkVat_BakerSubsystem>();
    if (Baker == nullptr)
    { Fail(TEXT("no baker subsystem")); return; }

    TArray<FCk_VatCollection_ClipDef> Clips;
    Clips.Add(FCk_VatCollection_ClipDef{Idle, FName{TEXT("Idle")}});
    auto* Collection = Baker->CreateAndBake_TransientCollection(Skeleton, Mesh, Clips, 30, ECk_Vat_BakeMode::Bone, ECk_Vat_Precision::High);
    if (Collection == nullptr || NOT Collection->Get_IsBaked())
    { Fail(TEXT("transient bake")); return; }

    // ---- recompute ground truth (same shared core the bake used) ----
    FCk_AnimBake_SampleParams SampleParams;
    const auto SkeletonData = ck::anim_bake::BuildSkeletonData(*Skeleton, *Mesh, SampleParams);
    if (NOT SkeletonData.IsSet())
    { Fail(TEXT("BuildSkeletonData ground truth")); return; }

    TArray<UAnimSequenceBase*> Sequences;
    Sequences.Add(Idle);
    const auto Layout = ck::anim_bake::BuildFrameLayout(Sequences, 30);
    const int32 BoneCount = SkeletonData->RenderBoneCount;
    const int32 TotalRows = Layout.TotalFrameCount;

    // Middle frame of the clip; global row = FrameIndex + local.
    const int32 LocalFrame = Layout.Sequences[0].FrameCount / 2;
    const int32 GlobalRow = Layout.Sequences[0].FrameIndex + LocalFrame;

    TArray<FMatrix44f> ExpectedShaderMatrix; // per render bone, at GlobalRow
    ExpectedShaderMatrix.SetNum(BoneCount);
    ck::anim_bake::SamplePoses(*Skeleton, *SkeletonData, Layout, SampleParams,
        [&](TArrayView<const FTransform> InPose, int32 InGlobalFrame) -> void
        {
            if (InGlobalFrame != GlobalRow)
            { return; }
            for (int32 RenderBone = 0; RenderBone < BoneCount; ++RenderBone)
            {
                const int32 SkelBone = SkeletonData->RenderRequiredBones[RenderBone];
                const FMatrix44f BoneMatrix = static_cast<FTransform3f>(InPose[SkelBone]).ToMatrixWithScale();
                ExpectedShaderMatrix[RenderBone] = SkeletonData->RefPoseInverse[SkelBone] * BoneMatrix;
            }
        });

    // ---- [A] collection shape ----
    {
        auto* PosTex = Collection->Get_BonePositionTexture().Get();
        auto* RotTex = Collection->Get_BoneRotationTexture().Get();
        if (PosTex == nullptr || RotTex == nullptr)
        { Fail(TEXT("[A] baked textures null")); return; }
        const auto Ok = PosTex->Source.GetSizeX() == BoneCount && PosTex->Source.GetSizeY() == TotalRows &&
                        RotTex->Source.GetSizeX() == BoneCount && RotTex->Source.GetSizeY() == TotalRows;
        if (Ok) { Pass(FString::Printf(TEXT("[A] texture shape %dx%d (bones x rows)"), BoneCount, TotalRows)); }
        else
        {
            Fail(FString::Printf(TEXT("[A] texture shape: got %dx%d / %dx%d, expected %dx%d"),
                PosTex->Source.GetSizeX(), PosTex->Source.GetSizeY(), RotTex->Source.GetSizeX(), RotTex->Source.GetSizeY(), BoneCount, TotalRows));
            return;
        }
    }

    // ---- [B] texel row vs expected transforms ----
    TArray<FVector3f> TexelT;
    TArray<FQuat4f> TexelQ;
    TexelT.SetNum(BoneCount);
    TexelQ.SetNum(BoneCount);
    {
        auto* PosTex = Collection->Get_BonePositionTexture().Get();
        auto* RotTex = Collection->Get_BoneRotationTexture().Get();
        TArray64<uint8> PosData;
        TArray64<uint8> RotData;
        if (NOT PosTex->Source.GetMipData(PosData, 0) || NOT RotTex->Source.GetMipData(RotData, 0))
        { Fail(TEXT("[B] texture source mip read")); return; }

        const auto* PosPixels = reinterpret_cast<const FFloat16Color*>(PosData.GetData());
        const auto* RotPixels = reinterpret_cast<const FFloat16Color*>(RotData.GetData());

        auto MaxTErr = 0.0f;
        auto MaxQErr = 0.0f;
        auto WorstBone = 0;
        for (int32 RenderBone = 0; RenderBone < BoneCount; ++RenderBone)
        {
            const auto& PosPx = PosPixels[GlobalRow * BoneCount + RenderBone];
            const auto& RotPx = RotPixels[GlobalRow * BoneCount + RenderBone];
            TexelT[RenderBone] = FVector3f(PosPx.R.GetFloat(), PosPx.G.GetFloat(), PosPx.B.GetFloat());
            auto Quat = FQuat4f(RotPx.R.GetFloat(), RotPx.G.GetFloat(), RotPx.B.GetFloat(), RotPx.A.GetFloat());
            Quat.Normalize();
            TexelQ[RenderBone] = Quat;

            const auto ExpectedT = FVector3f(ExpectedShaderMatrix[RenderBone].GetOrigin());
            auto ExpectedQ = FQuat4f(ExpectedShaderMatrix[RenderBone].GetMatrixWithoutScale());
            ExpectedQ.Normalize();

            const auto TErr = (TexelT[RenderBone] - ExpectedT).Size();
            // q and -q are the same rotation.
            const auto QErr = FMath::Min((TexelQ[RenderBone] - ExpectedQ).Size(), (TexelQ[RenderBone] + ExpectedQ).Size());
            if (TErr > MaxTErr) { MaxTErr = TErr; WorstBone = RenderBone; }
            MaxQErr = FMath::Max(MaxQErr, QErr);
        }

        if (MaxTErr < 0.5f && MaxQErr < 0.05f)
        { Pass(FString::Printf(TEXT("[B] texel row %d matches sampled pose (maxTErr %.4f cm, maxQErr %.4f)"), GlobalRow, MaxTErr, MaxQErr)); }
        else
        { Fail(FString::Printf(TEXT("[B] texel row %d: maxTErr %.4f cm (bone %d), maxQErr %.4f"), GlobalRow, MaxTErr, WorstBone, MaxQErr)); }
    }

    // ---- expected per-vertex skinning from the SOURCE model (mirrors the baker incl. the
    //      single-influence diagnostic if active) ----
    const FSkeletalMeshModel* ImportedModel = Mesh->GetImportedModel();
    if (ImportedModel == nullptr || ImportedModel->LODModels.Num() == 0)
    { Fail(TEXT("no imported model")); return; }
    const FSkeletalMeshLODModel& LODModel = ImportedModel->LODModels[0];

    TMap<FIntVector, FExpectedSkin> ExpectedByQuantizedPos;
    for (int32 SectionIndex = 0; SectionIndex < LODModel.Sections.Num(); ++SectionIndex)
    {
        const FSkelMeshSection& Section = LODModel.Sections[SectionIndex];
        for (const FSoftSkinVertex& Sv : Section.SoftVertices)
        {
            FExpectedSkin Expected;
            Expected.Position = Sv.Position;

            struct FInfluence { int32 BoneMapIndex; uint16 Weight; };
            TArray<FInfluence, TInlineAllocator<MAX_TOTAL_INFLUENCES>> Influences;
            for (int32 i = 0; i < MAX_TOTAL_INFLUENCES; ++i)
            {
                if (Sv.InfluenceWeights[i] == 0) { continue; }
                Influences.Add(FInfluence{Sv.InfluenceBones[i], Sv.InfluenceWeights[i]});
            }
            Influences.Sort([](const FInfluence& A, const FInfluence& B) { return A.Weight > B.Weight; });

            auto TotalWeight = 0.0f;
            const int32 Kept = FMath::Min(Influences.Num(), 4);
            for (int32 i = 0; i < Kept; ++i) { TotalWeight += static_cast<float>(Influences[i].Weight); }
            for (int32 i = 0; i < Kept; ++i)
            {
                const int32 MeshBone = Section.BoneMap.IsValidIndex(Influences[i].BoneMapIndex)
                    ? static_cast<int32>(Section.BoneMap[Influences[i].BoneMapIndex]) : 0;
                const int32 SkelBone = Skeleton->GetSkeletonBoneIndexFromMeshBoneIndex(Mesh, MeshBone);
                const int32 RenderBone = SkelBone != INDEX_NONE && SkeletonData->SkeletonBoneToRenderBone.IsValidIndex(SkelBone)
                    ? SkeletonData->SkeletonBoneToRenderBone[SkelBone] : 0;
                Expected.RenderBones[i] = RenderBone == INDEX_NONE ? 0 : RenderBone;
                Expected.Weights[i] = static_cast<float>(Influences[i].Weight) / TotalWeight;
            }

            const auto Quantized = FIntVector(
                FMath::RoundToInt(Sv.Position.X * 64.0f),
                FMath::RoundToInt(Sv.Position.Y * 64.0f),
                FMath::RoundToInt(Sv.Position.Z * 64.0f));
            ExpectedByQuantizedPos.FindOrAdd(Quantized) = Expected;
        }
    }

    // ---- [C] built-mesh render buffers vs expected ----
    auto* BakedMesh = Collection->Get_BakedMesh().Get();
    if (BakedMesh == nullptr || BakedMesh->GetRenderData() == nullptr || BakedMesh->GetRenderData()->LODResources.Num() == 0)
    { Fail(TEXT("[C] baked mesh render data missing")); return; }
    const FStaticMeshLODResources& LOD = BakedMesh->GetRenderData()->LODResources[0];
    const auto NumRenderVerts = static_cast<int32>(LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices());
    const auto NumUVs = LOD.VertexBuffers.StaticMeshVertexBuffer.GetNumTexCoords();
    const auto NumColors = static_cast<int32>(LOD.VertexBuffers.ColorVertexBuffer.GetNumVertices());

    ck::vat::Display(TEXT("[VAT-VERIFY] [C] render verts [{}] uv channels [{}] color verts [{}]"), NumRenderVerts, NumUVs, NumColors);
    if (NumUVs < 3) { Fail(TEXT("[C] built mesh has fewer than 3 UV channels — lookup channels were dropped by the build")); return; }
    if (NumColors != NumRenderVerts) { Fail(TEXT("[C] color buffer missing/short — weights are not reaching the GPU")); return; }

    auto IndexMismatches = 0;
    auto WeightMismatches = 0;
    auto PosMisses = 0;
    auto Checked = 0;
    auto MaxReconstructionErr = 0.0f;
    auto WorstVert = -1;

    const auto Step = FMath::Max(1, NumRenderVerts / 512); // ~512 samples across the mesh
    for (int32 Vert = 0; Vert < NumRenderVerts; Vert += Step)
    {
        const auto Pos = LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(Vert);
        const auto Quantized = FIntVector(
            FMath::RoundToInt(Pos.X * 64.0f), FMath::RoundToInt(Pos.Y * 64.0f), FMath::RoundToInt(Pos.Z * 64.0f));
        const auto* Expected = ExpectedByQuantizedPos.Find(Quantized);
        if (Expected == nullptr) { ++PosMisses; continue; }
        ++Checked;

        const auto UV1 = LOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Vert, 1);
        const auto UV2 = LOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Vert, 2);
        const float GpuIndices[4] = { UV1.X, UV1.Y, UV2.X, UV2.Y };

        const auto Color = LOD.VertexBuffers.ColorVertexBuffer.VertexColor(Vert);
        // GPU sees raw bytes/255; the baker pre-decoded so this should equal the linear weight.
        const float GpuWeights[4] = {
            Color.R / 255.0f, Color.G / 255.0f, Color.B / 255.0f,
            FMath::Clamp(1.0f - Color.R / 255.0f - Color.G / 255.0f - Color.B / 255.0f, 0.0f, 1.0f) };

        for (int32 i = 0; i < 4; ++i)
        {
            if (FMath::RoundToInt(GpuIndices[i]) != Expected->RenderBones[i] && Expected->Weights[i] > 0.0f)
            { ++IndexMismatches; break; }
        }
        for (int32 i = 0; i < 4; ++i)
        {
            if (FMath::Abs(GpuWeights[i] - Expected->Weights[i]) > 0.02f)
            { ++WeightMismatches; break; }
        }

        // ---- [D] full reconstruction: decode exactly like Vat.ush at GlobalRow ----
        auto GpuSkinned = FVector3f::ZeroVector;
        for (int32 i = 0; i < 4; ++i)
        {
            if (GpuWeights[i] <= 0.0001f) { continue; }
            const auto Bone = FMath::Clamp(FMath::RoundToInt(GpuIndices[i]), 0, BoneCount - 1);
            GpuSkinned += GpuWeights[i] * (TexelQ[Bone].RotateVector(Pos) + TexelT[Bone]);
        }

        auto CpuSkinned = FVector3f::ZeroVector;
        for (int32 i = 0; i < 4; ++i)
        {
            if (Expected->Weights[i] <= 0.0001f) { continue; }
            CpuSkinned += Expected->Weights[i] * ExpectedShaderMatrix[Expected->RenderBones[i]].TransformPosition(Expected->Position);
        }

        const auto Err = (GpuSkinned - CpuSkinned).Size();
        if (Err > MaxReconstructionErr) { MaxReconstructionErr = Err; WorstVert = Vert; }
    }

    ck::vat::Display(TEXT("[VAT-VERIFY] [C] checked [{}] verts (posMisses [{}]): indexMismatches [{}] weightMismatches [{}]"),
        Checked, PosMisses, IndexMismatches, WeightMismatches);
    if (IndexMismatches == 0 && WeightMismatches == 0 && Checked > 100) { Pass(TEXT("[C] mesh data channels")); }
    else { Fail(TEXT("[C] mesh data channels (see counts above)")); }

    if (MaxReconstructionErr < 1.0f && Checked > 100)
    { Pass(FString::Printf(TEXT("[D] GPU-decode reconstruction matches CPU skinning (max %.4f cm)"), MaxReconstructionErr)); }
    else
    { Fail(FString::Printf(TEXT("[D] reconstruction max err %.4f cm at render vert %d"), MaxReconstructionErr, WorstVert)); }

    // ---- [E] render-state MID uniforms ----
    {
        auto* World = GEditor != nullptr ? GEditor->GetEditorWorldContext().World() : nullptr;
        auto* Subsystem = World != nullptr ? World->GetSubsystem<UCk_Vat_Subsystem_UE>() : nullptr;
        if (Subsystem == nullptr)
        { ck::vat::Display(TEXT("[VAT-VERIFY] [E] skipped — no Vat subsystem on the editor world")); return; }

        const auto* RenderState = Subsystem->GetOrCreate_RenderState(Collection);
        if (RenderState == nullptr || NOT ck::IsValid(RenderState->_Mid))
        { Fail(TEXT("[E] no render state / MID")); return; }

        auto* Mid = RenderState->_Mid.Get();
        float MidBoneCount = 0.0f;
        float MidTotalRows = 0.0f;
        float MidFreq = 0.0f;
        float MidDecode = -1.0f;
        Mid->GetScalarParameterValue(FName{TEXT("BoneCount")}, MidBoneCount);
        Mid->GetScalarParameterValue(FName{TEXT("TotalRows")}, MidTotalRows);
        Mid->GetScalarParameterValue(FName{TEXT("SampleFrequency")}, MidFreq);
        Mid->GetScalarParameterValue(FName{TEXT("DecodeNormalized")}, MidDecode);

        UTexture* MidPosTex = nullptr;
        UTexture* MidRotTex = nullptr;
        Mid->GetTextureParameterValue(FName{TEXT("BonePosVat")}, MidPosTex);
        Mid->GetTextureParameterValue(FName{TEXT("BoneRotVat")}, MidRotTex);

        ck::vat::Display(TEXT("[VAT-VERIFY] [E] MID: BoneCount [{}] TotalRows [{}] Freq [{}] Decode [{}] PosTex [{}] RotTex [{}] Master [{}]"),
            MidBoneCount, MidTotalRows, MidFreq, MidDecode, MidPosTex, MidRotTex, Mid->Parent.Get());

        const auto Ok = FMath::RoundToInt(MidBoneCount) == BoneCount &&
                        FMath::RoundToInt(MidTotalRows) == TotalRows &&
                        MidPosTex == Collection->Get_BonePositionTexture().Get() &&
                        MidRotTex == Collection->Get_BoneRotationTexture().Get();
        if (Ok) { Pass(TEXT("[E] MID uniforms match the collection")); }
        else { Fail(TEXT("[E] MID uniforms DO NOT match (values above)")); }
    }
}

}

// --------------------------------------------------------------------------------------------------------------------

static FAutoConsoleCommand GCkVatDebugVerifyBake(
    TEXT("Ck_Vat_DebugVerifyBake"),
    TEXT("TEMP: transient-bakes a Mannequin Bone-mode collection and numerically verifies every CPU-checkable link (texels, mesh channels, reconstruction, MID uniforms)."),
    FConsoleCommandDelegate::CreateStatic(&ck_vat_bake_verify::Run));
