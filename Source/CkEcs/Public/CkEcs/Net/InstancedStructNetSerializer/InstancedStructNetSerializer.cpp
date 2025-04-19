#include UE_INLINE_GENERATED_CPP_BY_NAME(InstancedStructNetSerializer)

#if UE_WITH_IRIS

#include "CkNet/InstancedStructNetSerializer/InstancedStructNetSerializer.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <InstancedStruct.h>
#include <HAL/IConsoleManager.h>
#include <Iris/Core/IrisLog.h>
#include <Iris/Core/NetObjectReference.h>
#include <Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h>
#include <Iris/ReplicationState/ReplicationStateDescriptorBuilder.h>
#include <Iris/Serialization/NetReferenceCollector.h>
#include <Iris/Serialization/NetSerializerArrayStorage.h>
#include <Iris/Serialization/NetSerializerDelegates.h>
#include <Iris/Serialization/NetSerializers.h>
#include <Templates/IsPODType.h>

// HACK - we normally cannot include this file. Until we are on 5.5, we have to do this hack
#include <Iris/Serialization/InternalNetSerializationContext.h>

// --------------------------------------------------------------------------------------------------------------------

namespace UE::Net::Private
{

    // With-editor, we need to define these functions since we can't include the cpp file. Without-editor, the static library
    // compilations result in multiply defined symbols.
#if WITH_EDITOR

auto
    FInternalNetSerializationContext::
    Alloc(
        SIZE_T Size,
        SIZE_T Alignment)
    -> void*
{
    return GMalloc->Malloc(Size, static_cast<uint32>(Alignment));
}

auto
    FInternalNetSerializationContext::
    Free(
        void* Ptr)
    -> void
{
    return GMalloc->Free(Ptr);
}

auto
    FInternalNetSerializationContext::
    Realloc(
        void* PrevAddress,
        SIZE_T NewSize,
        uint32 Alignment)
    -> void*
{
    return GMalloc->Realloc(PrevAddress, NewSize, Alignment);
}

#endif

}

// --------------------------------------------------------------------------------------------------------------------
namespace UE::Net
{

    // --------------------------------------------------------------------------------------------------------------------


auto
    FNetSerializerAlignedStorage::
    AdjustSize(
        FNetSerializationContext& Context,
        SizeType InNum,
        SizeType InAlignment)
    -> void
{
    if (!InNum)
    {
        Free(Context);
        return;
    }

    // If the allocation isn't properly aligned or if the allocation is too small we make a new allocation.
    if ((InNum > StorageMaxCapacity) || !IsAligned(Data, InAlignment))
    {
        // Can't use realloc as the alignment of the original allocation isn't tracked.
        void* NewData = Context.GetInternalContext()->Alloc(InNum, InAlignment);
        FMemory::Memzero(NewData, InNum);
        // Copy old data
        FMemory::Memcpy(NewData, Data, StorageNum);
        Context.GetInternalContext()->Free(Data);

        Data = static_cast<uint8*>(NewData);
        StorageNum = InNum;
        StorageMaxCapacity = InNum;
        StorageAlignment = InAlignment;
    }
    // Requested data size fits the current allocation
    else
    {
        // Clear capacity we're not using anymore. If we're growing we don't need to clear as it has already been cleared.
        if (InNum < StorageNum)
        {
            FMemory::Memzero(static_cast<void*>(Data + InNum), StorageNum - InNum);
        }
        StorageNum = InNum;
        StorageNum = InAlignment;
    }
}

auto
    FNetSerializerAlignedStorage::
    Free(
        FNetSerializationContext& Context)
    -> void
{
    if (Data != nullptr)
    { Context.GetInternalContext()->Free(Data); }

    Data = nullptr;
    StorageNum = 0;
    StorageMaxCapacity = 0;
    StorageAlignment = 0;
}

auto
    FNetSerializerAlignedStorage::
    Clone(
        FNetSerializationContext& Context,
        const FNetSerializerAlignedStorage& Source)
    -> void
{
    // Only allocate and copy the exact amount of memory needed.
    if (Source.StorageNum > 0)
    {
        Data = static_cast<uint8*>(Context.GetInternalContext()->Alloc(Source.StorageNum, Source.StorageAlignment));
        StorageNum = Source.StorageNum;
        StorageMaxCapacity = Source.StorageNum;
        StorageAlignment = Source.StorageAlignment;
        FMemory::Memcpy(Data, Source.Data, Source.StorageNum);
    }
    else
    {
        Data = nullptr;
        StorageNum = 0;
        StorageMaxCapacity = 0;
        StorageAlignment = 0;
    }
}

auto
    FNetSerializerAlignedStorage::
    GetData() const
    -> const uint8*
{
	return Data;
}

auto
    FNetSerializerAlignedStorage::
    GetData()
    -> uint8*
{
	return Data;
}

auto
    FNetSerializerAlignedStorage::
    Num() const
    -> FNetSerializerAlignedStorage::SizeType
{
	return StorageNum;
}

auto
    FNetSerializerAlignedStorage::
    GetAlignment() const
    -> FNetSerializerAlignedStorage::SizeType
{
	return StorageAlignment;
}

}

