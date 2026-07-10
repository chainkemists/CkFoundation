#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "CkVat_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_VatCollection_Data;
class UCk_IsmRenderer_Data;
class UMaterialInstanceDynamic;

// --------------------------------------------------------------------------------------------------------------------

// One shared render state per (collection, world). Per-instance playback rides custom-data floats,
// so ALL instances of a collection must share ONE MID — a per-entity MID would key a separate ISM
// component in the transient factory and defeat instancing.
USTRUCT()
struct CKVAT_API FCk_Vat_RenderState
{
    GENERATED_BODY()

public:
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> _Mid;

    UPROPERTY(Transient)
    TObjectPtr<UCk_IsmRenderer_Data> _RendererData;
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKVAT_API UCk_Vat_Subsystem_UE : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    // Per the per-instance float contract (Plan/Gate_02_Material.md): 12 scalars, slots [0..11].
    // The CkVat look assets declare exactly these per-instance params in this order.
    static constexpr int32 NumPerInstanceFloats = 12;

public:
    // Resolves (or builds) the collection's shared MID + transient ISM renderer. The MID is created
    // from the generated look master (VatVertex/VatBone by bake mode) and seeded with the
    // collection's uniforms (textures, sample frequency, texture dims, bounds, decode mode).
    // Returns nullptr (after a loud ensure) when the collection is unbaked or the look master is
    // missing (run `Ck_Usf_GenerateLooks` after changing the looks).
    auto
    GetOrCreate_RenderState(
        const UCk_VatCollection_Data* InCollection) -> const FCk_Vat_RenderState*;

private:
    UPROPERTY(Transient)
    TMap<TObjectPtr<const UCk_VatCollection_Data>, FCk_Vat_RenderState> _RenderStates;
};
