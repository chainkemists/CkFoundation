#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"

#include <Engine/StreamableManager.h>

#include "CkResourceLoader_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_ResourceLoader_LoadingPolicy : uint8
{
    Async,
    Synchronous
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ResourceLoader_LoadingPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOURCELOADER_API FCk_ResourceLoader_ObjectReference_Soft
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ResourceLoader_ObjectReference_Soft);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FSoftObjectPath _SoftObjectPath;

public:
    CK_PROPERTY_GET(_SoftObjectPath);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ResourceLoader_ObjectReference_Soft, _SoftObjectPath)
};

auto CKRESOURCELOADER_API GetTypeHash(const FCk_ResourceLoader_ObjectReference_Soft& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOURCELOADER_API FCk_ResourceLoader_ObjectReference_Hard
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ResourceLoader_ObjectReference_Hard);

public:
    FCk_ResourceLoader_ObjectReference_Hard() = default;
    explicit FCk_ResourceLoader_ObjectReference_Hard(UObject* InObject);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TObjectPtr<UObject> _Object;

public:
    CK_PROPERTY_GET(_Object);

};

auto CKRESOURCELOADER_API GetTypeHash(const FCk_ResourceLoader_ObjectReference_Hard& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOURCELOADER_API FCk_ResourceLoader_LoadedObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ResourceLoader_LoadedObject);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_ResourceLoader_ObjectReference_Soft _ObjectReference_Soft;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_ResourceLoader_ObjectReference_Hard _ObjectReference_Hard;

private:
    TSharedPtr<FStreamableHandle> _StreamableHandle;

public:
    CK_PROPERTY_GET(_ObjectReference_Soft);
    CK_PROPERTY_GET(_ObjectReference_Hard);
    CK_PROPERTY(_StreamableHandle);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ResourceLoader_LoadedObject, _ObjectReference_Soft, _ObjectReference_Hard);
};

auto CKRESOURCELOADER_API GetTypeHash(const FCk_ResourceLoader_LoadedObject& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOURCELOADER_API FCk_ResourceLoader_LoadedObjectBatch
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ResourceLoader_LoadedObjectBatch);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_ResourceLoader_LoadedObject> _UniqueLoadedObjects;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_ResourceLoader_LoadedObject> _AllOrderedLoadedObjects;

private:
    TSharedPtr<FStreamableHandle> _StreamableHandle;

public:
    CK_PROPERTY_GET(_UniqueLoadedObjects);
    CK_PROPERTY_GET(_AllOrderedLoadedObjects);
    CK_PROPERTY(_StreamableHandle);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ResourceLoader_LoadedObjectBatch, _UniqueLoadedObjects, _AllOrderedLoadedObjects);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOURCELOADER_API FCk_ResourceLoader_PendingObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ResourceLoader_PendingObject);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_ResourceLoader_ObjectReference_Soft _ObjectReference_Soft;

private:
    TSharedPtr<FStreamableHandle> _StreamableHandle;

public:
    CK_PROPERTY_GET(_ObjectReference_Soft);
    CK_PROPERTY(_StreamableHandle);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ResourceLoader_PendingObject, _ObjectReference_Soft);
};

auto CKRESOURCELOADER_API GetTypeHash(const FCk_ResourceLoader_PendingObject& InObj) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOURCELOADER_API FCk_ResourceLoader_PendingObjectBatch
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ResourceLoader_PendingObjectBatch);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_ResourceLoader_ObjectReference_Soft> _ObjectReferences_Soft;

private:
    TSharedPtr<FStreamableHandle> _StreamableHandle;

public:
    CK_PROPERTY_GET(_ObjectReferences_Soft)
    CK_PROPERTY(_StreamableHandle);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ResourceLoader_PendingObjectBatch, _ObjectReferences_Soft);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOURCELOADER_API FCk_Request_ResourceLoader_LoadObject
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ResourceLoader_LoadObject);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_ResourceLoader_ObjectReference_Soft _ObjectReference_Soft;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_ResourceLoader_LoadingPolicy _LoadingPolicy = ECk_ResourceLoader_LoadingPolicy::Async;