// --------------------------------------------------------------------------------------------------------------------

FInstancedStructNetSerializerConfig::
    FInstancedStructNetSerializerConfig()
    : FNetSerializerConfig()
{
    ConfigTraits = ENetSerializerConfigTraits::NeedDestruction;
}

FInstancedStructNetSerializerConfig::
    ~FInstancedStructNetSerializerConfig() = default;

namespace UE::Net
{

static auto MaxCachedInstancedStructDescriptorCount = 8;
static FAutoConsoleVariableRef CVarMaxCachedInstancedStructDescriptors(TEXT("InstancedStruct.MaxCachedReplicationStateDescriptors"),
    MaxCachedInstancedStructDescriptorCount,
    TEXT("How many ReplicationStateDescriptors the InstancedStructNetSerializer is allowed to cache for InstancedStructs without a type allow list. Warning: A value <= 0 means an unlimited amount of descriptors."));

const FName NetError_InstancedStructNetSerializer_InvalidStructType("Invalid struct type");

struct FFInstancedStructNetSerializerQuantizedData
{
    FNetSerializerAlignedStorage StructData;
    FNetObjectReference StructType;

    // Not serialized. Fully qualified path. For ReplicationStateDescriptor lookup, validation etc.
    FName StructName;
    // Not serialized. To optimize away some calls like dynamic memory management and object references.
    EReplicationStateTraits StructDescriptorTraits;
};

// ReSharper disable once CppPolymorphicClassWithNonVirtualPublicDestructor
struct FInstancedStructPropertyNetSerializerInfo : public FNamedStructPropertyNetSerializerInfo
{
public:
    FInstancedStructPropertyNetSerializerInfo();

protected:
    auto
    CanUseDefaultConfig(
        const FProperty* Property) const -> bool override;

    auto
    BuildNetSerializerConfig(
        void* NetSerializerConfigBuffer,
        const FProperty* Property) const -> FNetSerializerConfig* override;
};

}

template<> struct TIsPODType<UE::Net::FFInstancedStructNetSerializerQuantizedData> { enum { Value = true }; };

namespace UE::Net
{

struct FInstancedStructNetSerializer
{
    // Version
    static constexpr uint32 Version = 0;

    // Traits
    // ReSharper disable once CppInconsistentNaming
    static constexpr bool bHasDynamicState = true;
    // ReSharper disable once CppInconsistentNaming
    static constexpr bool bIsForwardingSerializer = true;
    // ReSharper disable once CppInconsistentNaming
    static constexpr bool bHasCustomNetReference = true;

    typedef FInstancedStruct SourceType;
    typedef FFInstancedStructNetSerializerQuantizedData QuantizedType;
    typedef FInstancedStructNetSerializerConfig ConfigType;

    static auto
    Serialize(
        FNetSerializationContext&,
        const FNetSerializeArgs&) -> void;

    static auto
    Deserialize(
        FNetSerializationContext&,
        const FNetDeserializeArgs&) -> void;

    static auto
    SerializeDelta(
        FNetSerializationContext&,
        const FNetSerializeDeltaArgs& Args) -> void;

    static auto
    DeserializeDelta(
        FNetSerializationContext&,
        const FNetDeserializeDeltaArgs& Args) -> void;

    static auto
    Quantize(
        FNetSerializationContext&,
        const FNetQuantizeArgs&) -> void;

    static auto
    Dequantize(
        FNetSerializationContext&,
        const FNetDequantizeArgs&) -> void;

    static auto
    IsEqual(
        FNetSerializationContext&,
        const FNetIsEqualArgs&) -> bool;

    static auto
    Validate(
        FNetSerializationContext&,
        const FNetValidateArgs&) -> bool;

    static auto
    CloneDynamicState(
        FNetSerializationContext&,
        const FNetCloneDynamicStateArgs&) -> void;

    static auto
    FreeDynamicState(
        FNetSerializationContext&,
        const FNetFreeDynamicStateArgs&) -> void;

