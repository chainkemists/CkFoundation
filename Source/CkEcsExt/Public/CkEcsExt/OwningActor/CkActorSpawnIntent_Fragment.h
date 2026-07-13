#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkActorSpawnIntent_Fragment.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// Snapshotable record of "this entity owns a bridged actor of this class". Stamped by
// UCk_EntityScript_WithActor_UE::Construct at initial (non-load) construction. On load, after the snapshot
// restores the entity, the CkSnapshot respawn pass re-spawns an actor of _ActorClassPath at the entity's restored
// Transform and re-bridges it. The spawn transform is NOT stored here — it comes from the restored Transform
// fragment. The raw AActor pointer is intentionally never serialized (parent design §8); only the class PATH.
//
// The class is stored as a plain FString (the UClass GetPathName), NOT a TSoftClassPtr: a soft-object path does
// not survive the snapshot's SaveGame memory-archive SerializeItem round-trip (it comes back empty), whereas a
// plain FString round-trips reliably (same value path that FFragment_SaveKey's FGuid uses). Resolved at respawn
// via FSoftClassPath::TryLoadClass.
USTRUCT(BlueprintType)
struct CKECSEXT_API FFragment_ActorSpawnIntent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FFragment_ActorSpawnIntent);

private:
    // NOTE: bare `SaveGame` specifier (sets CPF_SaveGame), NOT `meta=(SaveGame)`. A reflection save archive with
    // ArIsSaveGame=true (v3: CkEntityScript_SaveFields) gates SerializeItem on CPF_SaveGame — a meta tag does NOT
    // set that flag, so meta=(SaveGame) fields are silently dropped (round-trip empty). This field is NOT
    // reflection-serialized under v3 — CkSnapshot_CaptureV3 reads Get_ActorClassPath directly into the entity-table
    // entry — so the specifier does not affect its capture. FFragment_SaveKey uses the same bare specifier —
    // see CkSnapshot_SaveKey_Fragment.h.
    UPROPERTY(SaveGame)
    FString _ActorClassPath;

public:
    CK_PROPERTY_GET(_ActorClassPath);
    CK_DEFINE_CONSTRUCTORS(FFragment_ActorSpawnIntent, _ActorClassPath);
};

// --------------------------------------------------------------------------------------------------------------------
