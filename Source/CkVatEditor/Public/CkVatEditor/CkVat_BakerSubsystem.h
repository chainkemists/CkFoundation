#pragma once

#include "CkVat/Collection/CkVatCollection_Data.h"

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "CkVat_BakerSubsystem.generated.h"

// Editor entry point for the VAT baker; callable from Blueprint editor utilities and AngelScript editor scripts.
UCLASS()
class CKVATEDITOR_API UCkVat_BakerSubsystem : public UEditorSubsystem
{
    GENERATED_BODY()

public:
    // Bakes the collection in place: generates the static mesh + VAT textures as sibling assets
    // (overwrite-in-place) and writes the serialized clip table back onto the collection.
    UFUNCTION(BlueprintCallable, Category = "CkVat",
              DisplayName = "Bake VAT Collection")
    bool Bake_VatCollection(UCk_VatCollection_Data* InCollection);

    // Builds a fully-baked TRANSIENT collection in memory — NOTHING is saved to disk, results die with the
    // session. Gym/test entry point; shipped collections are authored in the details panel + Bake button.
    // Returns nullptr on bake failure (the baker's ensures name the reason).
    UFUNCTION(BlueprintCallable, Category = "CkVat",
              DisplayName = "Create and Bake Transient VAT Collection")
    UCk_VatCollection_Data* CreateAndBake_TransientCollection(
        USkeleton* InSkeleton,
        USkeletalMesh* InSourceMesh,
        const TArray<FCk_VatCollection_ClipDef>& InClips,
        int32 InSampleFrequency = 30,
        ECk_Vat_BakeMode InBakeMode = ECk_Vat_BakeMode::Bone,
        ECk_Vat_Precision InPrecision = ECk_Vat_Precision::High,
        ECk_Vat_BoneWeightStorage InBoneWeightStorage = ECk_Vat_BoneWeightStorage::MeshChannels,
        ECk_Vat_BoneInfluences InBoneInfluences = ECk_Vat_BoneInfluences::Four,
        int32 InSourceLOD = 0);
};