public:
    CK_PROPERTY_GET(_ObjectReference_Soft);
    CK_PROPERTY(_LoadingPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ResourceLoader_LoadObject, _ObjectReference_Soft);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOURCELOADER_API FCk_Request_ResourceLoader_LoadObjectBatch
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ResourceLoader_LoadObjectBatch);

protected:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TArray<FCk_ResourceLoader_ObjectReference_Soft> _ObjectReferences_Soft;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_ResourceLoader_LoadingPolicy _LoadingPolicy = ECk_ResourceLoader_LoadingPolicy::Async;

public:
    CK_PROPERTY_GET(_ObjectReferences_Soft);
    CK_PROPERTY(_LoadingPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ResourceLoader_LoadObjectBatch, _ObjectReferences_Soft);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOURCELOADER_API FCk_Payload_ResourceLoader_OnObjectLoaded
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_ResourceLoader_OnObjectLoaded);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_ResourceLoader_LoadedObject  _LoadedObject;

public:
    CK_PROPERTY_GET(_LoadedObject);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_ResourceLoader_OnObjectLoaded, _LoadedObject);
};


// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKRESOURCELOADER_API FCk_Payload_ResourceLoader_OnObjectBatchLoaded
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Payload_ResourceLoader_OnObjectBatchLoaded);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_ResourceLoader_LoadedObjectBatch _LoadedObjectBatch;

public:
    CK_PROPERTY_GET(_LoadedObjectBatch);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Payload_ResourceLoader_OnObjectBatchLoaded, _LoadedObjectBatch);
};

// --------------------------------------------------------------------------------------------------------------------
// Delegates

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_ResourceLoader_OnObjectLoaded,
    FCk_Payload_ResourceLoader_OnObjectLoaded, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_ResourceLoader_OnObjectLoaded_MC,
    FCk_Payload_ResourceLoader_OnObjectLoaded, InPayload);

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_ResourceLoader_OnObjectBatchLoaded,
    FCk_Payload_ResourceLoader_OnObjectBatchLoaded, InPayload);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_ResourceLoader_OnObjectBatchLoaded_MC,
    FCk_Payload_ResourceLoader_OnObjectBatchLoaded, InPayload);

// --------------------------------------------------------------------------------------------------------------------
// IsValid and Formatters

CK_DEFINE_CUSTOM_FORMATTER(FCk_ResourceLoader_ObjectReference_Soft, [&]()
{
    return ck::Format(TEXT("{}"), InObj.Get_SoftObjectPath());
});

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ResourceLoader_ObjectReference_Soft, ck::IsValid_Policy_Default,
[=](const FCk_ResourceLoader_ObjectReference_Soft& InObj)
{
    return ck::IsValid(InObj.Get_SoftObjectPath());
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(FCk_ResourceLoader_ObjectReference_Hard, [&]()
{
    return ck::Format(TEXT("{}"), InObj.Get_Object());
});

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ResourceLoader_ObjectReference_Hard, ck::IsValid_Policy_Default,
[=](const FCk_ResourceLoader_ObjectReference_Hard& InObj)
{
    return ck::IsValid(InObj.Get_Object());
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(FCk_ResourceLoader_LoadedObject, [&]()
{
    return ck::Format
    (
        TEXT("ObjectReference_Soft: [{}]\nObjectReference_Hard: [{}]"),
        InObj.Get_ObjectReference_Soft(),
        InObj.Get_ObjectReference_Hard()
    );
});

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ResourceLoader_LoadedObject, ck::IsValid_Policy_Default,
[=](const FCk_ResourceLoader_LoadedObject& InObj)
{
    return ck::IsValid(InObj.Get_ObjectReference_Soft()) && ck::IsValid(InObj.Get_ObjectReference_Hard());
});

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_IS_VALID_INLINE(FCk_ResourceLoader_PendingObject, ck::IsValid_Policy_Default,
[=](const FCk_ResourceLoader_PendingObject& InObj)
{
    return ck::IsValid(InObj.Get_ObjectReference_Soft());
});

// --------------------------------------------------------------------------------------------------------------------
