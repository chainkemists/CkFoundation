#pragma once

#include "CkWebUmg/Asset/CkWebUmg_PageAsset.h"

#include "EditorReimportHandler.h"
#include "Factories/Factory.h"

#include "CkWebUmg_PageAssetFactory.generated.h"

// ====================================================================================================================
// Content-Browser face of the emission pipeline: drag a .html into the Content Browser (or use any
// standard import entry point) and the full toolchain runs — extractor, bundle, PageAsset, report.
// Also the FReimportHandler: right-click -> Reimport re-runs from the asset's stamped source
// (html re-extracts, json re-reads; unchanged sources no-op via the hash stamp — DECISION 3).
// ====================================================================================================================

UCLASS()
class CKWEBUMGEDITOR_API UCk_WebUmg_PageAssetFactory : public UFactory, public FReimportHandler
{
    GENERATED_BODY()

public:
    UCk_WebUmg_PageAssetFactory();

    auto FactoryCreateFile(
        UClass* InClass,
        UObject* InParent,
        FName InName,
        EObjectFlags InFlags,
        const FString& InFilename,
        const TCHAR* InParms,
        FFeedbackContext* InWarn,
        bool& OutOperationCanceled) -> UObject* override;

    auto CanReimport(
        UObject* InObj,
        TArray<FString>& OutFilenames) -> bool override;

    auto SetReimportPaths(
        UObject* InObj,
        const TArray<FString>& InNewReimportPaths) -> void override;

    auto Reimport(
        UObject* InObj) -> EReimportResult::Type override;
};
