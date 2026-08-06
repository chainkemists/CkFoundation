#include "CkEntityReplicationDriver_BuildRecipe.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_BuildRecipe_UE::
    Populate(
        TArray<FCk_EntityReplicationDriver_Spec> InConstructionInfos)
    -> void
{
    _ConstructionInfos = MoveTemp(InConstructionInfos);
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    auto
        FFragment_BuildRecipe::
        Get_ConstructionInfos() const
        -> const TArray<FCk_EntityReplicationDriver_Spec>&
    {
        static const auto Empty = TArray<FCk_EntityReplicationDriver_Spec>{};
        return _Recipe.IsValid() ? _Recipe->Get_ConstructionInfos() : Empty;
    }
}

// --------------------------------------------------------------------------------------------------------------------
