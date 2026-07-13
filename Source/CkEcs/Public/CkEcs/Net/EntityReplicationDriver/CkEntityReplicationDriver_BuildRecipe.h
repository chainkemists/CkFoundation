#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "UObject/Object.h"
#include "UObject/StrongObjectPtr.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment_Data.h" // FCk_EntityReplicationDriver_ConstructionInfo

#include "CkEntityReplicationDriver_BuildRecipe.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// GC-safe holder for a definition-built entity's construction recipe. An entity built via Request_BuildAndReplicate
// is constructed from ConstructionInfo(s) (a construction-script class + an optional archetype asset) that are
// consumed at build time and NOT retained on the entity. This holder pins them so a later save capture can read the
// recipe and re-create the entity on load. The archetype is a UObject ref a non-GC-traced fragment cannot hold
// directly; a UObject's UPROPERTYs ARE traced, so the fragment pins THIS holder via a TStrongObjectPtr. Mirrors
// UCk_EntityScript_SpawnRecipe_UE (the EntityScript-spawn equivalent).
UCLASS()
class CKECS_API UCk_BuildRecipe_UE : public UObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_BuildRecipe_UE);

public:
    auto
    Populate(
        TArray<FCk_EntityReplicationDriver_ConstructionInfo> InConstructionInfos) -> void;

private:
    UPROPERTY()
    TArray<FCk_EntityReplicationDriver_ConstructionInfo> _ConstructionInfos;

public:
    CK_PROPERTY_GET(_ConstructionInfos);
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    // Retained construction recipe for a definition-built (Request_BuildAndReplicate) entity, read live at save
    // capture. Read-live-only — never serialized as a fragment. GC storage lives on the holder UObject above; this
    // fragment only pins it.
    struct CKECS_API FFragment_BuildRecipe
    {
        CK_GENERATED_BODY(FFragment_BuildRecipe);

    private:
        TStrongObjectPtr<UCk_BuildRecipe_UE> _Recipe;

    public:
        CK_PROPERTY_GET(_Recipe);

        CK_DEFINE_CONSTRUCTORS(FFragment_BuildRecipe, _Recipe);

    public:
        // Convenience read forwarding to the holder (the capture reads through this). Empty when unpopulated.
        auto Get_ConstructionInfos() const -> const TArray<FCk_EntityReplicationDriver_ConstructionInfo>&;
    };
}

// --------------------------------------------------------------------------------------------------------------------
