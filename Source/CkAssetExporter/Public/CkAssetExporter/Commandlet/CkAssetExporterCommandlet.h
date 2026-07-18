#pragma once

#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"

#include "CkAssetExporterCommandlet.generated.h"

// --------------------------------------------------------------------------------------------------------------------

/**
 * Headless entry point for the asset exporters. Routes each asset to its most-specific exporter (Behavior Tree,
 * Blueprint/Widget, DataAsset, EQS, State Tree, struct, enum, Niagara, Cascade) which self-writes .json/.txt siblings
 * next to the source asset.
 *
 *   UnrealEditor-Cmd.exe <Project.uproject> -run=CkAssetExporter -Assets=/Game/A.A;/Game/B.B
 *   UnrealEditor-Cmd.exe <Project.uproject> -run=CkAssetExporter -Dir=/Game/Foo -Classes=DataAsset,Blueprint
 *   UnrealEditor-Cmd.exe <Project.uproject> -run=CkAssetExporter -Dir=/Game -List
 *   UnrealEditor-Cmd.exe <Project.uproject> -run=CkAssetExporter -DumpGraph -Dir=/Game/Foo
 *   UnrealEditor-Cmd.exe <Project.uproject> -run=CkAssetExporter -ExportServer
 *
 * Flags: -Assets= (semicolon-separated tokens), -Dir= (+ optional -Classes= comma-separated), -List, -SkipFresh,
 * -Force (overrides -SkipFresh), -DumpGraph (dependency-graph dump of every class under -Dir, default /Game, to
 * graph.json; composes with nothing else), -Out= (logged as unsupported in v1 — exporters always write sibling
 * paths), -ExportServer (runs the request-file server loop; "-Server" is a reserved engine switch). Exit 0 iff the
 * run completed and no asset failed.
 */
UCLASS()
class CKASSETEXPORTER_API UCkAssetExporterCommandlet : public UCommandlet
{
    GENERATED_BODY()

public:
    UCkAssetExporterCommandlet();

    virtual int32 Main(const FString& InParams) override;
};

// --------------------------------------------------------------------------------------------------------------------
