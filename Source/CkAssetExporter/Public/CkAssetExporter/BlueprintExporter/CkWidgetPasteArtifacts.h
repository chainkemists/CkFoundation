#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UBlueprint;

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
