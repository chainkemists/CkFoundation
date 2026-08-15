#pragma once

#include "CkDynamic/CkDynamic_Fragment_Data.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Request/CkRequest_Completion.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkDynamic_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FScriptStructWildcard;
struct FAngelscriptAnyStructParameter;

// --------------------------------------------------------------------------------------------------------------------

// Handle-only by construction. Passing an FInstancedStruct through a dynamic delegate routes it via
// ProcessEvent's frame buffer, so mutations reach registry storage only after the delegate returns and a
// same-tick Get_Fragment read observes stale data. Handlers resolve fragments themselves.
DECLARE_DYNAMIC_DELEGATE_OneParam(FCk_DynamicFragment_ForEachEntity,
    UPARAM(ref) FCk_Handle&, InHandle);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle"))
class CKDYNAMIC_API UCk_Utils_DynamicFragment_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_DynamicFragment_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Add Fragment (Type Unsafe)",
              meta=(BlueprintInternalUseOnly = "true"))
    static FCk_Handle
    Add_Fragment(
        UPARAM(ref) FCk_Handle& InHandle,
        const FInstancedStruct& InStructData,
        ECk_Replication InReplication = ECk_Replication::DoesNotReplicate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Add or Get Fragment (Type Unsafe)",
              meta=(DeterminesOutputType="InStructType", BlueprintInternalUseOnly = "true"))
    static FInstancedStruct&
    AddOrGet_Fragment_TypeUnsafe(
        UPARAM(ref) FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        ECk_Replication InReplication = ECk_Replication::DoesNotReplicate);

    // Failure-representable boundary for untrusted transports: nullptr when the entity, type, or schema is rejected.
    static auto
    TryAddOrGet_Fragment_TypeUnsafe(
        UPARAM(ref) FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        ECk_Replication InReplication = ECk_Replication::DoesNotReplicate) -> FInstancedStruct*;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Request Remove",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_Remove(
        UPARAM(ref) FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Request Try Remove",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static ECk_SucceededFailed
    Request_TryRemove(
        UPARAM(ref) FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Get Fragment (Type Unsafe)",
              meta=(BlueprintInternalUseOnly = "true"))
    static UPARAM(ref) FInstancedStruct&
    Get_Fragment_TypeUnsafe(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType);

    // Failure-representable read boundary: unlike the reflected reference API, it never exposes fallback storage.
    static auto
    TryGet_Fragment_TypeUnsafe(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType) -> FInstancedStruct*;

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Has Fragment")
    static bool
    Has_Fragment(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] For Each Entity With One Fragment",
              meta=(KeyWords = "get,all,fragments,1"))
    static void
    ForEach_EntityWithOneFragment(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructType,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter = ECk_DestroyFilter::IgnorePendingKill);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] For Each Entity With Two Fragments",
              meta=(KeyWords = "get,all,fragments,2"))
    static void
    ForEach_EntityWithTwoFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter = ECk_DestroyFilter::IgnorePendingKill);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] For Each Entity With Three Fragments",
              meta=(KeyWords = "get,all,fragments,3"))
    static void
    ForEach_EntityWithThreeFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const UScriptStruct* InStructTypeC,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter = ECk_DestroyFilter::IgnorePendingKill);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] For Each Entity With Four Fragments",
              meta=(KeyWords = "get,all,fragments,4"))
    static void
    ForEach_EntityWithFourFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const UScriptStruct* InStructTypeC,
        const UScriptStruct* InStructTypeD,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter = ECk_DestroyFilter::IgnorePendingKill);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] For Each Entity With Five Fragments",
              meta=(KeyWords = "get,all,fragments,5"))
    static void
    ForEach_EntityWithFiveFragments(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructTypeA,
        const UScriptStruct* InStructTypeB,
        const UScriptStruct* InStructTypeC,
        const UScriptStruct* InStructTypeD,
        const UScriptStruct* InStructTypeE,
        const FCk_DynamicFragment_ForEachEntity& InDelegate,
        ECk_DestroyFilter InFilter = ECk_DestroyFilter::IgnorePendingKill);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Get All Fragments",
              meta=(DevelopmentOnly))
    static TArray<FInstancedStruct>
    Get_AllFragments(
        const FCk_Handle& InHandle);

