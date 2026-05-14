#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkAngelscriptGenerator_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "AngelScript Generator"))
class CKANGELSCRIPTGENERATOR_API UCk_AngelscriptGenerator_ProjectSettings_UE
    : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_AngelscriptGenerator_ProjectSettings_UE);

private:
    // Master switch for the Rev 10 AS bootstrap self-heal dispatcher.
    //
    // When true (default): the OnReloadHadErrors hook is armed; failed AS
    // compiles caused by stale generated accessor files self-recover by
    // synthesizing minimum-viable stubs into sibling `_StubRecovery_*` files
    // alongside the affected canonicals (EntitySpawnParams + AssetRegistry
    // .as + DynamicHandleTypes.json). AS namespace-merge resolves the missing
    // accessor on the next compile pass.
    //
    // When false (or when the `-NoCkAsRegen` CLI flag is set): the hook is
    // not registered and the editor falls back to the unmodified Hazelight
    // AS-failure flow. Useful when debugging a real AS authoring bug that
    // happens to share an error shape we'd otherwise try to recover from.
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "AS Bootstrap Self-Heal",
              meta = (AllowPrivateAccess = true))
    bool _EnableAsBootstrapSelfHeal = true;

public:
    CK_PROPERTY_GET(_EnableAsBootstrapSelfHeal);
};

// --------------------------------------------------------------------------------------------------------------------
