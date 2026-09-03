#pragma once

#include "CkGroundNav/Shadow/CkGroundNav_Shadow_Fragment.h"

#include <CoreMinimal.h>

// --------------------------------------------------------------------------------------------------------------------
// Folding comparisons into the diagnostics, and rendering the diagnostics as text.
//
// Both take the fragment by reference and nothing else, so both are callable with a stack-allocated
// fragment and no world behind it. The rendered text is diff-stable by construction: a fixed column
// order, keys sorted rather than iterated in map order, fixed precision per unit, and a row for every
// fixture that was ever opened - an absent row and a zero row must not read the same.
// --------------------------------------------------------------------------------------------------------------------

namespace ck::groundnav::shadow
{
    /**
     * Fold one comparison into the fixture the diagnostics currently name, or into InFallbackKey when
     * no fixture is open. The key is resolved by the caller because the fallback is the world's map
     * name, and nothing in this file may know what a world is.
     *
     * A disagreement - an outcome only one provider reached, a fail reason they answered differently,
     * or a Partial exactly one of them reported - also records the comparison's query id, so the
     * report names what disagreed rather than only counting it.
     */
    CKGROUNDNAV_API auto Accumulate(
        FFragment_GroundNav_ShadowDiagnostics& InOutDiagnostics,
        const FCk_GroundNav_ShadowComparison&  InComparison,
        FName                                  InFallbackKey) -> void;

    /** The column names a row is read against, exposed so a test can assert the schema without
     *  re-deriving it. */
    CKGROUNDNAV_API auto Get_ReportHeader() -> FString;

    /**
     * The whole report as one multi-line string: a schema line, the header line, one row per fixture
     * sorted by name, the diverging query ids, and the artifact identity the run should be filed
     * under. Pure - the same fragment renders the same bytes every time.
     */
    CKGROUNDNAV_API auto Get_Report(
        const FFragment_GroundNav_ShadowDiagnostics& InDiagnostics,
        const FString&                               InArtifactIdentity) -> FString;
}

// --------------------------------------------------------------------------------------------------------------------