public:
    // True when the dynamic-fragment struct declares snapshot-transience — by deriving
    // FCk_DynamicFragment_SnapshotTransient (C++) or carrying a marker-typed field (AngelScript, which
    // cannot inherit structs). Save capture and hydration skip such fragments; fences and tooling must
    // use THIS predicate rather than re-deriving the rule.
    static auto
    Get_IsSnapshotTransient(
        const UScriptStruct* InStructType) -> bool;

    static auto
    Get_StorageId(
        const UScriptStruct* InStructType) -> entt::id_type;

    // Hash key for the scheduler's dirty-marker VERSION domain — distinct from Get_StorageId's entt storage id.
    // EVERY dynamic-fragment mutation path must bump it, or the pump short-circuit leaves the node deaf.
    static auto
    Get_DirtyMarkerHash(
        const UScriptStruct* InStructType) -> uint32;

    // Registry-wide: true if ANY entity reachable through InAnyHandle holds a dynamic fragment of InStructType.
    static auto
    Has_AnyEntityWith_Fragment(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructType) -> bool;

    // Tombstone-aware counterpart of Has_AnyEntityWith_Fragment, mirroring FCk_Registry::Has_AnyLiveEntityWith.
    // Dynamic-fragment storage is in_place_delete like every Ck pool, so the storage stays non-empty forever
    // once anything has been removed from it — O(leading holes), so keep it to change-gated checks.
    static auto
    Has_AnyLiveEntityWith_Fragment(
        const FCk_Handle& InAnyHandle,
        const UScriptStruct* InStructType) -> bool;

public:
#if WITH_ANGELSCRIPT_CK
    static auto
    Add_Fragment(
        FCk_Handle& InHandle,
        const FAngelscriptAnyStructParameter& InStructData,
        ECk_Replication InReplication = ECk_Replication::DoesNotReplicate) -> FCk_Handle;

    static auto
    AddOrGet_Fragment(
        FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        ECk_Replication InReplication = ECk_Replication::DoesNotReplicate) -> FScriptStructWildcard&;

    static auto
    Get_Fragment(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType) -> FScriptStructWildcard&;
#endif

public:
    // ---- Replication ----

    // There is no automatic change detection: after mutating a replicated fragment in place via Get_Fragment,
    // call this. Ensures the fragment was added with ECk_Replication::Replicates.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Request Mark Replication Dirty",
              meta = (AutoCreateRefTerm = "InDelegate"))
    static void
    Request_MarkReplicationDirty(
        UPARAM(ref) FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        const FCk_Delegate_Request_OnCompleted& InDelegate);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Bind To OnRepNotify")
    static void
    BindTo_OnRepNotify(
        UPARAM(ref) FCk_Handle& InHandle,
        const UScriptStruct* InStructType,
        const FCk_DynamicFragment_OnRepNotify& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Unbind From OnRepNotify")
    static void
    UnbindFrom_OnRepNotify(
        UPARAM(ref) FCk_Handle& InHandle,
        const FCk_DynamicFragment_OnRepNotify& InDelegate);

private:
    static auto
    CanSetupReplication(
        const FCk_Handle& InHandle,
        const FInstancedStruct& InStructData) -> bool;

    static auto
    DoSetupReplication(
        FCk_Handle& InHandle,
        const FInstancedStruct& InStructData) -> void;

    template<size_t N, typename T_Callback>
    static auto
    ForEachEntity_WithDynamicFragments(
        const FCk_Handle& InAnyHandle,
        const std::array<const UScriptStruct*, N>& InStructTypes,
        ECk_DestroyFilter InFilter,
        T_Callback&& InCallback) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
