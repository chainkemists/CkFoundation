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
// each tick by the host before ForEachBatch; _Entities is snapshotted at join time; _Generation is bumped by the host
// AFTER ForEachBatch returns, invalidating any batch the script stashed past the call.
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
// AngelScript mixin over FCk_ScriptQueryBatch. The generated driver's ForEachBatch reads:
//   for (int32 i = 0; i < Batch.Num(); ++i) { FFragment_X& P = Batch.Get(i, FFragment_X); ... Batch.GetHandle(i) ... }
// Num/GetHandle are ordinary ScriptMixin UFUNCTIONs; Get returns a wildcard whose concrete type is the struct arg, so
// it is hand-bound like UCk_Utils_DynamicFragment_UE::Get_Fragment (a UFUNCTION cannot express that). Every accessor
// compares the batch's captured generation against the live state and ensures+sentinels (Get) / returns empty
// (Num) / returns an invalid handle (GetHandle) if the batch was stashed past its ForEachBatch call.
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
#endif
};
