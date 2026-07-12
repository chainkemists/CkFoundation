#pragma once

#include "CkCore/Macros/CkMacros.h"

#include <InstancedStruct.h>

#include "CkEntityScript_SaveFields.generated.h"

// --------------------------------------------------------------------------------------------------------------------
//
// Save-transport payload for an EntityScript instance's SaveGame-tagged UPROPERTYs.
//
// The v3 load rebuilds every EntityScript by re-Constructing a FRESH UObject from its recipe class-path, so any
// UPROPERTY(SaveGame) fields on the script reset to their class defaults. This framework handler (CkEcs, registered in
// the .cpp) reflect-walks the script instance's CPF_SaveGame FProperties and round-trips them as a Save-only payload:
//   - Produce serializes them (FObjectAndNameAsStringProxyArchive with ArIsSaveGame=true) into _FieldBytes.
//   - Apply, under a load-path hydration scope only, replays them back onto the re-Constructed script instance.
//
// One handler serves ALL scripts (framework-level, not per-script). The FCk_SaveData_ prefix is deliberately NOT
// FCk_RepData_ — this payload never rides the wire (Transport = Save) and must stay off the RepData census ratchet
// (Ck.Snapshot.Meta.RepDataRestoreCoverage enumerates FCk_RepData_*).
//
// SCOPE NOTE: FCk_Handle-typed SaveGame fields inside the script are NOT saved-id-remapped (the inner byte blob is
// opaque to the outer handle walker). SaveGame handle fields on a script are out of scope for this handler.
//
// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKECS_API FCk_SaveData_EntityScriptFields
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SaveData_EntityScriptFields);

private:
    // Path name of the script's UClass at capture time — used only for a diagnostic warning on class drift; the
    // tagged-property replay itself is name-based and layout-tolerant, so a mismatch still applies.
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
