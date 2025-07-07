#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTags.h>

#include "CkEntityCollection_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKENTITYCOLLECTION_API FCk_Handle_EntityCollection : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_EntityCollection); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_EntityCollection);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYCOLLECTION_API FCk_Fragment_EntityCollection_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_EntityCollection_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "EntityCollection"))
    FGameplayTag _Name;

public:
    CK_PROPERTY_GET(_Name)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_EntityCollection_ParamsData, _Name);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYCOLLECTION_API FCk_Fragment_MultipleEntityCollection_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleEntityCollection_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Name"))
    TArray<FCk_Fragment_EntityCollection_ParamsData> _EntityCollectionParams;

public:
    CK_PROPERTY_GET(_EntityCollectionParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleEntityCollection_ParamsData, _EntityCollectionParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYCOLLECTION_API FCk_EntityCollection_Content
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityCollection_Content);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "EntityCollection"))
    FGameplayTag _CollectionName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle> _EntitiesInCollection;

public:
    CK_PROPERTY(_CollectionName);
    CK_PROPERTY(_EntitiesInCollection);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_EntityCollection_Content, _CollectionName, _EntitiesInCollection);
};

CK_DEFINE_CUSTOM_FORMATTER_INLINE(FCk_EntityCollection_Content, [](const FCk_EntityCollection_Content& InObj)
{
    return ck::Format(TEXT("Collection: {} | {}"), InObj.Get_CollectionName(), InObj.Get_EntitiesInCollection().Num());
});

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYCOLLECTION_API FCk_Request_EntityCollection_AddEntities : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityCollection_AddEntities);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_EntityCollection_AddEntities);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle> _EntitiesToAdd;

public:
    CK_PROPERTY(_EntitiesToAdd);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityCollection_AddEntities, _EntitiesToAdd);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYCOLLECTION_API FCk_Request_EntityCollection_RemoveEntities : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityCollection_RemoveEntities);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_EntityCollection_RemoveEntities);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle> _EntitiesToRemove;

public:
    CK_PROPERTY(_EntitiesToRemove);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityCollection_RemoveEntities, _EntitiesToRemove);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_EntityCollection_OnCollectionUpdated,
    FCk_Handle_EntityCollection, InEntityCollection,
    const TArray<FCk_Handle>&, InPreviousContent,
    const TArray<FCk_Handle>&, InCurrentContent);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_EntityCollection_OnCollectionUpdated_MC,
    FCk_Handle_EntityCollection, InEntityCollection,
    const TArray<FCk_Handle>&, InPreviousContent,
    const TArray<FCk_Handle>&, InCurrentContent);


// --------------------------------------------------------------------------------------------------------------------