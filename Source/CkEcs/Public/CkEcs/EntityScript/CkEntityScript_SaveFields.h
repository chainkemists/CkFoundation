#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <InstancedStruct.h>

#include "CkEntityScript_SaveFields.generated.h"

// --------------------------------------------------------------------------------------------------------------------
// Save-transport payload for an EntityScript instance's SaveGame-tagged UPROPERTYs: the v3 load re-Constructs a FRESH
// script UObject, so those fields would otherwise reset to class defaults. The FCk_SaveData_ prefix is deliberately NOT
// FCk_RepData_ — this never rides the wire and must stay off the RepData census ratchet.
// SCOPE: FCk_Handle-typed SaveGame fields inside the script are NOT saved-id-remapped (the blob is opaque to the
// outer handle walker) and are out of scope for this handler.
// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKECS_API FCk_SaveData_EntityScriptFields
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SaveData_EntityScriptFields);

private:
    // Capture-time UClass path; used only for a class-drift diagnostic — the replay itself is name-based.
    UPROPERTY()
    FString _ScriptClassPath;

    // Tagged-property bytes of the script's CPF_SaveGame UPROPERTYs (ArIsSaveGame=true capture).
    UPROPERTY()
    TArray<uint8> _FieldBytes;

public:
    CK_PROPERTY(_ScriptClassPath);
    CK_PROPERTY(_FieldBytes);
};

// --------------------------------------------------------------------------------------------------------------------
