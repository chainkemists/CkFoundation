#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkTagSet_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKTAGSET_API FCk_Handle_TagSet : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_TagSet); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_TagSet);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTAGSET_API FCk_Request_TagSet_AddTags : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_TagSet_AddTags);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_TagSet_AddTags);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _TagsToAdd;

public:
    CK_PROPERTY_GET(_TagsToAdd);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_TagSet_AddTags, _TagsToAdd);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKTAGSET_API FCk_Request_TagSet_RemoveTags : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_TagSet_RemoveTags);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_TagSet_RemoveTags);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _TagsToRemove;

public:
    CK_PROPERTY_GET(_TagsToRemove);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_TagSet_RemoveTags, _TagsToRemove);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_TagSet_OnTagsChanged,
    FCk_Handle_TagSet, InTagSet,
    const FGameplayTagContainer&, InTagsAdded,
    const FGameplayTagContainer&, InTagsRemoved);

// --------------------------------------------------------------------------------------------------------------------
