#include "CkVatBaker.h"

#include "CkVat/CkVat_Log.h"
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
    // Hard slot count of the per-vertex carriers (4 indices + 4 weights). The KEPT count within these
    // slots is the collection's _BoneInfluences (bone mode).
    constexpr int32 MaxInfluences = 4;

    // SaveAssets: sibling packages of the collection, written to disk. Transient: outered to the transient
    // package, nothing written — results die with the session. The compute is identical.
    enum class EPersistence
    {
        SaveAssets,
        Transient
    };

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
        ECk_Vat_Precision InPrecision,
        EPersistence InPersistence)
        -> UTexture2D*
    {
        auto Package = static_cast<UPackage*>(nullptr);
        auto PkgPath = FString{};
        auto TextureName = FName{*InAssetName};
        auto ObjectFlags = RF_Public | RF_Standalone;

        if (InPersistence == EPersistence::SaveAssets)
        {
            PkgPath = FString::Printf(TEXT("%s/%s"), *InCollectionPkgDir, *InAssetName);
            Package = GetOrCreatePackageFor<UTexture2D>(PkgPath, InAssetName);
            CK_ENSURE_IF_NOT(Package != nullptr, TEXT("VatBaker: could not create package [{}]"), PkgPath)
            { return nullptr; }
        }
        else
        {
            Package = GetTransientPackage();
            TextureName = MakeUniqueObjectName(Package, UTexture2D::StaticClass(), FName{*InAssetName});
            ObjectFlags = RF_Transient;
        }

        auto* Texture = NewObject<UTexture2D>(Package, TextureName, ObjectFlags);
        CK_ENSURE_IF_NOT(ck::IsValid(Texture), TEXT("VatBaker: could not create texture [{}]"), InAssetName)
        { return nullptr; }

        Texture->PreEditChange(nullptr);

        if (InPrecision == ECk_Vat_Precision::Ultra)
        {
            TArray<FLinearColor> Pixels;
            Pixels.SetNumUninitialized(InPlane.Texels.Num());
            for (int32 i = 0; i < InPlane.Texels.Num(); ++i)
            {
                const FVector4f& T = InPlane.Texels[i];
                Pixels[i] = FLinearColor(T.X, T.Y, T.Z, T.W);
            }
            Texture->Source.Init(InPlane.Width, InPlane.Rows, /*Slices*/ 1, /*Mips*/ 1, TSF_RGBA32F,
                reinterpret_cast<const uint8*>(Pixels.GetData()));
            Texture->CompressionSettings = TC_HDR_F32;
        }
        else if (InPrecision == ECk_Vat_Precision::High)
        {
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

        if (InPersistence == EPersistence::SaveAssets)
        {
            if (NOT SaveAssetPackage(Package, Texture, PkgPath))
            { return nullptr; }
        }
        return Texture;
    }

    // ---- source-vertex gathering -------------------------------------------------------------------------------------

    auto GatherSourceVertices(
        USkeleton& InSkeleton,
        USkeletalMesh& InMesh,
        const FCk_AnimBake_SkeletonData& InSkeletonData,
        const FSkeletalMeshLODModel& InLODModel,
        int32 InKeptInfluences,
        TArray<FSourceVertex>& OutVertices)
        -> bool
    {
        const int32 KeptInfluences = FMath::Clamp(InKeptInfluences, 1, MaxInfluences);
        OutVertices.Reserve(InLODModel.NumVertices);

        // Keep-strongest-N is expected behavior (standard Mannequin content carries 5-influence vertices),
        // so this is summarized ONCE per bake below rather than ensured per vertex.
        int32 OverInfluencedVertexCount = 0;
        int32 MostInfluencesSeen = 0;

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

                if (NonZeroInfluences > KeptInfluences)
                {
                    ++OverInfluencedVertexCount;
                    MostInfluencesSeen = FMath::Max(MostInfluencesSeen, NonZeroInfluences);
                }

                float TotalWeight = 0.0f;
                const int32 Kept = FMath::Min(Influences.Num(), KeptInfluences);
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

        // Display, not Warning: the automation harness escalates warnings to test failures.
        if (OverInfluencedVertexCount > 0)
        {
            ck::vat::Display(TEXT("VatBaker: mesh [{}] has [{}] vertices with more than [{}] bone influences (most seen: [{}]) — strongest [{}] kept per vertex, weights renormalized."),
                &InMesh, OverInfluencedVertexCount, KeptInfluences, MostInfluencesSeen, KeptInfluences);
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
        USkeletalMesh& InSourceMesh,
        int32 InWeightTexWidth,
        int32 InWeightTexRows,
        EPersistence InPersistence)
        -> UStaticMesh*
    {
        auto Package = static_cast<UPackage*>(nullptr);
        auto PkgPath = FString{};
        auto MeshName = FName{*InAssetName};
        auto ObjectFlags = RF_Public | RF_Standalone;

        if (InPersistence == EPersistence::SaveAssets)
        {
            PkgPath = FString::Printf(TEXT("%s/%s"), *InCollectionPkgDir, *InAssetName);
            Package = GetOrCreatePackageFor<UStaticMesh>(PkgPath, InAssetName);
            CK_ENSURE_IF_NOT(Package != nullptr, TEXT("VatBaker: could not create package [{}]"), PkgPath)
            { return nullptr; }
        }
        else
        {
            Package = GetTransientPackage();
            MeshName = MakeUniqueObjectName(Package, UStaticMesh::StaticClass(), FName{*InAssetName});
            ObjectFlags = RF_Transient;
        }

        auto* StaticMesh = NewObject<UStaticMesh>(Package, MeshName, ObjectFlags);
        CK_ENSURE_IF_NOT(ck::IsValid(StaticMesh), TEXT("VatBaker: could not create static mesh [{}]"), InAssetName)
        { return nullptr; }

        StaticMesh->SetLightingGuid(FGuid::NewGuid());
        StaticMesh->SetNumSourceModels(1);

        // The lookup UVs must survive the build untouched — half-float UVs would quantize the coordinate.
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

        const auto BakeMode = InCollection.Get_BakeSettings().Get_BakeMode();
        const auto WeightStorage = InCollection.Get_BakeSettings().Get_BoneWeightStorage();
        const auto UsesWeightTexture = BakeMode == ECk_Vat_BakeMode::Bone &&
            WeightStorage == ECk_Vat_BoneWeightStorage::WeightTexture;
        const int32 LookupCh = InCollection.Get_BakeSettings().Get_LookupUVChannel();
        // Vertex mode and weight-texture storage carry ONE lookup UV; mesh-channel bone storage carries two.
        const int32 NumUVChannels = BakeMode == ECk_Vat_BakeMode::Vertex || UsesWeightTexture
            ? LookupCh + 1
            : LookupCh + 2;
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

        // Copied from the source mesh so the baked asset is viewable as-authored; the VAT look MID swaps in at runtime.
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
            else if (UsesWeightTexture)
            {
                // Full UV of this vertex's texel in the index/weight textures (row-major layout).
                const int32 Col = InFlatVertexIndex % InWeightTexWidth;
                const int32 Row = InFlatVertexIndex / InWeightTexWidth;
                UVs.Set(InstanceID, LookupCh, FVector2f(
                    (Col + 0.5f) / static_cast<float>(InWeightTexWidth),
                    (Row + 0.5f) / static_cast<float>(InWeightTexRows)));
            }
            else
            {
                // Raw render-bone indices (floats are exact for these magnitudes); weights ride vertex color.
                UVs.Set(InstanceID, LookupCh,
                    FVector2f(static_cast<float>(Sv.RenderBones[0]), static_cast<float>(Sv.RenderBones[1])));
                UVs.Set(InstanceID, LookupCh + 1,
                    FVector2f(static_cast<float>(Sv.RenderBones[2]), static_cast<float>(Sv.RenderBones[3])));

                // The static-mesh build sRGB-ENCODES vertex-color RGB and the GPU hands the shader the raw
                // encoded bytes, so weights are pre-DECODED here and the build's encode cancels to identity.
                // Without it every BLENDED weight warps upward while pure 0/1 weights stay correct ("feet fine,
                // torso explodes"). Alpha is not encoded by the build, so it passes through raw.
                const auto SrgbToLinear = [](float InEncoded) -> float
                {
                    return InEncoded <= 0.04045f
                        ? InEncoded / 12.92f
                        : FMath::Pow((InEncoded + 0.055f) / 1.055f, 2.4f);
                };
                Colors[InstanceID] = FVector4f(
                    SrgbToLinear(Sv.Weights[0]),
                    SrgbToLinear(Sv.Weights[1]),
                    SrgbToLinear(Sv.Weights[2]),
                    Sv.Weights[3]);
            }

            return InstanceID;
        };

        // Hoisted so the inner triangle loop needs no ensures: a well-formed model bounds its indices by
        // NumVertices, so these two checks cover every MakeVertexInstance call below.
        CK_ENSURE_IF_NOT(InLODModel.NumVertices == static_cast<uint32>(InVertices.Num()),
            TEXT("VatBaker: [{}] LODModel NumVertices [{}] != gathered vertices [{}]"),
            &InSourceMesh, InLODModel.NumVertices, InVertices.Num())
        { return nullptr; }

        for (int32 SectionIndex = 0; SectionIndex < InLODModel.Sections.Num(); ++SectionIndex)
        {
            const FSkelMeshSection& Section = InLODModel.Sections[SectionIndex];
            CK_ENSURE_IF_NOT(Section.BaseIndex + Section.NumTriangles * 3 <= static_cast<uint32>(InLODModel.IndexBuffer.Num()),
                TEXT("VatBaker: [{}] section [{}] index range exceeds index buffer [{}]"),
                &InSourceMesh, SectionIndex, InLODModel.IndexBuffer.Num())
            { return nullptr; }
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

        if (InPersistence == EPersistence::SaveAssets)
        {
            if (NOT SaveAssetPackage(Package, StaticMesh, PkgPath))
            { return nullptr; }
        }
        return StaticMesh;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck_vat_baker
{

auto
    DoBake(
        UCk_VatCollection_Data& InCollection,
        EPersistence InPersistence)
    -> bool
{
    // ---- validate inputs ----
    USkeleton* Skeleton = InCollection.Get_Skeleton().Get();
    USkeletalMesh* SourceMesh = InCollection.Get_SourceMesh().Get();

    CK_ENSURE_IF_NOT(ck::IsValid(Skeleton) && ck::IsValid(SourceMesh),
        TEXT("VatBaker: collection [{}] is missing its Skeleton or SourceMesh"), &InCollection)
    { return false; }

    CK_ENSURE_IF_NOT(InCollection.Get_Clips().Num() > 0,
        TEXT("VatBaker: collection [{}] has no clips"), &InCollection)
    { return false; }

    // ---- shared sampling core ----
    FCk_AnimBake_SampleParams SampleParams;
    SampleParams.ExtractRootMotion = InCollection.Get_BakeSettings().Get_ExtractRootMotion();
    SampleParams.DisableRetargeting = InCollection.Get_BakeSettings().Get_DisableRetargeting();
    const auto SkeletonData = ck::anim_bake::BuildSkeletonData(*Skeleton, *SourceMesh, SampleParams);
    CK_ENSURE_IF_NOT(SkeletonData.IsSet(),
        TEXT("VatBaker: collection [{}] is not bakeable (no skeleton bones, render data, or skinned bones)"), &InCollection)
    { return false; }

    // A mesh-bind inverse override (GetRefBasesInvMatrix) does NOT belong here: the skeleton-chain
    // inverse is correct for this content. Do not re-add without clean-build evidence.

    TArray<UAnimSequenceBase*> SequenceAssets;
    SequenceAssets.Reserve(InCollection.Get_Clips().Num());
    for (const FCk_VatCollection_ClipDef& Def : InCollection.Get_Clips())
    {
        CK_ENSURE_IF_NOT(ck::IsValid(Def.Get_Sequence().Get()),
            TEXT("VatBaker: collection [{}] clip [{}] has no Sequence set — the bake would serialize a dead clip"),
            &InCollection, Def.Get_Name())
        { return false; }
        SequenceAssets.Add(Def.Get_Sequence().Get());
    }

    const int32 MaxTextureWidth = InCollection.Get_BakeSettings().Get_MaxTextureWidth();
    const int32 MaxTextureRows = InCollection.Get_BakeSettings().Get_MaxTextureRows();

    const auto Layout = ck::anim_bake::BuildFrameLayout(SequenceAssets, InCollection.Get_BakeSettings().Get_SampleFrequency());
    const int32 Rows = Layout.TotalFrameCount;
    CK_ENSURE_IF_NOT(Rows > 1 && Rows <= MaxTextureRows,
        TEXT("VatBaker: collection [{}] bakes to {} frame rows (valid: 2..{}) — reduce clips or sample frequency, or raise _MaxTextureRows"),
        &InCollection, Rows, MaxTextureRows)
    { return false; }

    // ---- editor source model (always CPU-side, no bNeedsCPUAccess requirement) ----
    FSkeletalMeshModel* ImportedModel = SourceMesh->GetImportedModel();
    const int32 SourceLOD = InCollection.Get_BakeSettings().Get_SourceLOD();
    CK_ENSURE_IF_NOT(ImportedModel != nullptr && ImportedModel->LODModels.IsValidIndex(SourceLOD),
        TEXT("VatBaker: source mesh [{}] has no imported model data for _SourceLOD [{}] ([{}] LODs)"),
        SourceMesh, SourceLOD, ImportedModel != nullptr ? ImportedModel->LODModels.Num() : 0)
    { return false; }
    const FSkeletalMeshLODModel& LODModel = ImportedModel->LODModels[SourceLOD];

    const auto BakeMode = InCollection.Get_BakeSettings().Get_BakeMode();
    const int32 KeptInfluences = [&]() -> int32
    {
        if (BakeMode != ECk_Vat_BakeMode::Bone)
        { return MaxInfluences; }
        switch (InCollection.Get_BakeSettings().Get_BoneInfluences())
        {
            case ECk_Vat_BoneInfluences::One: return 1;
            case ECk_Vat_BoneInfluences::Two: return 2;
            default: return 4;
        }
    }();

    TArray<FSourceVertex> SourceVertices;
    CK_ENSURE_IF_NOT(GatherSourceVertices(*Skeleton, *SourceMesh, *SkeletonData, LODModel, KeptInfluences, SourceVertices),
        TEXT("VatBaker: source mesh [{}] yielded no vertices"), SourceMesh)
    { return false; }

    const int32 NumVertices = SourceVertices.Num();
    const int32 RenderBoneCount = SkeletonData->RenderBoneCount;

    if (BakeMode == ECk_Vat_BakeMode::Vertex)
    {
        CK_ENSURE_IF_NOT(NumVertices <= MaxTextureWidth,
            TEXT("VatBaker: Vertex mode needs one texture column per vertex; [{}] has {} (max {}). Use Bone mode, a higher _SourceLOD, or a reduced mesh."),
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

                // Encode in the BIND-POSE TANGENT frame: tangent-space is invariant under the per-instance
                // transform, and the pixel shader has no per-instance basis to transform with (VS-only outside
                // Nanite). Binormal is RECONSTRUCTED exactly as the engine rebuilds it at render time.
                const FVector3f SkinnedN = SkinnedNormal.GetSafeNormal();
                const FVector3f BindT = Sv.TangentX;
                const FVector3f BindN = FVector3f(Sv.TangentZ.X, Sv.TangentZ.Y, Sv.TangentZ.Z);
                const FVector3f BindB = FVector3f::CrossProduct(BindN, BindT) * (Sv.TangentZ.W < 0.0f ? -1.0f : 1.0f);
                const FVector3f Nts = FVector3f(
                    FVector3f::DotProduct(SkinnedN, BindT),
                    FVector3f::DotProduct(SkinnedN, BindB),
                    FVector3f::DotProduct(SkinnedN, BindN));

                const FVector3f N = Nts * 0.5f + FVector3f(0.5f, 0.5f, 0.5f);
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

    // ---- precision-normalize where the format requires it ----
    if (InCollection.Get_BakeSettings().Get_Precision() == ECk_Vat_Precision::Low)
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

    // ---- write the outputs (SaveAssets: siblings of the collection; Transient: transient package) ----
    const FString CollectionPkgDir = InPersistence == EPersistence::SaveAssets
        ? FPackageName::GetLongPackagePath(InCollection.GetPackage()->GetName())
        : FString{};
    const FString BaseName = InCollection.GetName();

    UTexture2D* PositionTexture = SaveTexture(CollectionPkgDir,
        FString::Printf(TEXT("%s_%s"), *BaseName, BakeMode == ECk_Vat_BakeMode::Vertex ? TEXT("Pos") : TEXT("BonePos")),
        PositionPlane, InCollection.Get_BakeSettings().Get_Precision(), InPersistence);
    CK_ENSURE_IF_NOT(ck::IsValid(PositionTexture), TEXT("VatBaker: position texture bake failed for [{}]"), &InCollection)
    { return false; }

    UTexture2D* SecondaryTexture = SaveTexture(CollectionPkgDir,
        FString::Printf(TEXT("%s_%s"), *BaseName, BakeMode == ECk_Vat_BakeMode::Vertex ? TEXT("Nrm") : TEXT("BoneRot")),
        SecondaryPlane, InCollection.Get_BakeSettings().Get_Precision(), InPersistence);
    CK_ENSURE_IF_NOT(ck::IsValid(SecondaryTexture), TEXT("VatBaker: secondary texture bake failed for [{}]"), &InCollection)
    { return false; }

    // ---- weight-texture storage (bone mode): per-vertex indices/weights as data textures ----
    UTexture2D* BoneIndexTexture = nullptr;
    UTexture2D* BoneWeightTexture = nullptr;
    int32 WeightTexWidth = 0;
    int32 WeightTexRows = 0;
    if (BakeMode == ECk_Vat_BakeMode::Bone &&
        InCollection.Get_BakeSettings().Get_BoneWeightStorage() == ECk_Vat_BoneWeightStorage::WeightTexture)
    {
        WeightTexWidth = FMath::Min(NumVertices, MaxTextureWidth);
        WeightTexRows = FMath::DivideAndRoundUp(NumVertices, WeightTexWidth);
        CK_ENSURE_IF_NOT(WeightTexRows <= MaxTextureRows,
            TEXT("VatBaker: weight texture needs {} rows (max {}) for [{}] vertices — raise _MaxTextureRows or use a higher _SourceLOD"),
            WeightTexRows, MaxTextureRows, NumVertices)
        { return false; }

        FTexelPlane IndexPlane;
        FTexelPlane WeightPlane;
        IndexPlane.Init(WeightTexWidth, WeightTexRows);
        WeightPlane.Init(WeightTexWidth, WeightTexRows);
        for (int32 V = 0; V < NumVertices; ++V)
        {
            const FSourceVertex& Sv = SourceVertices[V];
            IndexPlane.Texels[V] = FVector4f(
                static_cast<float>(Sv.RenderBones[0]), static_cast<float>(Sv.RenderBones[1]),
                static_cast<float>(Sv.RenderBones[2]), static_cast<float>(Sv.RenderBones[3]));
            WeightPlane.Texels[V] = FVector4f(Sv.Weights[0], Sv.Weights[1], Sv.Weights[2], Sv.Weights[3]);
        }

        // Indices are ALWAYS 16F raw — Low would quantize them. Weights follow the collection precision and,
        // unlike vertex colors, these textures are linear end-to-end: no sRGB pre-decode.
        BoneIndexTexture = SaveTexture(CollectionPkgDir,
            FString::Printf(TEXT("%s_BoneIdx"), *BaseName), IndexPlane, ECk_Vat_Precision::High, InPersistence);
        CK_ENSURE_IF_NOT(ck::IsValid(BoneIndexTexture), TEXT("VatBaker: bone-index texture bake failed for [{}]"), &InCollection)
        { return false; }

        BoneWeightTexture = SaveTexture(CollectionPkgDir,
            FString::Printf(TEXT("%s_BoneWeight"), *BaseName), WeightPlane, InCollection.Get_BakeSettings().Get_Precision(), InPersistence);
        CK_ENSURE_IF_NOT(ck::IsValid(BoneWeightTexture), TEXT("VatBaker: bone-weight texture bake failed for [{}]"), &InCollection)
        { return false; }
    }

    UStaticMesh* BakedMesh = BuildBakedStaticMesh(InCollection, CollectionPkgDir,
        FString::Printf(TEXT("%s_Mesh"), *BaseName), SourceVertices, LODModel, *SourceMesh,
        WeightTexWidth, WeightTexRows, InPersistence);
    CK_ENSURE_IF_NOT(ck::IsValid(BakedMesh), TEXT("VatBaker: static-mesh build failed for [{}]"), &InCollection)
    { return false; }

    // ---- write back the serialized bake results ----
    UCk_VatCollection_Data::FCk_Vat_BakeResults Results;
    Results.BakedMesh = TStrongObjectPtr{BakedMesh};
    if (BakeMode == ECk_Vat_BakeMode::Vertex)
    {
        Results.PositionTexture = TStrongObjectPtr{PositionTexture};
        Results.NormalTexture = TStrongObjectPtr{SecondaryTexture};
    }
    else
    {
        Results.BonePositionTexture = TStrongObjectPtr{PositionTexture};
        Results.BoneRotationTexture = TStrongObjectPtr{SecondaryTexture};
        Results.BoneIndexTexture = TStrongObjectPtr{BoneIndexTexture};
        Results.BoneWeightTexture = TStrongObjectPtr{BoneWeightTexture};
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

    Results.TextureWidth = Width;
    Results.TextureRows = Rows;
    Results.AnimatedBounds = ck::anim_bake::ComputeAnimatedBounds(*SkeletonData, BoneBoundsAllFrames, *SourceMesh);
    Results.PositionBoundsMin = PositionBounds.Min;
    Results.PositionBoundsMax = PositionBounds.Max;

    if (NOT InCollection.ApplyBakeResults(Results))
    { return false; } // ApplyBakeResults already ensured loudly; nothing was stamped — do NOT save the package

    if (InPersistence == EPersistence::Transient)
    { return true; }

    // The collection is a pre-existing asset (no AssetCreated) — just persist the bake write-back.
    const FString CollectionPkgPath = InCollection.GetPackage()->GetName();
    const FString CollectionFileName =
        FPackageName::LongPackageNameToFilename(CollectionPkgPath, FPackageName::GetAssetPackageExtension());
    FSavePackageArgs SaveArgs;
    SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
    const auto Saved = UPackage::SavePackage(InCollection.GetPackage(), &InCollection, *CollectionFileName, SaveArgs);
    CK_ENSURE_IF_NOT(Saved,
        TEXT("VatBaker: could not save collection package [{}] — bake applied in memory but NOT persisted (file read-only / locked?)"),
        CollectionPkgPath)
    { return false; }
    return true;
}

}

// --------------------------------------------------------------------------------------------------------------------

auto
    ck::vat_editor::
    Bake_VatCollection(
        UCk_VatCollection_Data& InCollection)
    -> bool
{
    return ck_vat_baker::DoBake(InCollection, ck_vat_baker::EPersistence::SaveAssets);
}

auto
    ck::vat_editor::
    Bake_VatCollection_Transient(
        UCk_VatCollection_Data& InCollection)
    -> bool
{
    return ck_vat_baker::DoBake(InCollection, ck_vat_baker::EPersistence::Transient);
}
