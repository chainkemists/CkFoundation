#pragma once

#include "Iris/ReplicationState/ReplicationStateDescriptor.h"
#include "Iris/Serialization/NetSerializer.h"
#include "Containers/LruCache.h"

#include "Iris/Serialization/NetSerializerArrayStorage.h"

#include "UObject/ObjectMacros.h"

#include "InstancedStructNetSerializer.generated.h"

// NOTES:
// This (and the cpp file) is cobbled together from various sources and is complete just enough to work for serializing instanced structs.
// It is not a complete implementation and is not intended to be used as a reference for how to implement a net serializer.
// It is also not intended to be used as a base class for other net serializers.
// Once we have a proper implementation of instanced struct replication in Unreal 5.5, this should be removed.

namespace UE::Net
{

// --------------------------------------------------------------------------------------------------------------------

class FNetSerializerAlignedStorage
{
public:
    // Use whatever SizeType that is typically used by FNetSerializerArrayStorage
    typedef typename AllocationPolicies::FElementAllocationPolicy::SizeType SizeType;

public:
    CKECS_API auto
    AdjustSize(
        FNetSerializationContext& Context,
        SizeType InNum,
        SizeType InAlignment) -> void;

    CKECS_API auto
    Free(
        FNetSerializationContext& Context) -> void;

    CKECS_API auto
    Clone(
        FNetSerializationContext& Context,
        const FNetSerializerAlignedStorage& Source) -> void;

    inline auto
    GetData() const -> const uint8*;

    inline auto
    GetData() -> uint8*;

    inline auto
    Num() const -> SizeType;

    inline auto
    GetAlignment() const -> SizeType;

private:
    uint8* Data = nullptr;
    SizeType StorageNum = 0;
    SizeType StorageMaxCapacity = 0;
    SizeType StorageAlignment = 0;
};

}

// --------------------------------------------------------------------------------------------------------------------

class UScriptStruct;

namespace UE::Net::Private
{

class FInstancedStructDescriptorCache
{
public:
    FInstancedStructDescriptorCache();
    ~FInstancedStructDescriptorCache();

    auto
    SetDebugName(
        const FString& DebugName) -> void;

    auto
    SetMaxCachedDescriptorCount(
        int32 MaxCount) -> void;

    auto
    AddSupportedTypes(
        const TConstArrayView<TSoftObjectPtr<UScriptStruct>>& SupportedTypes) -> void;

    auto
    IsSupportedType(
        const UScriptStruct* Struct) const -> bool;

    // Find descriptor for struct with fully qualified name.
    auto
    FindDescriptor(
        FName StructPath) -> TRefCountPtr<const FReplicationStateDescriptor>;

    // Find or create descriptor for struct with fully qualified name.
    auto
    FindOrAddDescriptor(
        FName StructPath) -> TRefCountPtr<const FReplicationStateDescriptor>;

    // Find or create descriptor for struct.
    auto
    FindOrAddDescriptor(
        const UScriptStruct* Struct) -> TRefCountPtr<const FReplicationStateDescriptor>;

private:
    auto
    CreateAndCacheDescriptor(
        const UScriptStruct* Struct,
        FName StructPath) -> TRefCountPtr<const FReplicationStateDescriptor>;

    FCriticalSection Mutex;
    TLruCache<FName, TRefCountPtr<const FReplicationStateDescriptor>> DescriptorLruCache;
    TMap<FName, TRefCountPtr<const FReplicationStateDescriptor>> DescriptorMap;
    TSet<TSoftObjectPtr<UScriptStruct>> SupportedTypes;
    FString DebugName;
    int32 MaxCachedDescriptorCount = 0;
};

}

USTRUCT()
struct FInstancedStructNetSerializerConfig : public FNetSerializerConfig
{
    GENERATED_BODY()

    FInstancedStructNetSerializerConfig();
    ~FInstancedStructNetSerializerConfig();

    FInstancedStructNetSerializerConfig(const FInstancedStructNetSerializerConfig&) = delete;
    FInstancedStructNetSerializerConfig& operator=(const FInstancedStructNetSerializerConfig&) = delete;

    UPROPERTY()
    TArray<TSoftObjectPtr<UScriptStruct>> SupportedTypes;

    UE::Net::Private::FInstancedStructDescriptorCache DescriptorCache;
};

template<>
struct TStructOpsTypeTraits<FInstancedStructNetSerializerConfig> : public TStructOpsTypeTraitsBase2<FInstancedStructNetSerializerConfig>
{
    enum
    {
        WithCopy = false
    };
};

namespace UE::Net
{

UE_NET_DECLARE_SERIALIZER(FInstancedStructNetSerializer, CKECS_API);

}
