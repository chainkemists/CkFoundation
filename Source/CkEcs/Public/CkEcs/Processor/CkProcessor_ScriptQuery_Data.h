#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"   // USTRUCT/UENUM/GENERATED_BODY infra + TObjectPtr + UScriptStruct

#include "CkCore/Macros/CkMacros.h"

#include "CkProcessor_ScriptQuery_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UScriptStruct;

// FCk_ScriptQueryBatchState lives in CkDynamic (it holds entt-storage pointers CkEcs must not see); CkEcs keeps
// only an opaque pointer to it, so the dependency direction CkEcs -/-> CkDynamic stays intact.
struct FCk_ScriptQueryBatchState;

// --------------------------------------------------------------------------------------------------------------------
// ReadWrite/ReadOnly feed the descriptor's RW/RO fragment sets (write-conflict detection + auto-ordering);
// Require is presence-only (no data param); Exclude rejects entities carrying the fragment.
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
// A script processor's fragment slots plus the NoEntities escape (run every tick with no entity join). Built by
// the generated driver's Configure merged with the dev class's optional one.
USTRUCT(BlueprintType)
struct CKECS_API FCk_ScriptProcessorQuery
{
    GENERATED_BODY()

    UPROPERTY()
    TArray<FCk_ScriptQuerySlot> _Slots;

    UPROPERTY()
    bool _NoEntities = false;

    // Sticky fail-closed state: Configure is an imperative sequence, so one rejected slot must invalidate the
    // WHOLE query rather than leave the added subset eligible for harvest or dispatch. Host plumbing only.
    bool _AdmissionFailed = false;
};

// --------------------------------------------------------------------------------------------------------------------
// Opaque by-value handle to the host wrapper's per-tick join state, which outlives the ForEachBatch call by
// construction. _Generation is compared on every accessor, so a batch stashed past the call ensures+sentinels
// instead of reading freed slots. Non-UPROPERTY: transient plumbing, never reflected/serialized/GC-tracked.
USTRUCT(BlueprintType)
struct CKECS_API FCk_ScriptQueryBatch
{
    GENERATED_BODY()

    FCk_ScriptQueryBatchState* _State = nullptr;
    uint64 _Generation = 0;
};
