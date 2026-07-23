#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Time/CkTime.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkDialog/CkDialogBank_DataAsset.h"

#include "Kismet/BlueprintFunctionLibrary.h"

#include "CkDialog_ProjectSettings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Dialog"))
class CKDIALOG_API UCk_Dialog_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Dialog_ProjectSettings_UE);

private:
    // Banks loaded synchronously into the world's Dialog registry at world init (banks are small data-only assets).
    UPROPERTY(Config, EditDefaultsOnly, Category = "Banks",
              meta = (AllowPrivateAccess = true))
    TArray<TSoftObjectPtr<UCk_DialogBank_DataAsset>> _DialogBanks;

    // A query issued before the registry is ready is deferred; if the registry never becomes ready within this
    // budget the query processor fires a loud ensure (a never-ready registry is a misconfiguration, not silence).
    UPROPERTY(Config, EditDefaultsOnly, Category = "Query",
              meta = (AllowPrivateAccess = true))
    FCk_Time _QueryReadinessTimeoutSeconds = FCk_Time{5.0};

    // Per-emitter debugger ring-buffer depth (last N query results retained for the graph-debugger live overlay).
    UPROPERTY(Config, EditDefaultsOnly, Category = "Debugger",
              meta = (AllowPrivateAccess = true, ClampMin = "1", UIMin = "1", ClampMax = "64", UIMax = "64"))
    int32 _DebugHistorySize = 8;

public:
    CK_PROPERTY_GET(_DialogBanks);
    CK_PROPERTY_GET(_QueryReadinessTimeoutSeconds);
    CK_PROPERTY_GET(_DebugHistorySize);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKDIALOG_API UCk_Utils_Dialog_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Dialog_Settings_UE);

public:
    // Internal C++ accessor avoiding repeated GetDefault calls in hot paths.
    static const UCk_Dialog_ProjectSettings_UE*
    Get();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Dialog|Settings")
    static TArray<TSoftObjectPtr<UCk_DialogBank_DataAsset>>
    Get_DialogBanks();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Dialog|Settings")
    static FCk_Time
    Get_QueryReadinessTimeoutSeconds();

    UFUNCTION(BlueprintPure, Category = "Ck|Utils|Dialog|Settings")
    static int32
    Get_DebugHistorySize();
};

// --------------------------------------------------------------------------------------------------------------------
