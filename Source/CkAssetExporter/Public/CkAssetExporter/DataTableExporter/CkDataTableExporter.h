#pragma once

#include "CoreMinimal.h"

// --------------------------------------------------------------------------------------------------------------------

class UDataTable;

// --------------------------------------------------------------------------------------------------------------------

struct CKASSETEXPORTER_API FCk_DataTableExportResult
{
    bool Succeeded = false;
    FString JsonFilePath;
    FString CsvFilePath;
    FString ErrorMessage;
    FString AssetName;
};

// --------------------------------------------------------------------------------------------------------------------
// Writes two siblings next to the source .uasset: <Base>.csv (GetTableAsCSV) and <Base>.json (a versioned root that
// carries the "_meta" freshness stamp, the row-struct path, and the row array parsed back out of GetTableAsJSON). A
// GetTableAsJSON output that fails to parse is a loud failure (ErrorMessage), never a silent drop.
// --------------------------------------------------------------------------------------------------------------------

class CKASSETEXPORTER_API FCk_DataTableExporter
{
public:
    static auto
    ExportDataTable(
        UDataTable* InDataTable) -> FCk_DataTableExportResult;
};

// --------------------------------------------------------------------------------------------------------------------
