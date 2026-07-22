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
// Native per-tick state owned by FProcessor_ScriptQueryHosted. NOT a USTRUCT and NOT exposed to script: it
// holds raw entt-storage pointers. The script side sees only FCk_ScriptQueryBatch (CkEcs), which carries an opaque
// pointer to one of these plus a captured generation. All lifetimes: _Storage pointers and _AnyHandle are re-resolved
// each tick by the host before ForEachBatch; _Entities is snapshotted at join time. During ForEachBatch, a
// process-lifetime resolver maps the opaque pointer + generation to this state. The mapping is removed before the
// call returns, so a stashed batch never dereferences freed state and cannot alias a later state at the same address.
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
    // Opens one synchronous ForEachBatch access window. Generations are process-unique, so reopening the same state
    // address cannot make a previously stashed batch live again.
    CKDYNAMIC_API auto
    Open_ScriptQueryBatchState(
        FCk_ScriptQueryBatchState& InState) -> uint64;

    CKDYNAMIC_API auto
    Close_ScriptQueryBatchState(
        FCk_ScriptQueryBatchState& InState,
        uint64 InGeneration) -> void;

    // Resolves through the process-lifetime table before returning the state pointer. The opaque pointer in the batch
    // is never dereferenced unless its generation is still registered.
    CKDYNAMIC_API auto
    Resolve_ScriptQueryBatchState(
        const FCk_ScriptQueryBatch& InBatch) -> FCk_ScriptQueryBatchState*;

    // Query admission is all-or-nothing at both descriptor harvest and runtime resolution. This helper validates the
    // complete slot list before either caller publishes a hash or resolves an EnTT storage.
    CKDYNAMIC_API auto
    Validate_ScriptQuerySlots(
        const FCk_ScriptProcessorQuery& InQuery,
        const TCHAR* InContext) -> bool;
}

// --------------------------------------------------------------------------------------------------------------------
// AngelScript mixin over FCk_ScriptQueryBatch. The generated driver's ForEachBatch reads:
//   for (int32 i = 0; i < Batch.Num(); ++i) { FFragment_X& P = Batch.Get(i, FFragment_X); ... Batch.GetHandle(i) ... }
// Num/GetHandle are ordinary ScriptMixin UFUNCTIONs; Get returns a wildcard whose concrete type is the struct arg, so
// it is hand-bound like UCk_Utils_DynamicFragment_UE::Get_Fragment (a UFUNCTION cannot express that). Every accessor
// resolves the batch's captured pointer + generation through the live-state table and ensures+sentinels (Get) /
// returns empty (Num) / returns an invalid handle (GetHandle) if it was stashed past its ForEachBatch call.
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
    // Wildcard accessor — mirrors the WITH_ANGELSCRIPT_CK Get_Fragment overload on UCk_Utils_DynamicFragment_UE and
    // its manual bind (SetPreviousBindArgumentDeterminesOutputType). Registered in the .cpp, not via ScriptMixin.
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
