#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Processor/CkProcessor_ScriptQuery_Data.h"

#include "CkDynamic/CkDynamic_Fragment.h"   // ck::FFragment_DynamicFragment_Data (entt storage payload)

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkDynamic_ScriptQueryBatch.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FScriptStructWildcard;

// --------------------------------------------------------------------------------------------------------------------
// Native per-tick state owned by FProcessor_ScriptQueryHosted. NOT a USTRUCT and NOT exposed to script — it holds
// raw entt-storage pointers; script sees only FCk_ScriptQueryBatch (CkEcs), an opaque pointer plus a generation.
struct CKDYNAMIC_API FCk_ScriptQueryBatchState
{
    struct FSlot
    {
        const UScriptStruct*                               _Type = nullptr;
        entt::storage<ck::FFragment_DynamicFragment_Data>* _Storage = nullptr;   // null for Exclude slots
        ECk_ScriptQueryAccess                              _Access = ECk_ScriptQueryAccess::ReadOnly;
    };

    TArray<FSlot, TInlineAllocator<8>> _Slots;
    TArray<entt::entity>               _Entities;    // persistent across ticks; Reset()+Append, never shrink
    FCk_Handle                         _AnyHandle;   // transient-entity handle for MakeHandle / registry access
    uint64                             _Generation = 0;
};

// --------------------------------------------------------------------------------------------------------------------

namespace ck::dynamic
{
    // Generations are process-unique, so reopening the same state address cannot revive a stashed batch.
    CKDYNAMIC_API auto
    Open_ScriptQueryBatchState(
        FCk_ScriptQueryBatchState& InState) -> uint64;

    CKDYNAMIC_API auto
    Close_ScriptQueryBatchState(
        FCk_ScriptQueryBatchState& InState,
        uint64 InGeneration) -> void;

    // The batch's opaque pointer is never dereferenced unless its generation is still registered.
    CKDYNAMIC_API auto
    Resolve_ScriptQueryBatchState(
        const FCk_ScriptQueryBatch& InBatch) -> FCk_ScriptQueryBatchState*;

    // All-or-nothing: validates the complete slot list before any caller publishes a hash or resolves a storage.
    CKDYNAMIC_API auto
    Validate_ScriptQuerySlots(
        const FCk_ScriptProcessorQuery& InQuery,
        const TCHAR* InContext) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
// AngelScript mixin over FCk_ScriptQueryBatch. Get returns a wildcard whose concrete type is the struct arg,
// which a UFUNCTION cannot express — so it is hand-bound in the .cpp rather than declared via ScriptMixin.
UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_ScriptQueryBatch"))
class CKDYNAMIC_API UCk_ScriptQueryBatch_Mixin_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "Ck|ScriptProcessor")
    static int32
    Num(
        const FCk_ScriptQueryBatch& InBatch);

    UFUNCTION(BlueprintCallable, Category = "Ck|ScriptProcessor")
    static FCk_Handle
    GetHandle(
        const FCk_ScriptQueryBatch& InBatch,
        int32 InIndex);

public:
#if WITH_ANGELSCRIPT_CK
    static auto
    Get(
        const FCk_ScriptQueryBatch& InBatch,
        int32 InIndex,
        const UScriptStruct* InType) -> FScriptStructWildcard&;

    static auto
    GetReadOnly(
        const FCk_ScriptQueryBatch& InBatch,
        int32 InIndex,
        const UScriptStruct* InType) -> const FScriptStructWildcard&;
#endif
};
