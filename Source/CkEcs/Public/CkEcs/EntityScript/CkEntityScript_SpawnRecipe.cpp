#include "CkEcs/EntityScript/CkEntityScript_SpawnRecipe.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_EntityScript_SpawnRecipe_UE::
    Populate(
        TSubclassOf<UCk_EntityScript_UE> InScriptClass,
        FInstancedStruct InSpawnParams)
    -> void
{
    _ScriptClass = InScriptClass;
    _SpawnParams = MoveTemp(InSpawnParams);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FFragment_SpawnRecipe::
        Get_ScriptClass() const
        -> TSubclassOf<UCk_EntityScript_UE>
    {
        return _Recipe.IsValid() ? _Recipe->Get_ScriptClass() : TSubclassOf<UCk_EntityScript_UE>{};
    }

    auto
        FFragment_SpawnRecipe::
        Get_SpawnParams() const
        -> const FInstancedStruct&
    {
        static const auto EmptyParams = FInstancedStruct{};
        return _Recipe.IsValid() ? _Recipe->Get_SpawnParams() : EmptyParams;
    }
}

// --------------------------------------------------------------------------------------------------------------------
