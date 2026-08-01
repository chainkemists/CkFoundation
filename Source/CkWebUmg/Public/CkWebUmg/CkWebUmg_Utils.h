#pragma once

#include "CkWebUmg/Asset/CkWebUmg_PageAsset.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkWebUmg_Utils.generated.h"

// ====================================================================================================================
// The public API surface of CkWebUmg (house doctrine: Utils is the ONLY public surface). BP/AS
// entry points over the imported PageAsset — the three-environments face of the Gate 4 output.
// Widget-tree construction stays C++ (Slate widgets are not reflected types); a UMG wrapper is
// Gate 5+ scope.
// ====================================================================================================================

UCLASS(NotBlueprintable)
class CKWEBUMG_API UCk_Utils_WebUmg_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_WebUmg_UE);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|WebUmg",
              DisplayName="[Ck][WebUmg] Get Node Count")
    static int32
    Get_NodeCount(
        const UCk_WebUmg_PageAsset_UE* InAsset);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|WebUmg",
              DisplayName="[Ck][WebUmg] Get Conversion Report")
    static TArray<FCk_WebUmg_ReportEntryData>
    Get_ConversionReport(
        const UCk_WebUmg_PageAsset_UE* InAsset);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|WebUmg",
              DisplayName="[Ck][WebUmg] Get Named Node")
    static FCk_WebUmg_NodeData
    Get_NamedNode(
        const UCk_WebUmg_PageAsset_UE* InAsset,
        const FString& InCkName,
        bool& OutFound);
};
