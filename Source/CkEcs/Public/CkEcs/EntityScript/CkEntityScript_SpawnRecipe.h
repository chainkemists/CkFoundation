#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "Templates/SubclassOf.h"
#include "UObject/Object.h"
#include "UObject/StrongObjectPtr.h"

#include <StructUtils/InstancedStruct.h>

#include "CkEntityScript_SpawnRecipe.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_EntityScript_UE;

// --------------------------------------------------------------------------------------------------------------------

// GC-safe holder for a RuntimeSpawned entity's construction recipe (save/load rebuild+hydrate, spec §4.2). The
// recipe (EntityScript class + spawn params) is retained from spawn until the v3 save capture reads it. It CANNOT
// live directly in the plain ck::FFragment_SpawnRecipe: CkFoundation fragments are not GC-traced, so a bare
// FInstancedStruct member (and its UScriptStruct type ptr) would dangle — worst for NotInstanced scripts (no
// per-entity object exists) and AS-defined params structs (reinstanced on script reload). A UObject's UPROPERTYs
// ARE traced (FInstancedStruct traces its inner refs via AddStructReferencedObjects), so the fragment pins THIS
// holder via a TStrongObjectPtr. Mirrors FCk_EntityReplicationDriver_ReplicationData_EntityScript (the UPROPERTY
// carrier) + FFragment_EntityScript_Current._SnapshotLoadPin (the fragment-pins-a-UObject shape). See [P3A-F1].
UCLASS()
class CKECS_API UCk_EntityScript_SpawnRecipe_UE : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_EntityScript_SpawnRecipe_UE);

public:
    auto
    Populate(
        TSubclassOf<UCk_EntityScript_UE> InScriptClass,
        FInstancedStruct InSpawnParams) -> void;

private:
    UPROPERTY()
    TSubclassOf<UCk_EntityScript_UE> _ScriptClass;

    UPROPERTY()
    FInstancedStruct _SpawnParams;

public:
    CK_PROPERTY_GET(_ScriptClass);
    CK_PROPERTY_GET(_SpawnParams);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Retained construction recipe for save-file capture (spec §4.2 RuntimeSpawned). Stamped at spawn by the
    // EntityScript spawn pipeline; the params are serialized with handle-remap at v3 capture time. NOT snapshotable
    // (v3-only; read live at capture) — it must never round-trip through Model A. GC storage lives on the holder
    // UObject above; this fragment only pins it.
    struct CKECS_API FFragment_SpawnRecipe
    {
        CK_GENERATED_BODY(FFragment_SpawnRecipe);

    private:
        TStrongObjectPtr<UCk_EntityScript_SpawnRecipe_UE> _Recipe;

    public:
        CK_PROPERTY_GET(_Recipe);

        CK_DEFINE_CONSTRUCTORS(FFragment_SpawnRecipe, _Recipe);

    public:
        // Convenience reads forwarding to the holder (the v3 writer reads through these).
        auto Get_ScriptClass() const -> TSubclassOf<UCk_EntityScript_UE>;
        auto Get_SpawnParams() const -> const FInstancedStruct&;
    };
}

// --------------------------------------------------------------------------------------------------------------------
