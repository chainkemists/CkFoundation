#pragma once

#include "CkTagSet/CkTagSet_Fragment.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Net/CkNet_Utils.h"

#include "CkEcs/Signal/CkSignal_Fragment_Data.h"

#include "CkTagSet_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_TagSet_HandleRequests;
    class FProcessor_TagSet_Replicate;
    class FProcessor_TagSet_SyncReplication;
}

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCk_Handle_TagSet"))
class CKTAGSET_API UCk_Utils_TagSet_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_TagSet_UE);
    CK_DEFINE_CPP_CASTCHECKED_TYPESAFE(FCk_Handle_TagSet);

public:
    friend class ck::FProcessor_TagSet_HandleRequests;
    friend class ck::FProcessor_TagSet_Replicate;
    friend class ck::FProcessor_TagSet_SyncReplication;

    // ---- Creation ----

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Add Feature")
    static FCk_Handle_TagSet
    Add(
        UPARAM(ref) FCk_Handle& InHandle,
        FGameplayTagContainer InInitialTags,
        ECk_Replication InReplicates = ECk_Replication::Replicates);

    // ---- Queries ----

public:
    static bool
    Has(
        const FCk_Handle& InHandle);

private:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Cast",
              meta = (ExpandEnumAsExecs = "OutResult"))
    static FCk_Handle_TagSet
    DoCast(
        UPARAM(ref) FCk_Handle& InHandle,
        ECk_SucceededFailed& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Handle -> TagSet Handle",
              meta = (CompactNodeTitle = "<AsTagSet>", BlueprintAutocast))
    static FCk_Handle_TagSet
    DoCastChecked(
        FCk_Handle InHandle);

    UFUNCTION(BlueprintPure,
              DisplayName = "[Ck] Get Invalid TagSet Handle",
              Category = "Ck|Utils|TagSet",
              meta = (CompactNodeTitle = "INVALID_TagSetHandle", Keywords = "make"))
    static FCk_Handle_TagSet
    Get_InvalidHandle() { return {}; };

public:
    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Get Tags")
    static FGameplayTagContainer
    Get_Tags(
        const FCk_Handle_TagSet& InTagSet);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Get Num Tags")
    static int32
    Get_NumTags(
        const FCk_Handle_TagSet& InTagSet);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Has Tag")
    static bool
    HasTag(
        const FCk_Handle_TagSet& InTagSet,
        FGameplayTag InTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Has Tag Exact")
    static bool
    HasTagExact(
        const FCk_Handle_TagSet& InTagSet,
        FGameplayTag InTag);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Has Any")
    static bool
    HasAny(
        const FCk_Handle_TagSet& InTagSet,
        FGameplayTagContainer InTags);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Has Any Exact")
    static bool
    HasAnyExact(
        const FCk_Handle_TagSet& InTagSet,
        FGameplayTagContainer InTags);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Has All")
    static bool
    HasAll(
        const FCk_Handle_TagSet& InTagSet,
        FGameplayTagContainer InTags);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Has All Exact")
    static bool
    HasAllExact(
        const FCk_Handle_TagSet& InTagSet,
        FGameplayTagContainer InTags);

    // ---- Requests (Authority Only) ----

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Request Add Tags")
    static void
    Request_AddTags(
        UPARAM(ref) FCk_Handle_TagSet& InTagSet,
        const FGameplayTagContainer& InTags);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Request Add Tag")
    static void
    Request_AddTag(
        UPARAM(ref) FCk_Handle_TagSet& InTagSet,
        FGameplayTag InTag);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Request Remove Tags")
    static void
    Request_RemoveTags(
        UPARAM(ref) FCk_Handle_TagSet& InTagSet,
        const FGameplayTagContainer& InTags);

    UFUNCTION(BlueprintCallable,
              BlueprintAuthorityOnly,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[Ck][TagSet] Request Remove Tag")
    static void
    Request_RemoveTag(
        UPARAM(ref) FCk_Handle_TagSet& InTagSet,
        FGameplayTag InTag);

    // ---- Signals ----

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[TagSet] Bind to OnTagsChanged")
    static void
    BindTo_OnTagsChanged(
        UPARAM(ref) FCk_Handle_TagSet& InTagSet,
        const FCk_Delegate_TagSet_OnTagsChanged& InDelegate,
        ECk_Signal_BindingPolicy InBindingPolicy = ECk_Signal_BindingPolicy::FireIfPayloadInFlightThisFrame,
        ECk_Signal_PostFireBehavior InPostFireBehavior = ECk_Signal_PostFireBehavior::DoNothing);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|TagSet",
              DisplayName = "[TagSet] Unbind From OnTagsChanged")
    static void
    UnbindFrom_OnTagsChanged(
        UPARAM(ref) FCk_Handle_TagSet& InTagSet,
        const FCk_Delegate_TagSet_OnTagsChanged& InDelegate);

    // ---- Internal ----

private:
    static auto
    Request_TryReplicateTagSet(
        FCk_Handle_TagSet& InTagSet) -> void;
};

// --------------------------------------------------------------------------------------------------------------------