    static auto
    CollectNetReferences(
        FNetSerializationContext&,
        const FNetCollectReferencesArgs&) -> void;

private:
    static auto
    FreeStructInstance(
        FNetSerializationContext&,
        FInstancedStructNetSerializerConfig*,
        QuantizedType& Value) -> void;

    static auto
    Reset(
        FNetSerializationContext&,
        FInstancedStructNetSerializerConfig*,
        QuantizedType&) -> void;

    static auto
    InternalFreeStructInstance(
        FNetSerializationContext&,
        FInstancedStructNetSerializerConfig*,
        QuantizedType&) -> void;

    // --------------------------------------------------------------------------------------------------------------------

    class FNetSerializerRegistryDelegates final : private UE::Net::FNetSerializerRegistryDelegates
    {
    public:
        virtual
        ~FNetSerializerRegistryDelegates() override;

    private:
        auto
        OnPreFreezeNetSerializerRegistry() -> void override;

        UE_NET_IMPLEMENT_NETSERIALIZER_INFO(FInstancedStructPropertyNetSerializerInfo);
    };

    inline static auto StructNetSerializer = &UE_NET_GET_SERIALIZER(FStructNetSerializer);
    inline static auto ObjectNetSerializer = &UE_NET_GET_SERIALIZER(FObjectNetSerializer);
    inline static FNetSerializerRegistryDelegates NetSerializerRegistryDelegates;
};

UE_NET_IMPLEMENT_SERIALIZER(FInstancedStructNetSerializer);

// --------------------------------------------------------------------------------------------------------------------

FInstancedStructNetSerializer::FNetSerializerRegistryDelegates::
    ~FNetSerializerRegistryDelegates()
{
    UE_NET_UNREGISTER_NETSERIALIZER_INFO(FInstancedStructPropertyNetSerializerInfo);
}

auto
    FInstancedStructNetSerializer::
    FNetSerializerRegistryDelegates::OnPreFreezeNetSerializerRegistry()
    -> void
{
    UE_NET_REGISTER_NETSERIALIZER_INFO(FInstancedStructPropertyNetSerializerInfo);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FInstancedStructNetSerializer::
    Serialize(
        FNetSerializationContext& Context,
        const FNetSerializeArgs& Args)
    -> void
{
    const QuantizedType& Value = *reinterpret_cast<QuantizedType*>(Args.Source);

    FInstancedStructNetSerializerConfig* Config = const_cast<FInstancedStructNetSerializerConfig*>(static_cast<const FInstancedStructNetSerializerConfig*>(Args.NetSerializerConfig));

    FNetBitStreamWriter* Writer = Context.GetBitStreamWriter();

    FStructNetSerializerConfig StructConfig;
    if (!Value.StructName.IsNone())
    {
        StructConfig.StateDescriptor = Config->DescriptorCache.FindOrAddDescriptor(Value.StructName);
        ensureMsgf(StructConfig.StateDescriptor.IsValid(), TEXT("Struct type is no longer resolvable: %s. Sending FInstancedStruct as uninitialized."), ToCStr(Value.StructName.ToString()));
    }

    if (Writer->WriteBool(StructConfig.StateDescriptor.IsValid()))
    {
        // Serialize struct type
        {
            FNetSerializeArgs SerializeArgs = Args;
            SerializeArgs.Source = reinterpret_cast<NetSerializerValuePointer>(&Value.StructType);
            SerializeArgs.NetSerializerConfig = ObjectNetSerializer->DefaultConfig;
            ObjectNetSerializer->Serialize(Context, SerializeArgs);
        }

        // Serialize struct data
        {
            FNetSerializeArgs SerializeArgs = Args;
            SerializeArgs.Source = reinterpret_cast<NetSerializerValuePointer>(Value.StructData.GetData());
            SerializeArgs.NetSerializerConfig = static_cast<NetSerializerConfigParam>(&StructConfig);
            StructNetSerializer->Serialize(Context, SerializeArgs);
        }
    }
}

auto
    FInstancedStructNetSerializer::
    Deserialize(
        FNetSerializationContext& Context,
        const FNetDeserializeArgs& Args)
    -> void
{
    QuantizedType& Value = *reinterpret_cast<QuantizedType*>(Args.Target);

    FInstancedStructNetSerializerConfig* Config = const_cast<FInstancedStructNetSerializerConfig*>(static_cast<const FInstancedStructNetSerializerConfig*>(Args.NetSerializerConfig));

    FNetBitStreamReader* Reader = Context.GetBitStreamReader();
    // Was the instanced struct valid on the sending side?
    if (Reader->ReadBool())
    {
        auto StructType = FNetObjectReference{};
        const UScriptStruct* Struct = nullptr;

        {
            FNetDeserializeArgs DeserializeArgs = Args;
            DeserializeArgs.Target = reinterpret_cast<NetSerializerValuePointer>(&StructType);
            DeserializeArgs.NetSerializerConfig = ObjectNetSerializer->DefaultConfig;
            ObjectNetSerializer->Deserialize(Context, DeserializeArgs);
        }

        {
            UObject* Object = nullptr;

            FNetDequantizeArgs DequantizeArgs = {};
            DequantizeArgs.Source = reinterpret_cast<NetSerializerValuePointer>(&StructType);
            DequantizeArgs.Target = reinterpret_cast<NetSerializerValuePointer>(&Object);
            DequantizeArgs.NetSerializerConfig = ObjectNetSerializer->DefaultConfig;
            ObjectNetSerializer->Dequantize(Context, DequantizeArgs);

            if (Object != nullptr)
            {
                Struct = Cast<UScriptStruct>(Object);
                ensureMsgf(Struct != nullptr, TEXT("Unable to cast object %s to UScriptStruct"), ToCStr(Object->GetPathName()));
                if (Struct == nullptr)
                {
                    Context.SetError(GNetError_InvalidValue);
                    return;
                }
            }
        }

        // Deserialize struct data
        if (ensureMsgf(Struct != nullptr, TEXT("Unable to find struct using NetObjectReference %s"), ToCStr(StructType.ToString())))
        {
            FStructNetSerializerConfig StructConfig;
            StructConfig.StateDescriptor = Config->DescriptorCache.FindOrAddDescriptor(Struct);
            if (!StructConfig.StateDescriptor.IsValid())
            {
                ensureMsgf(StructConfig.StateDescriptor.IsValid(), TEXT("Unable to create ReplicationStateDescriptor for struct %s."), ToCStr(Struct->GetPathName()));
                Context.SetError(GNetError_InvalidValue);
                return;
            }

            // If we changed types we need to free the struct, adjust the storage size and update struct info.
            if (StructType != Value.StructType)
            {
                FreeStructInstance(Context, Config, Value);
                Value.StructData.AdjustSize(Context, StructConfig.StateDescriptor->InternalSize, StructConfig.StateDescriptor->InternalAlignment);
                Value.StructType = StructType;

                Value.StructDescriptorTraits = StructConfig.StateDescriptor->Traits;
                const FString& PathNameString = Struct->GetPathName();
                Value.StructName = FName(PathNameString);
            }

            FNetDeserializeArgs DeserializeArgs = Args;
            DeserializeArgs.Target = reinterpret_cast<NetSerializerValuePointer>(Value.StructData.GetData());
            DeserializeArgs.NetSerializerConfig = static_cast<NetSerializerConfigParam>(&StructConfig);
            StructNetSerializer->Deserialize(Context, DeserializeArgs);
        }
        else
        {
            Context.SetError(GNetError_InvalidValue);
            return;
        }
    }
    else
    {
        Reset(Context, Config, Value);
    }
}

auto
    FInstancedStructNetSerializer::
    SerializeDelta(
        FNetSerializationContext& Context,
        const FNetSerializeDeltaArgs& Args)
    -> void
{
    Serialize(Context, Args);
}

auto
    FInstancedStructNetSerializer::
    DeserializeDelta(
        FNetSerializationContext& Context,
        const FNetDeserializeDeltaArgs& Args)
    -> void
{
    Deserialize(Context, Args);
}

auto
    FInstancedStructNetSerializer::
    Quantize(
        FNetSerializationContext& Context,
        const FNetQuantizeArgs& Args)
    -> void
{
    const auto& Source = *reinterpret_cast<SourceType*>(Args.Source);
    auto& Target = *reinterpret_cast<QuantizedType*>(Args.Target);

    auto Config = const_cast<FInstancedStructNetSerializerConfig*>(static_cast<const FInstancedStructNetSerializerConfig*>(Args.NetSerializerConfig));

    if (Source.IsValid())
    {
        auto Struct = Source.GetScriptStruct();

        auto DescriptorRef = Config->DescriptorCache.FindOrAddDescriptor(Struct);
        if (ensureMsgf(DescriptorRef.IsValid(), TEXT("Unable to create descriptor for struct %s. Unexpected."), ToCStr(Struct->GetFullName())))
        {
            const auto& PathNameString = Struct->GetPathName();
            const auto PathName = FName{PathNameString};
            // If the struct type is the same as previous instance we don't need to free memory or adjust allocations.
            if (PathName != Target.StructName)
            {
                // We need to free the previous struct data prior to overwriting it. Doing this early.
                FreeStructInstance(Context, Config, Target);

                Target.StructData.AdjustSize(Context, DescriptorRef->InternalSize, DescriptorRef->InternalAlignment);
                Target.StructName = PathName;
                Target.StructDescriptorTraits = DescriptorRef->Traits;

                {
                    FNetQuantizeArgs QuantizeArgs = Args;
                    QuantizeArgs.Source = reinterpret_cast<NetSerializerValuePointer>(&Struct);
                    QuantizeArgs.Target = reinterpret_cast<NetSerializerValuePointer>(&Target.StructType);
                    QuantizeArgs.NetSerializerConfig = ObjectNetSerializer->DefaultConfig;
                    ObjectNetSerializer->Quantize(Context, QuantizeArgs);
                }
            }

            // Quantize the struct instance into the target memory.
            if (Target.StructData.Num() > 0)
            {
                FStructNetSerializerConfig StructConfig;
                StructConfig.StateDescriptor = DescriptorRef;

                FNetQuantizeArgs QuantizeArgs = Args;
                QuantizeArgs.Source = reinterpret_cast<NetSerializerValuePointer>(Source.GetMemory());
                QuantizeArgs.Target = reinterpret_cast<NetSerializerValuePointer>(Target.StructData.GetData());
                QuantizeArgs.NetSerializerConfig = static_cast<NetSerializerConfigParam>(&StructConfig);
                StructNetSerializer->Quantize(Context, QuantizeArgs);
            }

            return;
        }
    }

    // Path taken for uninitialized FInstancedStruct or if an error was detected.
    Reset(Context, Config, Target);
}

auto
    FInstancedStructNetSerializer::
    Dequantize(
        FNetSerializationContext& Context,
        const FNetDequantizeArgs& Args)
    -> void
{
    const auto& [StructData, StructType, StructName, StructDescriptorTraits] = *reinterpret_cast<QuantizedType*>(Args.Source);
    auto& Target = *reinterpret_cast<SourceType*>(Args.Target);

    FInstancedStructNetSerializerConfig* Config = const_cast<FInstancedStructNetSerializerConfig*>(static_cast<const FInstancedStructNetSerializerConfig*>(Args.NetSerializerConfig));

    if (!StructType.IsValid())
    {
        Target.Reset();
        return;
    }

    const UScriptStruct* Struct = nullptr;
    {
        UObject* Object = nullptr;

        FNetDequantizeArgs DequantizeArgs = {};
        DequantizeArgs.Source = reinterpret_cast<NetSerializerValuePointer>(&StructType);
        DequantizeArgs.Target = reinterpret_cast<NetSerializerValuePointer>(&Object);
        DequantizeArgs.NetSerializerConfig = ObjectNetSerializer->DefaultConfig;
        ObjectNetSerializer->Dequantize(Context, DequantizeArgs);

        if (Object != nullptr)
        {
            Struct = Cast<UScriptStruct>(Object);
            ensureMsgf(Struct != nullptr, TEXT("Unable to cast object %s to UScriptStruct"), ToCStr(Object->GetPathName()));
        }
    }

    if (ensureMsgf(Struct != nullptr, TEXT("Unable to find struct using NetObjectReference %s"), ToCStr(StructType.ToString())))
    {
        // Re-initialize if the type changes.
        if (Struct != Target.GetScriptStruct())
        {
            Target.InitializeAs(Struct);
        }

        FStructNetSerializerConfig StructConfig;
        StructConfig.StateDescriptor = Config->DescriptorCache.FindOrAddDescriptor(Struct);

        if (ensureMsgf(StructConfig.StateDescriptor.IsValid(), TEXT("Unable to create ReplicationStateDescriptor for struct %s."), ToCStr(StructName.ToString())))
        {
            FNetDequantizeArgs DequantizeArgs = Args;
            DequantizeArgs.NetSerializerConfig = &StructConfig;
            DequantizeArgs.Source = NetSerializerValuePointer(StructData.GetData());
            DequantizeArgs.Target = NetSerializerValuePointer(Target.GetMutableMemory());
            StructNetSerializer->Dequantize(Context, DequantizeArgs);
        }
    }
}

auto
    FInstancedStructNetSerializer::
    IsEqual(
        FNetSerializationContext& Context,
        const FNetIsEqualArgs& Args)
    -> bool
{
    if (Args.bStateIsQuantized)
    {
        const auto& Value0 = *reinterpret_cast<const QuantizedType*>(Args.Source0);
        const auto& Value1 = *reinterpret_cast<const QuantizedType*>(Args.Source1);

        if (Value0.StructData.Num() != Value1.StructData.Num())
        {
            return false;
        }

        if (Value0.StructType != Value1.StructType)
        {
            return false;
        }

        if (FMemory::Memcmp(Value0.StructData.GetData(), Value1.StructData.GetData(), Value0.StructData.Num()) != 0)
        {
            return false;
        }

        return true;
    }
    else
    {
        const auto& Value0 = *reinterpret_cast<const SourceType*>(Args.Source0);
        const auto& Value1 = *reinterpret_cast<const SourceType*>(Args.Source1);
        return Value0 == Value1;
    }
}

auto
    FInstancedStructNetSerializer::
    Validate(
        FNetSerializationContext& Context,
        const FNetValidateArgs& Args)
    -> bool
{
    const SourceType& Value = *reinterpret_cast<const SourceType*>(Args.Source);

    return true;
}

auto
    FInstancedStructNetSerializer::
    CloneDynamicState(
        FNetSerializationContext& Context,
        const FNetCloneDynamicStateArgs& Args)
    -> void
{
    const auto& Source = *reinterpret_cast<const QuantizedType*>(Args.Source);
    auto& Target = *reinterpret_cast<QuantizedType*>(Args.Target);
    auto Config = const_cast<FInstancedStructNetSerializerConfig*>(static_cast<const FInstancedStructNetSerializerConfig*>(Args.NetSerializerConfig));

    Target.StructData.Clone(Context, Source.StructData);

    if (EnumHasAnyFlags(Source.StructDescriptorTraits, EReplicationStateTraits::HasDynamicState))
    {
        auto StructConfig = FStructNetSerializerConfig{};
        StructConfig.StateDescriptor = Config->DescriptorCache.FindOrAddDescriptor(Source.StructName);

        if (ensureMsgf(StructConfig.StateDescriptor.IsValid(),
            TEXT("Unable to create ReplicationStateDescriptor for struct %s."), ToCStr(Source.StructName.ToString())))
        {
            FNetCloneDynamicStateArgs CloneArgs = Args;
            CloneArgs.NetSerializerConfig = &StructConfig;
            CloneArgs.Source = reinterpret_cast<NetSerializerValuePointer>(Source.StructData.GetData());
            CloneArgs.Target = reinterpret_cast<NetSerializerValuePointer>(Target.StructData.GetData());
            StructNetSerializer->CloneDynamicState(Context, CloneArgs);
        }
    }
}

auto
    FInstancedStructNetSerializer::
    FreeDynamicState(
        FNetSerializationContext& Context,
        const FNetFreeDynamicStateArgs& Args)
    -> void
{
    auto& Value = *reinterpret_cast<QuantizedType*>(Args.Source);
    const auto Config = const_cast<FInstancedStructNetSerializerConfig*>(static_cast<const FInstancedStructNetSerializerConfig*>(Args.NetSerializerConfig));

    InternalFreeStructInstance(Context, Config, Value);
    Value.StructData.Free(Context);
}

auto
    FInstancedStructNetSerializer::
    CollectNetReferences(
        FNetSerializationContext& Context,
        const FNetCollectReferencesArgs& Args)
    -> void
{
    const auto& [StructData, StructType, StructName, StructDescriptorTraits] = *reinterpret_cast<QuantizedType*>(Args.Source);
    const auto Config = const_cast<FInstancedStructNetSerializerConfig*>(static_cast<const FInstancedStructNetSerializerConfig*>(Args.NetSerializerConfig));

    if (StructType.IsValid())
    {
        FNetReferenceCollector& Collector = *reinterpret_cast<FNetReferenceCollector*>(Args.Collector);

        // What's the proper reference type?
        const auto ReferenceInfo = FNetReferenceInfo{FNetReferenceInfo::EResolveType::ResolveOnClient};
        Collector.Add(ReferenceInfo, StructType, Args.ChangeMaskInfo);
    }

    if (EnumHasAnyFlags(StructDescriptorTraits, EReplicationStateTraits::HasObjectReference))
    {
        auto StructConfig = FStructNetSerializerConfig{};
        StructConfig.StateDescriptor = Config->DescriptorCache.FindOrAddDescriptor(StructName);

        if (ensureMsgf(StructConfig.StateDescriptor.IsValid(),
            TEXT("Unable to create ReplicationStateDescriptor for struct %s."), ToCStr(StructName.ToString())))
        {
            FNetCollectReferencesArgs CollectReferencesArgs = Args;
            CollectReferencesArgs.NetSerializerConfig = &StructConfig;
            CollectReferencesArgs.Source = reinterpret_cast<NetSerializerValuePointer>(StructData.GetData());
            StructNetSerializer->CollectNetReferences(Context, CollectReferencesArgs);
        }
    }
}

auto
    FInstancedStructNetSerializer::
    FreeStructInstance(
        FNetSerializationContext& Context,
        FInstancedStructNetSerializerConfig* Config,
        QuantizedType& Value)
    -> void
{
    InternalFreeStructInstance(Context, Config, Value);
    if (Value.StructData.Num() > 0)
    {
        FMemory::Memzero(Value.StructData.GetData(), Value.StructData.Num());
    }
}

auto
    FInstancedStructNetSerializer::
    Reset(
        FNetSerializationContext& Context,
        FInstancedStructNetSerializerConfig* Config,
        QuantizedType& Value)
    -> void
{
    InternalFreeStructInstance(Context, Config, Value);
    Value.StructData.Free(Context);

    FMemory::Memzero(&Value, sizeof(QuantizedType));
}

auto
    FInstancedStructNetSerializer::
    InternalFreeStructInstance(
        FNetSerializationContext& Context,
        FInstancedStructNetSerializerConfig* Config,
        QuantizedType& Value)
    -> void
{
    if (Value.StructData.Num() > 0 && EnumHasAnyFlags(Value.StructDescriptorTraits, EReplicationStateTraits::HasDynamicState))
    {
        FStructNetSerializerConfig StructConfig;
        StructConfig.StateDescriptor = Config->DescriptorCache.FindOrAddDescriptor(Value.StructName);

        FNetFreeDynamicStateArgs FreeArgs;
        FreeArgs.NetSerializerConfig = &StructConfig;
        FreeArgs.Source = reinterpret_cast<NetSerializerValuePointer>(Value.StructData.GetData());

        StructNetSerializer->FreeDynamicState(Context, FreeArgs);
    }
}

}

namespace UE::Net
{

void InitInstancedStructNetSerializerConfig(FInstancedStructNetSerializerConfig* Config, const FProperty* Property)
{
    Config->SupportedTypes.Reset();

    {
        FString DebugName;
        DebugName.Reserve(256);

        FFieldVariant Owner = Property->GetOwnerVariant();
        if (const UObject* Object = Owner.ToUObject())
        {
            DebugName.Append(Object->GetName()).AppendChar(TEXT('.'));
        }
        else if (const FField* Field = Owner.ToField())
        {
            DebugName.Append(Field->GetName()).AppendChar(TEXT('.'));
        }
        DebugName.Append(Property->GetName());

        Config->DescriptorCache.SetDebugName(DebugName);
    }

    // Add supported type info to the cache.
    Config->DescriptorCache.AddSupportedTypes(TConstArrayView<TSoftObjectPtr<UScriptStruct>>(Config->SupportedTypes));

    const bool bIsAllowingArbitraryStruct = true;
    if (bIsAllowingArbitraryStruct)
    {
        Config->DescriptorCache.SetMaxCachedDescriptorCount(MaxCachedInstancedStructDescriptorCount);
    }
}

FInstancedStructPropertyNetSerializerInfo::
    FInstancedStructPropertyNetSerializerInfo()
    : FNamedStructPropertyNetSerializerInfo(FName("InstancedStruct")
    , UE_NET_GET_SERIALIZER(FInstancedStructNetSerializer))
{
}

auto
    FInstancedStructPropertyNetSerializerInfo::
    CanUseDefaultConfig(
        const FProperty* Property) const
    -> bool
{
    return false;
}

auto
    FInstancedStructPropertyNetSerializerInfo::
    BuildNetSerializerConfig(
        void* NetSerializerConfigBuffer,
        const FProperty* Property) const
    -> FNetSerializerConfig*
{
    auto Config = new (NetSerializerConfigBuffer) FInstancedStructNetSerializerConfig();
    InitInstancedStructNetSerializerConfig(Config, Property);
    return Config;
}

}

namespace UE::Net::Private
{

FInstancedStructDescriptorCache::
    FInstancedStructDescriptorCache()
{
}

FInstancedStructDescriptorCache::
    ~FInstancedStructDescriptorCache()
{
}

auto
    FInstancedStructDescriptorCache::
    SetDebugName(
        const FString& InDebugName)
    -> void
{
    DebugName = InDebugName;
}

auto
    FInstancedStructDescriptorCache::
    SetMaxCachedDescriptorCount(
        int32 MaxCount)
    -> void
{
    if (MaxCount <= 0)
    {
        DescriptorLruCache.Empty(0);
        MaxCachedDescriptorCount = 0;
    }
    else
    {
        // Clear DescriptorMap which is only used for unlimited MaxCount
        UE_CLOG(!DescriptorMap.IsEmpty(), LogIris, Warning, TEXT("Clearing DescriptorMap from FIstancedStructDescriptorCache %s"), ToCStr(DebugName));
        DescriptorMap.Empty();
        DescriptorLruCache.Empty(MaxCount);
        MaxCachedDescriptorCount = MaxCount;
    }
}

auto
    FInstancedStructDescriptorCache::
    AddSupportedTypes(
        const TConstArrayView<TSoftObjectPtr<UScriptStruct>>& InSupportedTypes)
    -> void
{
    for (const TSoftObjectPtr<UScriptStruct>& Type : InSupportedTypes)
    {
        SupportedTypes.Add(Type);
    }
}


auto
    FInstancedStructDescriptorCache::
    IsSupportedType(
        const UScriptStruct* Struct) const
    -> bool
{
    if (ensure(Struct != nullptr))
    {
        if (SupportedTypes.IsEmpty())
        {
            return true;
        }

        for (const TSoftObjectPtr<UScriptStruct>& SupportedType : SupportedTypes)
        {
            if (const UScriptStruct* SupportedStruct = SupportedType.Get())
            {
                if (Struct->IsChildOf(SupportedStruct))
                {
                    return true;
                }
            }
        }
    }

    return false;
}

auto
    FInstancedStructDescriptorCache::
    FindDescriptor(
        FName StructPath)
    -> TRefCountPtr<const FReplicationStateDescriptor>
{
    FScopeLock Lock(&Mutex);
    if (MaxCachedDescriptorCount > 0)
    {
        return DescriptorLruCache.FindAndTouchRef(StructPath);
    }
    else
    {
        return DescriptorMap.FindRef(StructPath);
    }
}

auto
    FInstancedStructDescriptorCache::
    FindOrAddDescriptor(
        FName StructPath)
    -> TRefCountPtr<const FReplicationStateDescriptor>
{
    auto Descriptor = TRefCountPtr<const FReplicationStateDescriptor>{};

    Descriptor = FindDescriptor(StructPath);
    if (Descriptor.IsValid())
    {
        return Descriptor;
    }

    const UObject* Object = StaticLoadObject(UScriptStruct::StaticClass(), nullptr, ToCStr(StructPath.ToString()), nullptr, LOAD_None);
    if (const UScriptStruct* Struct = Cast<UScriptStruct>(Object))
    {
        Descriptor = CreateAndCacheDescriptor(Struct, StructPath);
    }
    else
    {
        ensureMsgf(Object == nullptr, TEXT("Unable to cast object %s to UScriptStruct"), ToCStr(Object->GetPathName()));
    }

    return Descriptor;
}

auto
    FInstancedStructDescriptorCache::
    FindOrAddDescriptor(
        const UScriptStruct* Struct)
    -> TRefCountPtr<const FReplicationStateDescriptor>
{
    auto Descriptor = TRefCountPtr<const FReplicationStateDescriptor>{};
    if (ensure(Struct != nullptr))
    {
        const FString& PathNameString = Struct->GetPathName();
        const FName PathName(PathNameString);

        Descriptor = FindDescriptor(PathName);
        if (Descriptor.IsValid())
        {
            return Descriptor;
        }

        if (!IsSupportedType(Struct))
        {
            return Descriptor;
        }

        Descriptor = CreateAndCacheDescriptor(Struct, PathName);
    }

    return Descriptor;
}

auto
    FInstancedStructDescriptorCache::
    CreateAndCacheDescriptor(
        const UScriptStruct* Struct,
        FName StructPath)
    -> TRefCountPtr<const FReplicationStateDescriptor>
{
    auto Descriptor = TRefCountPtr<const FReplicationStateDescriptor>{};

    const auto Params = FReplicationStateDescriptorBuilder::FParameters{};
    Descriptor = FReplicationStateDescriptorBuilder::CreateDescriptorForStruct(Struct, Params);

    {
        FScopeLock Lock(&Mutex);
        if (MaxCachedDescriptorCount > 0)
        {
            DescriptorLruCache.Add(StructPath, Descriptor);
        }
        else
        {
            DescriptorMap.Add(StructPath, Descriptor);
        }
    }

    return Descriptor;
}

}

#endif

