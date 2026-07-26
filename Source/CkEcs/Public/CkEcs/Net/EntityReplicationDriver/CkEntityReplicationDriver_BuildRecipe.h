#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "UObject/Object.h"
#include "UObject/StrongObjectPtr.h"

#include "CkEcs/Net/EntityReplicationDriver/CkEntityReplicationDriver_Fragment_Data.h" // FCk_EntityReplicationDriver_ConstructionInfo

#include "CkEntityReplicationDriver_BuildRecipe.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// GC-safe holder for the ConstructionInfo(s) a Request_BuildAndReplicate entity was built from, kept so a later
// save capture can re-create it. A UObject exists here because fragments are NOT GC-traced while a UObject's
// UPROPERTYs are, so the fragment pins this holder instead. Mirrors UCk_EntityScript_SpawnRecipe_UE.
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
    // Read live at save capture and NEVER serialized as a fragment — it only pins the holder above
    struct CKECS_API FFragment_BuildRecipe
    {
        CK_GENERATED_BODY(FFragment_BuildRecipe);

    private:
        TStrongObjectPtr<UCk_BuildRecipe_UE> _Recipe;

    public:
        CK_PROPERTY_GET(_Recipe);

        CK_DEFINE_CONSTRUCTORS(FFragment_BuildRecipe, _Recipe);

    public:
        // Forwards to the holder; empty when unpopulated
        auto Get_ConstructionInfos() const -> const TArray<FCk_EntityReplicationDriver_ConstructionInfo>&;
    };
}

// --------------------------------------------------------------------------------------------------------------------
