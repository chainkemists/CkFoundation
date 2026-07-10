#pragma once

#include "CoreMinimal.h"
#include "EditorSubsystem.h"
#include "CkVat_BakerSubsystem.generated.h"

class UCk_VatCollection_Data;

// Editor entry point for the VAT baker (mirrors UCkParticles_GeneratorSubsystem). Callable from
// Blueprint editor utilities and AngelScript editor scripts.
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
};
