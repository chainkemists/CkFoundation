#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"   // USTRUCT/UENUM/GENERATED_BODY infra + TObjectPtr + UScriptStruct

#include "CkCore/Macros/CkMacros.h"

#include "CkProcessor_ScriptQuery_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UScriptStruct;

// FCk_ScriptQueryBatchState lives in CkDynamic (it holds entt-storage pointers CkEcs must not see). CkEcs holds only
// an opaque pointer to it inside FCk_ScriptQueryBatch, so the base-class ForEachBatch event can pass the batch by
// value into script; the mixin methods that dereference the state are bound in CkDynamic. This keeps the dependency
// direction CkEcs -/-> CkDynamic intact (mirroring how FProcessorDescriptor lives here but is populated by CkDynamic).
struct FCk_ScriptQueryBatchState;

// --------------------------------------------------------------------------------------------------------------------
// Access intent a script processor declares for a fragment slot. ReadWrite/ReadOnly feed the descriptor's RW/RO
// fragment sets (write-conflict detection + auto-ordering); Require is presence-only (matched but no data param);
// Exclude rejects entities that carry the fragment.
UENUM(BlueprintType)
enum class ECk_ScriptQueryAccess : uint8
{
    ReadOnly,
    ReadWrite,
    Require,
    Exclude
};

// --------------------------------------------------------------------------------------------------------------------
// One declared fragment in a script processor's query.
USTRUCT(BlueprintType)
struct CKECS_API FCk_ScriptQuerySlot
{
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<const UScriptStruct> _StructType = nullptr;

    UPROPERTY()
    ECk_ScriptQueryAccess _Access = ECk_ScriptQueryAccess::ReadOnly;
};

// --------------------------------------------------------------------------------------------------------------------
// The query a script processor declares: its fragment slots plus the NoEntities escape (a processor that wants to run
// every tick with no entity join). Built by the generated driver's Configure (slots inferred from the ForEachEntity
// signature) merged with the dev class's optional Configure (Require/Exclude/NoEntities).
USTRUCT(BlueprintType)
struct CKECS_API FCk_ScriptProcessorQuery
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FCk_ScriptQuerySlot> _Slots;

    UPROPERTY()
    bool _NoEntities = false;
};

// --------------------------------------------------------------------------------------------------------------------
// Opaque, by-value handle to the host wrapper's per-tick native join state. Passed into the generated driver's
// ForEachBatch. _State outlives the call by construction (owned by the wrapper). _Generation is captured at hand-off
// and compared against the live state on every accessor, so a batch stashed and used past the call ensures+sentinels
// instead of reading freed slots. Non-UPROPERTY members: transient plumbing, never reflected/serialized/GC-tracked.
USTRUCT(BlueprintType)
struct CKECS_API FCk_ScriptQueryBatch
{
    GENERATED_BODY()

    FCk_ScriptQueryBatchState* _State = nullptr;
    uint64 _Generation = 0;
};
