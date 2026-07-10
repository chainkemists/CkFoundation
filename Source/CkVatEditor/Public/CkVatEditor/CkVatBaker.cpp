#include "CkVatBaker.h"

#include "CkVat/Collection/CkVatCollection_Data.h"

#include "CkAnimation/AnimBake/CkAnimBake.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "Animation/AnimSequenceBase.h"
#include "Animation/Skeleton.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "GPUSkinPublicDefs.h"
#include "Materials/MaterialInterface.h"
#include "Math/Float16Color.h"
#include "MeshDescription.h"
#include "Misc/PackageName.h"
#include "Rendering/SkeletalMeshLODModel.h"
#include "Rendering/SkeletalMeshModel.h"
#include "StaticMeshAttributes.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_vat_baker
{
    constexpr int32 MaxInfluences = 4;
    constexpr int32 MaxTextureWidth = 4096;
    constexpr int32 MaxTextureRows = 8192;

    // One source render-vertex, editor-model-sourced, with its influence chain fully resolved.
    struct FSourceVertex
    {
        FVector3f Position;
        FVector3f TangentX;
        FVector3f TangentY;
        FVector4f TangentZ; // W = binormal sign
        FVector2f UV0;
        int32 SectionIndex = 0;
        // Strongest <= 4 influences, renormalized. SkeletonBone drives CPU skinning (vertex mode);
        // RenderBone is what the mesh carries for the shader (bone mode).
        int32 SkeletonBones[MaxInfluences] = { 0, 0, 0, 0 };
        int32 RenderBones[MaxInfluences] = { 0, 0, 0, 0 };
        float Weights[MaxInfluences] = { 0.0f, 0.0f, 0.0f, 0.0f };
    };

    // ---- package ceremony (mirrors CkParticles_TextureGenerator::Bake) ----------------------------------------------

    template <typename T_Asset>
    auto GetOrCreatePackageFor(const FString& InPkgPath, const FString& InAssetName) -> UPackage*
    {
        UPackage* Package = FPackageName::DoesPackageExist(InPkgPath)
            ? LoadPackage(nullptr, *InPkgPath, LOAD_None)
            : nullptr;
        if (Package == nullptr)
        { Package = CreatePackage(*InPkgPath); }
        if (Package == nullptr)
        { return nullptr; }

        if (auto* Old = StaticFindObject(T_Asset::StaticClass(), Package, *InAssetName))
        {
            Old->ClearFlags(RF_Standalone | RF_Public);
            Old->Rename(nullptr, GetTransientPackage(), REN_DontCreateRedirectors | REN_NonTransactional);
        }
        return Package;
    }

    auto SaveAssetPackage(UPackage* InPackage, UObject* InAsset, const FString& InPkgPath) -> bool
    {
        InAsset->MarkPackageDirty();
        FAssetRegistryModule::AssetCreated(InAsset);

        const FString FileName = FPackageName::LongPackageNameToFilename(InPkgPath, FPackageName::GetAssetPackageExtension());
        FSavePackageArgs SaveArgs;
        SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
        return UPackage::SavePackage(InPackage, InAsset, *FileName, SaveArgs);
    }

    // ---- texture encode ----------------------------------------------------------------------------------------------

    // Texel stream in full float; encoded per the collection's precision at save time.
    struct FTexelPlane
    {
        int32 Width = 0;
        int32 Rows = 0;
        TArray<FVector4f> Texels; // [row * Width + column]

        auto Init(int32 InWidth, int32 InRows) -> void
        {
            Width = InWidth;
            Rows = InRows;
            Texels.SetNumZeroed(Width * Rows);
        }
    };

    auto SaveTexture(
        const FString& InCollectionPkgDir,
        const FString& InAssetName,
        const FTexelPlane& InPlane,
        ECk_Vat_Precision InPrecision)
        -> UTexture2D*
    {
        const FString PkgPath = FString::Printf(TEXT("%s/%s"), *InCollectionPkgDir, *InAssetName);
        UPackage* Package = GetOrCreatePackageFor<UTexture2D>(PkgPath, InAssetName);
        CK_ENSURE_IF_NOT(Package != nullptr, TEXT("VatBaker: could not create package [{}]"), PkgPath)
        { return nullptr; }

        auto* Texture = NewObject<UTexture2D>(Package, *InAssetName, RF_Public | RF_Standalone);
        CK_ENSURE_IF_NOT(ck::IsValid(Texture), TEXT("VatBaker: could not create texture [{}]"), InAssetName)
        { return nullptr; }

        Texture->PreEditChange(nullptr);

        if (InPrecision == ECk_Vat_Precision::High)
        {
            // Raw values, half-float source, HDR (RGBA16F, no sRGB) compression.
            TArray<FFloat16Color> Pixels;
            Pixels.SetNumUninitialized(InPlane.Texels.Num());
            for (int32 i = 0; i < InPlane.Texels.Num(); ++i)
            {
                const FVector4f& T = InPlane.Texels[i];
                Pixels[i] = FFloat16Color(FLinearColor(T.X, T.Y, T.Z, T.W));
            }
            Texture->Source.Init(InPlane.Width, InPlane.Rows, /*Slices*/ 1, /*Mips*/ 1, TSF_RGBA16F,
                reinterpret_cast<const uint8*>(Pixels.GetData()));
            Texture->CompressionSettings = TC_HDR;
        }
        else
        {
            // Caller has already normalized texels into [0,1]; quantize to uncompressed RGBA8.
            TArray<FColor> Pixels;
            Pixels.SetNumUninitialized(InPlane.Texels.Num());
            for (int32 i = 0; i < InPlane.Texels.Num(); ++i)
            {
                const FVector4f& T = InPlane.Texels[i];
                constexpr auto Srgb = false;
                Pixels[i] = FLinearColor(T.X, T.Y, T.Z, T.W).ToFColor(Srgb);
            }
            Texture->Source.Init(InPlane.Width, InPlane.Rows, /*Slices*/ 1, /*Mips*/ 1, TSF_BGRA8,
                reinterpret_cast<const uint8*>(Pixels.GetData()));
            Texture->CompressionSettings = TC_VectorDisplacementmap;
        }

        Texture->SRGB = false;
        Texture->MipGenSettings = TMGS_NoMipmaps;
        Texture->Filter = TF_Nearest;
        Texture->NeverStream = true;
        Texture->PostEditChange();
        Texture->UpdateResource();

        if (NOT SaveAssetPackage(Package, Texture, PkgPath))
        { return nullptr; }
        return Texture;
    }

    // ---- source-vertex gathering -------------------------------------------------------------------------------------

    auto GatherSourceVertices(
        USkeleton& InSkeleton,
        USkeletalMesh& InMesh,
        const FCk_AnimBake_SkeletonData& InSkeletonData,
        const FSkeletalMeshLODModel& InLODModel,
        TArray<FSourceVertex>& OutVertices)
        -> bool
    {
        OutVertices.Reserve(InLODModel.NumVertices);

        for (int32 SectionIndex = 0; SectionIndex < InLODModel.Sections.Num(); ++SectionIndex)
        {
            const FSkelMeshSection& Section = InLODModel.Sections[SectionIndex];
            for (const FSoftSkinVertex& Sv : Section.SoftVertices)
            {
                FSourceVertex Out;
                Out.Position = Sv.Position;
                Out.TangentX = Sv.TangentX;
                Out.TangentY = Sv.TangentY;
                Out.TangentZ = Sv.TangentZ;
                Out.UV0 = Sv.UVs[0];
                Out.SectionIndex = SectionIndex;

                // Strongest <= 4 influences (weights are uint16, BoneMap-relative bone indices).
                int32 NonZeroInfluences = 0;
                struct FInfluence { int32 BoneMapIndex; uint16 Weight; };
                TArray<FInfluence, TInlineAllocator<MAX_TOTAL_INFLUENCES>> Influences;
                for (int32 i = 0; i < MAX_TOTAL_INFLUENCES; ++i)
                {
                    if (Sv.InfluenceWeights[i] == 0)
                    { continue; }
                    ++NonZeroInfluences;
                    Influences.Add(FInfluence{Sv.InfluenceBones[i], Sv.InfluenceWeights[i]});
                }
                Influences.Sort([](const FInfluence& A, const FInfluence& B) { return A.Weight > B.Weight; });

                CK_ENSURE_IF_NOT(NonZeroInfluences <= MaxInfluences,
                    TEXT("VatBaker: mesh [{}] has a vertex with {} bone influences; VAT supports {} — strongest {} kept, weights renormalized."),
                    &InMesh, NonZeroInfluences, MaxInfluences, MaxInfluences)
                { } // fallthrough: keep the strongest 4 below

                float TotalWeight = 0.0f;
                const int32 Kept = FMath::Min(Influences.Num(), MaxInfluences);
                for (int32 i = 0; i < Kept; ++i)
                { TotalWeight += static_cast<float>(Influences[i].Weight); }

                for (int32 i = 0; i < Kept; ++i)
                {
                    const int32 MeshBone = Section.BoneMap.IsValidIndex(Influences[i].BoneMapIndex)
                        ? static_cast<int32>(Section.BoneMap[Influences[i].BoneMapIndex])
                        : 0;
                    const int32 SkeletonBone = InSkeleton.GetSkeletonBoneIndexFromMeshBoneIndex(&InMesh, MeshBone);
                    CK_ENSURE_IF_NOT(SkeletonBone != INDEX_NONE,
                        TEXT("VatBaker: mesh bone [{}] of [{}] has no skeleton bone — influence dropped to root"),
                        MeshBone, &InMesh)
                    {
                        Out.SkeletonBones[i] = 0;
                        Out.RenderBones[i] = 0;
                        Out.Weights[i] = static_cast<float>(Influences[i].Weight) / TotalWeight;
                        continue;
                    }

                    const int32 RenderBone = InSkeletonData.SkeletonBoneToRenderBone.IsValidIndex(SkeletonBone)
                        ? InSkeletonData.SkeletonBoneToRenderBone[SkeletonBone]
                        : INDEX_NONE;
                    CK_ENSURE_IF_NOT(RenderBone != INDEX_NONE,
                        TEXT("VatBaker: skeleton bone [{}] is skinned but not in the render-bone set of [{}]"),
                        SkeletonBone, &InMesh)
                    {
                        Out.SkeletonBones[i] = 0;
                        Out.RenderBones[i] = 0;
                        Out.Weights[i] = static_cast<float>(Influences[i].Weight) / TotalWeight;
                        continue;
                    }

                    Out.SkeletonBones[i] = SkeletonBone;
                    Out.RenderBones[i] = RenderBone;
                    Out.Weights[i] = static_cast<float>(Influences[i].Weight) / TotalWeight;
                }

                OutVertices.Add(Out);
            }
        }

        return OutVertices.Num() > 0;
    }

    // ---- static-mesh build -------------------------------------------------------------------------------------------

    auto BuildBakedStaticMesh(
        const UCk_VatCollection_Data& InCollection,
        const FString& InCollectionPkgDir,
        const FString& InAssetName,
        const TArray<FSourceVertex>& InVertices,
        const FSkeletalMeshLODModel& InLODModel,
        USkeletalMesh& InSourceMesh)
        -> UStaticMesh*
    {
        const FString PkgPath = FString::Printf(TEXT("%s/%s"), *InCollectionPkgDir, *InAssetName);
        UPackage* Package = GetOrCreatePackageFor<UStaticMesh>(PkgPath, InAssetName);
        CK_ENSURE_IF_NOT(Package != nullptr, TEXT("VatBaker: could not create package [{}]"), PkgPath)
        { return nullptr; }

        auto* StaticMesh = NewObject<UStaticMesh>(Package, *InAssetName, RF_Public | RF_Standalone);
        CK_ENSURE_IF_NOT(ck::IsValid(StaticMesh), TEXT("VatBaker: could not create static mesh [{}]"), InAssetName)
        { return nullptr; }

        StaticMesh->SetLightingGuid(FGuid::NewGuid());
        StaticMesh->SetNumSourceModels(1);

        // The lookup UVs must survive the build untouched: no recomputed tangent basis, no generated
        // lightmap UVs, full-precision UVs (half-float UVs would quantize the lookup coordinate).
        FStaticMeshSourceModel& SourceModel = StaticMesh->GetSourceModel(0);
        SourceModel.BuildSettings.bRecomputeNormals = false;
        SourceModel.BuildSettings.bRecomputeTangents = false;
        SourceModel.BuildSettings.bGenerateLightmapUVs = false;
        SourceModel.BuildSettings.bRemoveDegenerates = true;
        SourceModel.BuildSettings.bUseFullPrecisionUVs = true;

        FMeshDescription* MeshDescription = StaticMesh->CreateMeshDescription(0);
        CK_ENSURE_IF_NOT(MeshDescription != nullptr, TEXT("VatBaker: CreateMeshDescription failed for [{}]"), InAssetName)
        { return nullptr; }

        FStaticMeshAttributes Attributes(*MeshDescription);
        Attributes.Register();

        const auto BakeMode = InCollection.Get_BakeMode();
        const int32 LookupCh = InCollection.Get_LookupUVChannel();
        const int32 NumUVChannels = BakeMode == ECk_Vat_BakeMode::Vertex ? LookupCh + 1 : LookupCh + 2;
        CK_ENSURE_IF_NOT(NumUVChannels <= MAX_MESH_TEXTURE_COORDS_MD,
            TEXT("VatBaker: [{}] needs {} UV channels (lookup channel {}), max is {}"),
            InAssetName, NumUVChannels, LookupCh, static_cast<int32>(MAX_MESH_TEXTURE_COORDS_MD))
        { return nullptr; }

        Attributes.GetVertexInstanceUVs().SetNumChannels(NumUVChannels);
        MeshDescription->SetNumUVChannels(NumUVChannels);

        const int32 NumVertices = InVertices.Num();
        const float InvTextureWidth = 1.0f / static_cast<float>(NumVertices);

        TVertexAttributesRef<FVector3f> Positions = Attributes.GetVertexPositions();
        TVertexInstanceAttributesRef<FVector3f> Normals = Attributes.GetVertexInstanceNormals();
        TVertexInstanceAttributesRef<FVector3f> Tangents = Attributes.GetVertexInstanceTangents();
        TVertexInstanceAttributesRef<float> BinormalSigns = Attributes.GetVertexInstanceBinormalSigns();
        TVertexInstanceAttributesRef<FVector4f> Colors = Attributes.GetVertexInstanceColors();
        TVertexInstanceAttributesRef<FVector2f> UVs = Attributes.GetVertexInstanceUVs();
        TPolygonGroupAttributesRef<FName> GroupSlotNames = Attributes.GetPolygonGroupMaterialSlotNames();

        // One material slot per source section, copied from the source skeletal mesh so the baked asset
        // is viewable as-authored (Gate 3 swaps in the VAT look MID at runtime).
        const TArray<FSkeletalMaterial>& SourceMaterials = InSourceMesh.GetMaterials();
        TArray<FPolygonGroupID> PolygonGroups;
        TArray<FStaticMaterial> StaticMaterials;
        for (int32 SectionIndex = 0; SectionIndex < InLODModel.Sections.Num(); ++SectionIndex)
        {
            const int32 MaterialIndex = InLODModel.Sections[SectionIndex].MaterialIndex;
            const FName SlotName = *FString::Printf(TEXT("VatSection_%d"), SectionIndex);

            const FPolygonGroupID GroupID = MeshDescription->CreatePolygonGroup();
            GroupSlotNames[GroupID] = SlotName;
            PolygonGroups.Add(GroupID);

            FStaticMaterial Material;
            Material.MaterialInterface = SourceMaterials.IsValidIndex(MaterialIndex)
                ? SourceMaterials[MaterialIndex].MaterialInterface
                : nullptr;
            Material.MaterialSlotName = SlotName;
            Material.ImportedMaterialSlotName = SlotName;
            StaticMaterials.Add(Material);
        }

        TArray<FVertexID> VertexIDs;
        VertexIDs.Reserve(NumVertices);
        for (const FSourceVertex& Sv : InVertices)
        {
            const FVertexID VertexID = MeshDescription->CreateVertex();
            Positions[VertexID] = Sv.Position;
            VertexIDs.Add(VertexID);
        }

        const auto MakeVertexInstance = [&](int32 InFlatVertexIndex) -> FVertexInstanceID
        {
            const FSourceVertex& Sv = InVertices[InFlatVertexIndex];
            const FVertexInstanceID InstanceID = MeshDescription->CreateVertexInstance(VertexIDs[InFlatVertexIndex]);

            Normals[InstanceID] = FVector3f(Sv.TangentZ.X, Sv.TangentZ.Y, Sv.TangentZ.Z);
            Tangents[InstanceID] = Sv.TangentX;
            BinormalSigns[InstanceID] = Sv.TangentZ.W;
            UVs.Set(InstanceID, 0, Sv.UV0);

            if (BakeMode == ECk_Vat_BakeMode::Vertex)
            {
                // U addresses this vertex's texture column; the shader supplies the frame row V.
                UVs.Set(InstanceID, LookupCh, FVector2f((InFlatVertexIndex + 0.5f) * InvTextureWidth, 0.0f));
            }
            else
            {
                // Raw render-bone indices (floats are exact for these magnitudes); weights ride vertex color.
                UVs.Set(InstanceID, LookupCh,
                    FVector2f(static_cast<float>(Sv.RenderBones[0]), static_cast<float>(Sv.RenderBones[1])));
                UVs.Set(InstanceID, LookupCh + 1,
                    FVector2f(static_cast<float>(Sv.RenderBones[2]), static_cast<float>(Sv.RenderBones[3])));
                Colors[InstanceID] = FVector4f(Sv.Weights[0], Sv.Weights[1], Sv.Weights[2], Sv.Weights[3]);
            }

            return InstanceID;
        };

        for (int32 SectionIndex = 0; SectionIndex < InLODModel.Sections.Num(); ++SectionIndex)
        {
            const FSkelMeshSection& Section = InLODModel.Sections[SectionIndex];
            for (uint32 Tri = 0; Tri < Section.NumTriangles; ++Tri)
            {
                TArray<FVertexInstanceID, TInlineAllocator<3>> Corners;
                for (int32 Corner = 0; Corner < 3; ++Corner)
                {
                    const uint32 Index = InLODModel.IndexBuffer[Section.BaseIndex + Tri * 3 + Corner];
                    Corners.Add(MakeVertexInstance(static_cast<int32>(Index)));
                }
                MeshDescription->CreatePolygon(PolygonGroups[SectionIndex], Corners);
            }
        }

        StaticMesh->CommitMeshDescription(0);
        StaticMesh->SetStaticMaterials(StaticMaterials);
        StaticMesh->Build(/*bSilent*/ false);
        StaticMesh->PostEditChange();

        if (NOT SaveAssetPackage(Package, StaticMesh, PkgPath))
        { return nullptr; }
        return StaticMesh;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::vat_editor::
    Bake_VatCollection(
        UCk_VatCollection_Data& InCollection)
    -> bool
{
    using namespace ck_vat_baker;

    // ---- validate inputs ----
    USkeleton* Skeleton = InCollection.Get_Skeleton().Get();
    USkeletalMesh* SourceMesh = InCollection.Get_SourceMesh().Get();

    CK_ENSURE_IF_NOT(ck::IsValid(Skeleton) && ck::IsValid(SourceMesh),
        TEXT("VatBaker: collection [{}] is missing its Skeleton or SourceMesh"), &InCollection)
    { return false; }

    CK_ENSURE_IF_NOT(InCollection.Get_Clips().Num() > 0,
        TEXT("VatBaker: collection [{}] has no clips"), &InCollection)
    { return false; }

    // ---- shared sampling core (Gate 0) ----
    FCk_AnimBake_SampleParams SampleParams;
    const auto SkeletonData = ck::anim_bake::BuildSkeletonData(*Skeleton, *SourceMesh, SampleParams);
    CK_ENSURE_IF_NOT(SkeletonData.IsSet(),
        TEXT("VatBaker: collection [{}] is not bakeable (no skeleton bones, render data, or skinned bones)"), &InCollection)
    { return false; }

    TArray<UAnimSequenceBase*> SequenceAssets;
    SequenceAssets.Reserve(InCollection.Get_Clips().Num());
    for (const FCk_VatCollection_ClipDef& Def : InCollection.Get_Clips())
    { SequenceAssets.Add(Def.Get_Sequence().Get()); }

    const auto Layout = ck::anim_bake::BuildFrameLayout(SequenceAssets, InCollection.Get_SampleFrequency());
    const int32 Rows = Layout.TotalFrameCount;
    CK_ENSURE_IF_NOT(Rows > 1 && Rows <= MaxTextureRows,
        TEXT("VatBaker: collection [{}] bakes to {} frame rows (valid: 2..{}) — reduce clips or sample frequency"),
        &InCollection, Rows, MaxTextureRows)
    { return false; }

    // ---- editor source model (always CPU-side, no bNeedsCPUAccess requirement) ----
    FSkeletalMeshModel* ImportedModel = SourceMesh->GetImportedModel();
    CK_ENSURE_IF_NOT(ImportedModel != nullptr && ImportedModel->LODModels.Num() > 0,
        TEXT("VatBaker: source mesh [{}] has no imported model data"), SourceMesh)
    { return false; }
    const FSkeletalMeshLODModel& LODModel = ImportedModel->LODModels[0];

    TArray<FSourceVertex> SourceVertices;
    CK_ENSURE_IF_NOT(GatherSourceVertices(*Skeleton, *SourceMesh, *SkeletonData, LODModel, SourceVertices),
        TEXT("VatBaker: source mesh [{}] yielded no vertices"), SourceMesh)
    { return false; }

    const auto BakeMode = InCollection.Get_BakeMode();
    const int32 NumVertices = SourceVertices.Num();
    const int32 RenderBoneCount = SkeletonData->RenderBoneCount;

    if (BakeMode == ECk_Vat_BakeMode::Vertex)
    {
        CK_ENSURE_IF_NOT(NumVertices <= MaxTextureWidth,
            TEXT("VatBaker: Vertex mode needs one texture column per vertex; [{}] has {} (max {}). Use Bone mode or a reduced mesh."),
            SourceMesh, NumVertices, MaxTextureWidth)
        { return false; }
    }

    // ---- sample all frames, accumulate raw texel values ----
    const int32 Width = BakeMode == ECk_Vat_BakeMode::Vertex ? NumVertices : RenderBoneCount;

    FTexelPlane PositionPlane;
    FTexelPlane SecondaryPlane; // vertex mode: normals; bone mode: rotation quaternions
    PositionPlane.Init(Width, Rows);
    SecondaryPlane.Init(Width, Rows);

    FBox PositionBounds(ForceInit);

    // Scratch skinning matrices, indexed by skeleton bone (only bones referenced by influences are filled).
    TArray<FMatrix44f> SkinMatrices;
    SkinMatrices.SetNum(SkeletonData->RefPoseInverse.Num());
    TArray<int32> UsedSkeletonBones;
    {
        TSet<int32> Used;
        for (const FSourceVertex& Sv : SourceVertices)
        {
            for (int32 i = 0; i < MaxInfluences; ++i)
            {
                if (Sv.Weights[i] > 0.0f)
                { Used.Add(Sv.SkeletonBones[i]); }
            }
        }
        UsedSkeletonBones = Used.Array();
    }

    const auto PerFramePose = [&](TArrayView<const FTransform> InPoseComponentSpace, int32 InGlobalFrame) -> void
    {
        if (BakeMode == ECk_Vat_BakeMode::Vertex)
        {
            for (const int32 SkelBone : UsedSkeletonBones)
            {
                const FMatrix44f BoneMatrix = static_cast<FTransform3f>(InPoseComponentSpace[SkelBone]).ToMatrixWithScale();
                SkinMatrices[SkelBone] = SkeletonData->RefPoseInverse[SkelBone] * BoneMatrix;
            }

            for (int32 V = 0; V < NumVertices; ++V)
            {
                const FSourceVertex& Sv = SourceVertices[V];

                FVector3f Skinned = FVector3f::ZeroVector;
                FVector3f SkinnedNormal = FVector3f::ZeroVector;
                for (int32 i = 0; i < MaxInfluences; ++i)
                {
                    if (Sv.Weights[i] <= 0.0f)
                    { continue; }
                    const FMatrix44f& M = SkinMatrices[Sv.SkeletonBones[i]];
                    Skinned += Sv.Weights[i] * M.TransformPosition(Sv.Position);
                    SkinnedNormal += Sv.Weights[i] * M.TransformVector(FVector3f(Sv.TangentZ.X, Sv.TangentZ.Y, Sv.TangentZ.Z));
                }

                const FVector3f Offset = Skinned - Sv.Position;
                PositionBounds += FVector(Offset);
                PositionPlane.Texels[InGlobalFrame * Width + V] = FVector4f(Offset.X, Offset.Y, Offset.Z, 1.0f);

                const FVector3f N = SkinnedNormal.GetSafeNormal() * 0.5f + FVector3f(0.5f, 0.5f, 0.5f);
                SecondaryPlane.Texels[InGlobalFrame * Width + V] = FVector4f(N.X, N.Y, N.Z, 1.0f);
            }
        }
        else
        {
            for (int32 RenderBone = 0; RenderBone < RenderBoneCount; ++RenderBone)
            {
                const int32 SkelBone = SkeletonData->RenderRequiredBones[RenderBone];
                const FMatrix44f BoneMatrix = static_cast<FTransform3f>(InPoseComponentSpace[SkelBone]).ToMatrixWithScale();
                const FMatrix44f ShaderMatrix = SkeletonData->RefPoseInverse[SkelBone] * BoneMatrix;

                const FVector3f Translation = FVector3f(ShaderMatrix.GetOrigin());
                PositionBounds += FVector(Translation);
                PositionPlane.Texels[InGlobalFrame * Width + RenderBone] =
                    FVector4f(Translation.X, Translation.Y, Translation.Z, 1.0f);

                FQuat4f Rotation = FQuat4f(ShaderMatrix.GetMatrixWithoutScale());
                Rotation.Normalize();
                SecondaryPlane.Texels[InGlobalFrame * Width + RenderBone] =
                    FVector4f(Rotation.X, Rotation.Y, Rotation.Z, Rotation.W);
            }
        }
    };

    const FBox BoneBoundsAllFrames =
        ck::anim_bake::SamplePoses(*Skeleton, *SkeletonData, Layout, SampleParams, PerFramePose);

    // ---- precision-normalize where the format requires it (layout contract: Gate_01_Bake.md) ----
    if (InCollection.Get_Precision() == ECk_Vat_Precision::Low)
    {
        const FVector3f Min = FVector3f(PositionBounds.Min);
        const FVector3f Extent = FVector3f(PositionBounds.Max) - Min;
        const FVector3f SafeExtent = FVector3f(
            FMath::Max(Extent.X, UE_KINDA_SMALL_NUMBER),
            FMath::Max(Extent.Y, UE_KINDA_SMALL_NUMBER),
            FMath::Max(Extent.Z, UE_KINDA_SMALL_NUMBER));

        for (FVector4f& Texel : PositionPlane.Texels)
        {
            Texel.X = (Texel.X - Min.X) / SafeExtent.X;
            Texel.Y = (Texel.Y - Min.Y) / SafeExtent.Y;
            Texel.Z = (Texel.Z - Min.Z) / SafeExtent.Z;
        }

        if (BakeMode == ECk_Vat_BakeMode::Bone)
        {
            for (FVector4f& Texel : SecondaryPlane.Texels)
            { Texel = Texel * 0.5f + FVector4f(0.5f, 0.5f, 0.5f, 0.5f); }
        }
    }

    // ---- write the assets, siblings of the collection ----
    const FString CollectionPkgDir = FPackageName::GetLongPackagePath(InCollection.GetPackage()->GetName());
    const FString BaseName = InCollection.GetName();

    UTexture2D* PositionTexture = SaveTexture(CollectionPkgDir,
        FString::Printf(TEXT("%s_%s"), *BaseName, BakeMode == ECk_Vat_BakeMode::Vertex ? TEXT("Pos") : TEXT("BonePos")),
        PositionPlane, InCollection.Get_Precision());
    CK_ENSURE_IF_NOT(ck::IsValid(PositionTexture), TEXT("VatBaker: position texture bake failed for [{}]"), &InCollection)
    { return false; }

    UTexture2D* SecondaryTexture = SaveTexture(CollectionPkgDir,
        FString::Printf(TEXT("%s_%s"), *BaseName, BakeMode == ECk_Vat_BakeMode::Vertex ? TEXT("Nrm") : TEXT("BoneRot")),
        SecondaryPlane, InCollection.Get_Precision());
    CK_ENSURE_IF_NOT(ck::IsValid(SecondaryTexture), TEXT("VatBaker: secondary texture bake failed for [{}]"), &InCollection)
    { return false; }

    UStaticMesh* BakedMesh = BuildBakedStaticMesh(InCollection, CollectionPkgDir,
        FString::Printf(TEXT("%s_Mesh"), *BaseName), SourceVertices, LODModel, *SourceMesh);
    CK_ENSURE_IF_NOT(ck::IsValid(BakedMesh), TEXT("VatBaker: static-mesh build failed for [{}]"), &InCollection)
    { return false; }

    // ---- write back the serialized bake results ----
    UCk_VatCollection_Data::FCk_Vat_BakeResults Results;
    Results.BakedMesh = BakedMesh;
    if (BakeMode == ECk_Vat_BakeMode::Vertex)
    {
        Results.PositionTexture = PositionTexture;
        Results.NormalTexture = SecondaryTexture;
    }
    else
    {
        Results.BonePositionTexture = PositionTexture;
        Results.BoneRotationTexture = SecondaryTexture;
    }

    for (int32 ClipIndex = 0; ClipIndex < InCollection.Get_Clips().Num(); ++ClipIndex)
    {
        const FCk_AnimBake_SequenceLayout& SeqLayout = Layout.Sequences[ClipIndex];
        const UAnimSequenceBase* Seq = SeqLayout.Sequence.Get();
        Results.BakedClips.Emplace(
            InCollection.Get_Clips()[ClipIndex].Get_Name(),
            SeqLayout.FrameIndex,
            SeqLayout.FrameCount,
            SeqLayout.SampleFrequency,
            FCk_Time{ck::IsValid(Seq) ? Seq->GetPlayLength() : 0.0f});
    }

    Results.AnimatedBounds = ck::anim_bake::ComputeAnimatedBounds(*SkeletonData, BoneBoundsAllFrames, *SourceMesh);
    Results.PositionBoundsMin = PositionBounds.Min;
    Results.PositionBoundsMax = PositionBounds.Max;

    InCollection.ApplyBakeResults(Results);

    // The collection is a pre-existing asset (no AssetCreated) — just persist the bake write-back.
    const FString CollectionPkgPath = InCollection.GetPackage()->GetName();
    const FString CollectionFileName =
        FPackageName::LongPackageNameToFilename(CollectionPkgPath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    return UPackage::SavePackage(InCollection.GetPackage(), &InCollection, *CollectionFileName, SaveArgs);
}
