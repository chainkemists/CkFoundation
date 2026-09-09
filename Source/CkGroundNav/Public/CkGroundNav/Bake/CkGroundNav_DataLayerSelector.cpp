#include "CkGroundNav_DataLayerSelector.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_GroundNav_DataLayerSelector::
    Get_IsCanonical() const
    -> bool
{
    for (auto Index = 0; Index < _LayerNames.Num(); ++Index)
    {
        if (_LayerNames[Index].IsNone())
        { return false; }

        if (Index > 0 && NOT _LayerNames[Index - 1].LexicalLess(_LayerNames[Index]))
        { return false; }
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_GroundNav_DataLayerSelector::
    Get_MatchesAny(
        TConstArrayView<FName> InBodyLayerNames) const
    -> bool
{
    if (Get_IsAll())
    { return true; }

    for (const auto& BodyLayerName : InBodyLayerNames)
    {
        if (_LayerNames.Contains(BodyLayerName))
        { return true; }
    }

    return false;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav
{
    auto
        TryMake_DataLayerSelector(
            TConstArrayView<FName>              InLayerNames,
            FCk_GroundNav_DataLayerSelector& OutSelector)
        -> bool
    {
        auto CanonicalNames = TArray<FName>{};
        CanonicalNames.Reserve(InLayerNames.Num());

        for (const auto& LayerName : InLayerNames)
        {
            if (LayerName.IsNone())
            { return false; }

            CanonicalNames.AddUnique(LayerName);
        }

        CanonicalNames.Sort(FNameLexicalLess{});

        auto Candidate = FCk_GroundNav_DataLayerSelector{};
        Candidate._LayerNames = MoveTemp(CanonicalNames);
        OutSelector = MoveTemp(Candidate);

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------
