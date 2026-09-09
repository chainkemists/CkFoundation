#pragma once

#include <CoreMinimal.h>

#include "CkGroundNav_DataLayerSelector.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_GroundNav_DataLayerSelector;

namespace ck::groundnav
{
    CKGROUNDNAV_API auto TryMake_DataLayerSelector(
        TConstArrayView<FName> InLayerNames,
        ::FCk_GroundNav_DataLayerSelector& OutSelector) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------

/**
 * The authored data-layer constraint for one GroundNav bake. An empty list accepts every body;
 * otherwise a body contributes when it belongs to at least one listed layer. Bodies with no layer
 * membership never match a non-empty selector.
 *
 * Values are created through TryMake_DataLayerSelector: it rejects NAME_None and normalizes the
 * remaining names into FName lexical order with duplicates removed. This gives the runtime filter
 * and the fingerprint one stable representation regardless of authoring order.
 */
USTRUCT(BlueprintType)
struct CKGROUNDNAV_API FCk_GroundNav_DataLayerSelector
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_GroundNav_DataLayerSelector);

    friend auto ck::groundnav::TryMake_DataLayerSelector(
        TConstArrayView<FName> InLayerNames,
        FCk_GroundNav_DataLayerSelector& OutSelector) -> bool;

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TArray<FName> _LayerNames;

public:
    CK_PROPERTY_GET(_LayerNames);

    auto Get_IsAll() const -> bool { return _LayerNames.IsEmpty(); }

    /** True only for the canonical representation produced by TryMake_DataLayerSelector. */
    auto Get_IsCanonical() const -> bool;

    /** Exact-any matching over a body's already-resolved layer names. */
    auto Get_MatchesAny(TConstArrayView<FName> InBodyLayerNames) const -> bool;
};

namespace ck::groundnav
{
    using ::FCk_GroundNav_DataLayerSelector;

    /**
     * Validates and canonicalizes InLayerNames into OutSelector. On failure OutSelector is left unchanged,
     * so an invalid edit cannot partially change a pending build request.
     */
}

// --------------------------------------------------------------------------------------------------------------------
