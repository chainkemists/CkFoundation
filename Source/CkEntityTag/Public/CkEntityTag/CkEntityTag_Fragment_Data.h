#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkEntityTag_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EntityTagUpdate : uint8
{
    Added,
    Removed,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityTagUpdate);

// --------------------------------------------------------------------------------------------------------------------
//
// Save-transport payload for an entity's FName EntityTag set (v3 rebuild+hydrate, Phase 3.1).
//
// The runtime store (ck::FFragment_EntityTag_Current._Tags) holds TArray<FEntityTagCount>, and FEntityTagCount is a
// plain (non-USTRUCT) struct — it cannot ride a reflected UPROPERTY directly. So the counted FName set is flattened
// into two parallel reflected arrays. Save-only (Transport = Save): it never rides the wire, so the FCk_SaveData_
// prefix (deliberately NOT FCk_RepData_) keeps it off the RepData census ratchet (Ck.Snapshot.Meta.RepDataRestoreCoverage,
// which enumerates FCk_RepData_* only).
//
// SCOPE: only the FName _Tags set is persisted. The parallel FGameplayTag _GameplayTagCounts view is NOT captured —
// on restore each name is re-Added, which rebuilds _Tags, the per-tag EnTT storage (ForEach_Entity), and
// Has()/Has_UsingGameplayTag() (both read _Tags), but Get_AllTagsAsContainer() (which reads _GameplayTagCounts) will
// not reflect gameplay-tag-only state.
//
// --------------------------------------------------------------------------------------------------------------------

USTRUCT()
struct CKENTITYTAG_API FCk_SaveData_EntityTags
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_SaveData_EntityTags);

private:
    // Distinct tag names — one entry per unique FName tag.
    UPROPERTY()
    TArray<FName> _TagNames;

    // Per-name reference counts, parallel to _TagNames (a tag Added N times needs N removes to disappear).
    UPROPERTY()
    TArray<int32> _Counts;

public:
    CK_PROPERTY(_TagNames);
    CK_PROPERTY(_Counts);
};

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYTAG_API FCk_Fragment_EntityTag_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_EntityTag_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories="EntityTag"))
    FName _Tag;

public:
    CK_PROPERTY_GET(_Tag);

    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_EntityTag_ParamsData, _Tag);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYTAG_API FCk_Request_EntityTag_Add : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityTag_Add);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_EntityTag_Add);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories="EntityTag"))
    FName _Tag;

public:
    CK_PROPERTY_GET(_Tag);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityTag_Add, _Tag);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYTAG_API FCk_Request_EntityTag_TryRemove : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityTag_TryRemove);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_EntityTag_TryRemove);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories="EntityTag"))
    FName _Tag;

public:
    CK_PROPERTY_GET(_Tag);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityTag_TryRemove, _Tag);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYTAG_API FCk_Request_EntityTag_AddGameplayTag : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityTag_AddGameplayTag);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_EntityTag_AddGameplayTag);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories="EntityTag"))
    FGameplayTag _Tag;

public:
    CK_PROPERTY_GET(_Tag);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityTag_AddGameplayTag, _Tag);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYTAG_API FCk_Request_EntityTag_TryRemoveGameplayTag : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityTag_TryRemoveGameplayTag);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_EntityTag_TryRemoveGameplayTag);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories="EntityTag"))
    FGameplayTag _Tag;

public:
    CK_PROPERTY_GET(_Tag);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityTag_TryRemoveGameplayTag, _Tag);
};

// --------------------------------------------------------------------------------------------------------------------
//
// Save/load reconstitution request — enqueued ONLY by the EntityTag persistence handler's HydrationApply (v3 load
// path), never built from Blueprint/AngelScript, hence a plain USTRUCT() kept off the request palette (mirrors
// FCk_SaveData_EntityTags above).
//
// It carries the WHOLE saved counted FName set as two parallel arrays and is a COMPOSITE restore-set request: at
// drain time the handler (FProcessor_EntityTag_HandleRequests::DoHandleRequest) SETs the live tag set to EXACTLY this
// {name -> count} map, diffing against whatever is live AT DRAIN TIME. Diffing at drain — not at HydrationApply time —
// is the whole point. A rebuilt entity's Construct/BeginPlay may seed EntityTag tags through the same deferred Add
// requests, and those seeds are enqueued-but-not-yet-drained (FProcessor_EntityTag_HandleRequests is GatedDuringLoad)
// at the moment HydrationApply runs — so any Has<>/Get_ read of the "current" set then would be a lie. Because this
// request is enqueued AFTER those seeds onto the SAME FIFO request array, its handler runs once the seeds have
// materialised, and it reconstitutes exactly the saved set instead of MERGING onto the construct-seeds. Idempotent.
//
USTRUCT()
struct CKENTITYTAG_API FCk_Request_EntityTag_RestoreSet : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityTag_RestoreSet);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_EntityTag_RestoreSet);

private:
    // Distinct saved tag names — one entry per unique FName tag.
    UPROPERTY()
    TArray<FName> _TagNames;

    // Per-name saved reference counts, parallel to _TagNames (a tag Added N times needs N removes to disappear).
    UPROPERTY()
    TArray<int32> _Counts;

public:
    CK_PROPERTY_GET(_TagNames);
    CK_PROPERTY_GET(_Counts);

    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityTag_RestoreSet, _TagNames, _Counts);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_EntityTag_OnTagUpdated,
    FCk_Handle, InOwner,
    FName, InTagName,
    ECk_EntityTagUpdate, InUpdateType);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_EntityTag_OnGameplayTagUpdated,
    FCk_Handle, InOwner,
    FGameplayTag, InTag,
    ECk_EntityTagUpdate, InUpdateType);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_EntityTag_OnTagUpdated_AnyEntity,
    FName, InTag,
    FCk_Handle, InEntity,
    ECk_EntityTagUpdate, InUpdateType);

// --------------------------------------------------------------------------------------------------------------------
