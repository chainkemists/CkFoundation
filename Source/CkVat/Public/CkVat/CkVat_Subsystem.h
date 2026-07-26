#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "CkVat_Subsystem.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_VatCollection_Data;
class UCk_IsmRenderer_Data;
class UMaterialInstanceDynamic;

// --------------------------------------------------------------------------------------------------------------------

// One shared render state per (collection, world) — ALL instances of a collection MUST share ONE
// MID (see CkVat/CLAUDE.md).
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
    // Contract with the CkVat look assets: 12 per-instance scalars, slots [0..11], in the order Pack_CustomData writes them.
    static constexpr int32 NumPerInstanceFloats = 12;

public:
    // Unset (after a loud ensure) when the collection is unbaked or its generated look master is
    // missing (run `Ck_Usf_GenerateLooks` after changing the looks).
    // Returned BY VALUE, never as a pointer into _RenderStates — TMap element addresses are not stable.
    auto
    GetOrCreate_RenderState(
        const UCk_VatCollection_Data* InCollection) -> TOptional<FCk_Vat_RenderState>;

private:
    UPROPERTY(Transient)
    TMap<TObjectPtr<const UCk_VatCollection_Data>, FCk_Vat_RenderState> _RenderStates;
};
