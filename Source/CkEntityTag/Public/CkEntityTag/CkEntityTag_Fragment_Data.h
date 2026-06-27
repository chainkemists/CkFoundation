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

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYTAG_API FCk_Fragment_EntityTag_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_EntityTag_ParamsData);

    // Tier-A snapshot opt-in: the per-tag NAMED storage (keyed by GetTypeHash(_Tag)) drives ForEach_Entity(tag).
    // Registering it (CkEntityTag_Fragment.cpp) makes every per-tag storage round-trip so tag membership survives
    // a save/load. Only _Tag is reflected (no handles) -> SerializeItem captures it with no hand-written code.
    using IsSnapshotable = void;

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
