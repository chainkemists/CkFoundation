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
    // Master switch for the AS bootstrap self-heal dispatcher: failed AS compiles
    // caused by stale generated accessor files recover by synthesizing sibling
    // `_StubRecovery_*` stubs. Disable (or pass `-NoCkAsRegen`) to fall back to the
    // unmodified Hazelight AS-failure flow.
    UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "AS Bootstrap Self-Heal",
              meta = (AllowPrivateAccess = true))
    bool _EnableAsBootstrapSelfHeal = true;

public:
    CK_PROPERTY_GET(_EnableAsBootstrapSelfHeal);
};

// --------------------------------------------------------------------------------------------------------------------
