#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UBlueprint;

// --------------------------------------------------------------------------------------------------------------------
// Widget-Blueprint paste artifacts — the porting workflow's "copy the hierarchy in the Designer" step, automated.
//
//   <Base>.hierarchy.copy.txt        — the FULL widget tree as designer-clipboard exported text. Byte-identical to
//                                      what Ctrl+C in the UMG Designer produces for a select-all, so it PASTES
//                                      directly into another Widget Blueprint (5.5 and 5.7 produce/parse the same
//                                      format). Kept header-free on purpose: any prologue would no longer paste.
//   <Base>.animation.<Name>.t3d.txt  — one reference-grade T3D dump per UWidgetAnimation (tracks, sections, keys via
//                                      subobject recursion). NOT pasteable: the UMG Designer has no
//                                      paste-animation-from-text path at all (its only affordance is an in-memory
//                                      Duplicate), so these exist to make animations reconstructable by hand/agent,
//                                      not to round-trip.
//
// Stale <Base>.animation.*.t3d.txt files from renamed/removed animations are deleted before the current set is
// written, so the sibling set always mirrors the asset. Output is deterministic (no timestamps, no machine paths).
// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_WidgetPasteArtifacts
{
public:
    struct FCk_WidgetPasteArtifactsResult
    {
        bool    Succeeded = false;
        TArray<FString> WrittenFiles;
        FString ErrorMessage;
    };

    // No-op success for non-WidgetBlueprint assets. InBasePathNoExt is the sibling base the Blueprint exporter
    // already resolved (e.g. ".../MembershipBoard_BB_WBP" — no extension).
    static auto
    ExportFor(
        UBlueprint* InBlueprint,
        const FString& InBasePathNoExt) -> FCk_WidgetPasteArtifactsResult;
};

// --------------------------------------------------------------------------------------------------------------------
