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

// GC-safe holder for a RuntimeSpawned entity's construction recipe (EntityScript class + spawn params). It CANNOT
// live directly in the plain ck::FFragment_SpawnRecipe: CkFoundation fragments are not GC-traced, so a bare
// FInstancedStruct member would dangle. A UObject's UPROPERTYs ARE traced, so the fragment pins this holder instead.
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
    // Retained construction recipe, stamped at spawn and read live at capture. GC storage lives on the
    // holder UObject above; this fragment only pins it.
    struct CKECS_API FFragment_SpawnRecipe
    {
        CK_GENERATED_BODY(FFragment_SpawnRecipe);

    private:
        TStrongObjectPtr<UCk_EntityScript_SpawnRecipe_UE> _Recipe;

    public:
        CK_PROPERTY_GET(_Recipe);

        CK_DEFINE_CONSTRUCTORS(FFragment_SpawnRecipe, _Recipe);

    public:
        auto Get_ScriptClass() const -> TSubclassOf<UCk_EntityScript_UE>;
        auto Get_SpawnParams() const -> const FInstancedStruct&;
    };
}

// --------------------------------------------------------------------------------------------------------------------
