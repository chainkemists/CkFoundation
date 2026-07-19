#include "CkVat_Subsystem.h"

#include "CkVat/CkVat_Log.h"
#include "CkVat/Collection/CkVatCollection_Data.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include "CkIsmRenderer/Renderer/CkIsmRenderer_TransientFactory.h"
#include "CkUsf/Apply/CkUsf_Utils.h"
#include "CkUsf/LookDefinition/CkUsf_LookDefinition_Naming.h"

#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInstanceDynamic.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Vat_Subsystem_UE::
    GetOrCreate_RenderState(
        const UCk_VatCollection_Data* InCollection)
    -> TOptional<FCk_Vat_RenderState>
{
    CK_ENSURE_IF_NOT(ck::IsValid(InCollection), TEXT("GetOrCreate_RenderState with an invalid collection"))
    { return {}; }

    if (const auto* Found = _RenderStates.Find(InCollection);
        Found != nullptr && ck::IsValid(Found->_Mid) && ck::IsValid(Found->_RendererData))
    { return *Found; }

    CK_ENSURE_IF_NOT(InCollection->Get_BakedData().Get_IsBaked() && ck::IsValid(InCollection->Get_BakedData().Get_BakedMesh()),
        TEXT("VatCollection [{}] is not baked — bake it in-editor before rendering"), InCollection)
    { return {}; }

    const auto BakeMode = InCollection->Get_BakeSettings().Get_BakeMode();
    const auto LookName = BakeMode == ECk_Vat_BakeMode::Vertex ? FName(TEXT("VatVertex")) : FName(TEXT("VatBone"));

    const auto MasterPath = ck::usf::Get_GeneratedMasterObjectPath(LookName);
    auto* Master = LoadObject<UMaterialInterface>(nullptr, *MasterPath);
    CK_ENSURE_IF_NOT(ck::IsValid(Master),
        TEXT("VAT look master [{}] not found — run `Ck_Usf_GenerateLooks {}` (collection [{}])"),
        MasterPath, LookName, InCollection)
    { return {}; }

    auto* Mid = UMaterialInstanceDynamic::Create(Master, this);
    CK_ENSURE_IF_NOT(ck::IsValid(Mid), TEXT("Could not create the VAT MID for collection [{}]"), InCollection)
    { return {}; }

    // ---- seed the per-collection uniforms (names: the CkVat look assets / Vat.ush param lists) ----
    UTexture2D* PositionTexture = BakeMode == ECk_Vat_BakeMode::Vertex
        ? InCollection->Get_BakedData().Get_PositionTexture().Get()
        : InCollection->Get_BakedData().Get_BonePositionTexture().Get();
    CK_ENSURE_IF_NOT(ck::IsValid(PositionTexture),
        TEXT("VatCollection [{}] is marked baked but has no position texture for mode [{}]"), InCollection, BakeMode)
    { return {}; }

    if (ck::IsValid(InCollection->Get_BaseColorTexture()))
    { UCk_Utils_Usf_UE::Set_Texture(Mid, TEXT("BaseColorTex"), InCollection->Get_BaseColorTexture()); }

    // Texture dims come from the SERIALIZED bake results, never GetSizeX/Y: those read platform
    // data and return 0 while a freshly-baked texture is still async-compiling — which seeded
    // BoneCount/TotalRows = 0 and collapsed every lookup onto one texel (Ck_Vat_DebugVerifyBake).
    CK_ENSURE_IF_NOT(InCollection->Get_BakedData().Get_TextureWidth() > 0 && InCollection->Get_BakedData().Get_TextureRows() > 0,
        TEXT("VatCollection [{}] has no serialized texture dimensions — rebake with the current baker"), InCollection)
    { return {}; }

    if (BakeMode == ECk_Vat_BakeMode::Vertex)
    {
        UTexture2D* NormalTexture = InCollection->Get_BakedData().Get_NormalTexture().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(NormalTexture),
            TEXT("VatCollection [{}] (Vertex mode) has no normal texture — rebake with the current baker"), InCollection)
        { return {}; }

        UCk_Utils_Usf_UE::Set_Texture(Mid, TEXT("PosVat"), PositionTexture);
        UCk_Utils_Usf_UE::Set_Texture(Mid, TEXT("NrmVat"), NormalTexture);
        UCk_Utils_Usf_UE::Set_Scalar(Mid, TEXT("TexelCount"), static_cast<float>(InCollection->Get_BakedData().Get_TextureWidth()));
    }
    else
    {
        UTexture2D* RotationTexture = InCollection->Get_BakedData().Get_BoneRotationTexture().Get();
        CK_ENSURE_IF_NOT(ck::IsValid(RotationTexture),
            TEXT("VatCollection [{}] (Bone mode) has no rotation texture"), InCollection)
        { return {}; }

        UCk_Utils_Usf_UE::Set_Texture(Mid, TEXT("BonePosVat"), PositionTexture);
        UCk_Utils_Usf_UE::Set_Texture(Mid, TEXT("BoneRotVat"), RotationTexture);
        UCk_Utils_Usf_UE::Set_Scalar(Mid, TEXT("BoneCount"), static_cast<float>(InCollection->Get_BakedData().Get_TextureWidth()));

        if (InCollection->Get_BakeSettings().Get_BoneWeightStorage() == ECk_Vat_BoneWeightStorage::WeightTexture)
        {
            UTexture2D* IndexTexture = InCollection->Get_BakedData().Get_BoneIndexTexture().Get();
            UTexture2D* WeightTexture = InCollection->Get_BakedData().Get_BoneWeightTexture().Get();
            CK_ENSURE_IF_NOT(ck::IsValid(IndexTexture) && ck::IsValid(WeightTexture),
                TEXT("VatCollection [{}] (WeightTexture storage) is missing its index/weight textures — rebake"), InCollection)
            { return {}; }

            UCk_Utils_Usf_UE::Set_Texture(Mid, TEXT("BoneIdxTex"), IndexTexture);
            UCk_Utils_Usf_UE::Set_Texture(Mid, TEXT("BoneWeightTex"), WeightTexture);
            UCk_Utils_Usf_UE::Set_Scalar(Mid, TEXT("WeightStorage"), 1.0f);
        }
        else
        {
            UCk_Utils_Usf_UE::Set_Scalar(Mid, TEXT("WeightStorage"), 0.0f);
        }
    }

    UCk_Utils_Usf_UE::Set_Scalar(Mid, TEXT("SampleFrequency"), static_cast<float>(InCollection->Get_BakeSettings().Get_SampleFrequency()));
    UCk_Utils_Usf_UE::Set_Scalar(Mid, TEXT("TotalRows"), static_cast<float>(InCollection->Get_BakedData().Get_TextureRows()));
    UCk_Utils_Usf_UE::Set_Scalar(Mid, TEXT("DecodeNormalized"),
        InCollection->Get_BakeSettings().Get_Precision() == ECk_Vat_Precision::Low ? 1.0f : 0.0f);

    const auto& BoundsMin = InCollection->Get_BakedData().Get_PositionBoundsMin();
    const auto& BoundsMax = InCollection->Get_BakedData().Get_PositionBoundsMax();
    UCk_Utils_Usf_UE::Set_Vector(Mid, TEXT("BoundsMin"), FLinearColor(BoundsMin.X, BoundsMin.Y, BoundsMin.Z));
    UCk_Utils_Usf_UE::Set_Vector(Mid, TEXT("BoundsMax"), FLinearColor(BoundsMax.X, BoundsMax.Y, BoundsMax.Z));

    // ---- one transient ISM renderer per collection: every material slot gets the shared MID ----
    UStaticMesh* BakedMesh = InCollection->Get_BakedData().Get_BakedMesh().Get();

    TArray<FCk_MeshMaterialOverride> Overrides;
    const int32 NumSlots = BakedMesh->GetStaticMaterials().Num();
    Overrides.Reserve(NumSlots);
    for (int32 Slot = 0; Slot < NumSlots; ++Slot)
    { Overrides.Emplace(Slot, Mid); }

    // Movable is load-bearing: the ISM proxy pushes per-instance custom data to the GPU only on
    // the Movable path — and VAT entities generally move anyway.
    auto* RendererData = UCk_Utils_IsmRenderer_TransientFactory_UE::GetOrCreate_ForMeshWithMaterialsAndCustomData(
        GetWorld(), BakedMesh, Overrides, NumPerInstanceFloats, ECk_Mobility::Movable);
    CK_ENSURE_IF_NOT(ck::IsValid(RendererData),
        TEXT("Could not create the transient ISM renderer for VatCollection [{}]"), InCollection)
    { return {}; }

    FCk_Vat_RenderState State;
    State._Mid = Mid;
    State._RendererData = RendererData;
    _RenderStates.Add(InCollection, State);
    return State;
}
