#pragma once

#include "CkDynamic/CkDynamic_Fragment_Data.h"

#include "CkECS/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"
#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkDynamic_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

struct FScriptStructWildcard;
struct FAngelscriptAnyStructParameter;

// --------------------------------------------------------------------------------------------------------------------

// The ForEach delegate intentionally carries only the entity handle. Fragments are resolved
// by the handler via Handle.Get_Fragment(T), which returns a live reference into registry
// storage. Passing FInstancedStruct through a dynamic delegate would route parameters through
// ProcessEvent's frame buffer — mutations on the passed struct would only be written back to
// registry storage after the delegate returns, causing same-tick Handle.Get_Fragment reads to
// observe stale data. Keeping the delegate handle-only eliminates that hazard by construction.
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

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Request Remove")
    static void
    Request_Remove(
        UPARAM(ref) FCk_Handle& InHandle,
        const UScriptStruct* InStructType);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Request Try Remove")
    static ECk_SucceededFailed
    Request_TryRemove(
        UPARAM(ref) FCk_Handle& InHandle,
        const UScriptStruct* InStructType);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Get Fragment (Type Unsafe)",
              meta=(BlueprintInternalUseOnly = "true"))
    static UPARAM(ref) FInstancedStruct&
    Get_Fragment_TypeUnsafe(
        const FCk_Handle& InHandle,
        const UScriptStruct* InStructType);

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
    static auto
    Get_StorageId(
        const UScriptStruct* InStructType) -> entt::id_type;

    // Hash key for the scheduler's dirty-marker VERSION domain (distinct from Get_StorageId's
    // entt storage id — the two hash different representations of the struct path). The
    // script-processor host registers this value for MarkedDirtyBy script structs, and every
    // dynamic-fragment mutation path bumps the same key so the pump short-circuit observes
    // dynamic-marker changes (see FCk_Registry::BumpDirtyMarkerVersion).
    static auto
    Get_DirtyMarkerHash(
        const UScriptStruct* InStructType) -> uint32;

    // Registry-wide check: returns true if *any* entity in the registry reachable through InAnyHandle holds a
    // dynamic fragment of InStructType. Used by the script-processor scheduler wrapper to implement the
    // MarkedDirtyBy gate without CkEcs needing to depend on CkDynamic directly.
    static auto
    Has_AnyEntityWith_Fragment(
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

    // Re-replicate a replicated dynamic fragment after it was mutated in-place via Get_Fragment.
    // Ensures the fragment was added with ECk_Replication::Replicates. Host-side only (the AuthorityOnly
    // replicate processor is the real gate). No automatic change detection — call this after mutating.
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|DynamicFragment",
              DisplayName="[Ck][DynamicFragment] Request Mark Replication Dirty")
    static void
    Request_MarkReplicationDirty(
        UPARAM(ref) FCk_Handle& InHandle,
        const UScriptStruct* InStructType);

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
