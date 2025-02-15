#include "CkHandle.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/CkEcsLog.h"
#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/Handle/CkHandle_Debugging.h"
#include "CkEcs/Handle/CkHandle_Debugging_Data.h"
#include "CkEcs/Handle/CkHandle_Subsystem.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

#include <Iris/Serialization/ObjectNetSerializer.h>
#include <Iris/Core/NetObjectReference.h>
#include <Iris/ReplicationState/PropertyNetSerializerInfoRegistry.h>
#include <Iris/Serialization/NetReferenceCollector.h>


// --------------------------------------------------------------------------------------------------------------------

#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING

auto
    DEBUG_NAME::DoSet_DebugName(
        FName InDebugName,
        ECk_Override InOverride)
    -> void
{
    if (InOverride == ECk_Override::DoNotOverride)
    {
        if (NOT _Name.IsNone())
        { _PreviousNames.Emplace(InDebugName); }
        else
        { _Name = InDebugName; }
    }
    else
    {
        if (NOT _Name.IsNone())
        { _PreviousNames.Emplace(_Name); }

        _Name = InDebugName;
    }
}

TCk_DebugWrapper<DEBUG_NAME>::
    TCk_DebugWrapper(
        const DEBUG_NAME* InPtr)
    : _Fragment(InPtr)
{
}

auto
    TCk_DebugWrapper<DEBUG_NAME>::
    GetHash() const
    -> IdType
{
    return entt::type_id<DEBUG_NAME>().hash();
}

auto
    TCk_DebugWrapper<DEBUG_NAME>::
    SetFragmentPointer(
        const void* InFragmentPtr)
    -> void
{
    _Fragment = static_cast<const DEBUG_NAME*>(InFragmentPtr);
}

auto
    TCk_DebugWrapper<DEBUG_NAME>::
    Get_FragmentName(
        const FCk_Handle& InHandle) const
    -> FName
{
    return InHandle.Get<DEBUG_NAME>().Get_Name();
}

auto
    TCk_DebugWrapper<DEBUG_NAME>::
    operator==(
        const TCk_DebugWrapper<DEBUG_NAME>& InOther) const
    -> bool
{
    return GetHash() == InOther.GetHash();
}

auto
    TCk_DebugWrapper<DEBUG_NAME>::
    operator!=(
        const TCk_DebugWrapper<DEBUG_NAME>& InOther) const
    -> bool
{
    return GetHash() != InOther.GetHash();
}

#endif

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle::
    FCk_Handle(
        EntityType InEntity,
        const RegistryType& InRegistry)
    : _Entity(InEntity)
    , _Registry(InRegistry)
{
    DoUpdate_FragmentDebugInfo_Blueprints();
}

FCk_Handle::
    FCk_Handle(
        ThisType&& InOther) noexcept
    : _Entity(std::move(InOther._Entity))
    , _Registry(std::move(InOther._Registry))
    , _ReplicationDriver(std::move(InOther._ReplicationDriver))
#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING
    , _Mapper(std::move(InOther._Mapper))
#endif
#if WITH_EDITORONLY_DATA
    , _Fragments(std::move(InOther._Fragments))
#endif
{
    DoUpdate_FragmentDebugInfo_Blueprints();
}

FCk_Handle::
    FCk_Handle(
        const ThisType& InOther)
    : _Entity(InOther._Entity)
    , _Registry(InOther._Registry)
    , _ReplicationDriver(InOther._ReplicationDriver)
#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING
    , _Mapper(InOther._Mapper)
#endif
#if WITH_EDITORONLY_DATA
    , _Fragments(InOther._Fragments)
#endif
{
    DoUpdate_FragmentDebugInfo_Blueprints();
}

auto
    FCk_Handle::
    operator=(
        ThisType InOther) -> ThisType&
{
    Swap(InOther);
    return *this;
}

FCk_Handle::
    ~FCk_Handle()
{
    if (NOT IsValid(ck::IsValid_Policy_IncludePendingKill{}))
    { return; }
}

auto
    FCk_Handle::Swap(
        ThisType& InOther) -> void
{
    ::Swap(_Entity, InOther._Entity);
    ::Swap(_Registry, InOther._Registry);
    ::Swap(_ReplicationDriver, InOther._ReplicationDriver);
#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING
    ::Swap(_Mapper, InOther._Mapper);
#endif
#if WITH_EDITORONLY_DATA
    ::Swap(_Fragments, InOther._Fragments);
#endif
}

auto
    FCk_Handle::
    operator<(
        const ThisType& InOther) const
    -> bool
{
    return _Entity.operator<(InOther.Get_Entity());
}

auto
    FCk_Handle::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    if (Get_Entity() == InOther.Get_Entity() && Get_Entity().Get_IsTombstone())
    { return true; }

    return Get_Entity() == InOther.Get_Entity() && GetTypeHash(*_Registry) == GetTypeHash(*InOther._Registry);
}

auto
    FCk_Handle::
    operator*()
    -> TOptional<FCk_Registry>
{
    return _Registry;
}

auto
    FCk_Handle::
    operator*() const
    -> TOptional<FCk_Registry>
{
    return _Registry;
}

auto
    FCk_Handle::
    operator
    ->() -> TOptional<FCk_Registry>
{
    return _Registry;
}

auto
    FCk_Handle::
    operator->() const
    -> TOptional<FCk_Registry>
{
    return _Registry;
}

auto
    FCk_Handle::
    IsValid(
        ck::IsValid_Policy_Default) const
    -> bool
{
    return IsValid(ck::IsValid_Policy_IncludePendingKill{}) && NOT UCk_Utils_EntityLifetime_UE::Get_IsPendingDestroy(*this);
}

auto
    FCk_Handle::
    IsValid(
        ck::IsValid_Policy_IncludePendingKill) const
    -> bool
{
    return ck::IsValid(_Registry) && ck::IsValid(*_Registry) && _Registry->IsValid(_Entity);
}

auto
    FCk_Handle::
    Orphan() const
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(_Registry),
        TEXT("Unable to perform Orphan query. Handle [{}] does NOT have a valid Registry."), *this)
    { return true; }

    return _Registry->Orphan(_Entity);
}

auto
    FCk_Handle::
    Get_ValidHandle(EntityType::IdType InEntity) const
    -> ThisType
{
    CK_ENSURE_IF_NOT(IsValid(ck::IsValid_Policy_IncludePendingKill{}),
        TEXT("Unable to Validate Entity [{}]. Handle [{}] does NOT have a valid Registry."), FCk_Entity{InEntity}, *this)
    { return {}; }

    return ThisType{_Registry->Get_ValidEntity(InEntity), *_Registry};
}

auto
    FCk_Handle::
    Get_Registry()
    -> FCk_Registry&
{
    return *_Registry;
}

auto
    FCk_Handle::
    Get_Registry() const
    -> const FCk_Registry&
{
    return *_Registry;
}

auto
    FCk_Handle::
    DoUpdate_FragmentDebugInfo_Blueprints()
    -> void
{
    if (UCk_Utils_Ecs_Settings_UE::Get_HandleDebuggerBehavior() == ECk_Ecs_HandleDebuggerBehavior::Disable)
    { return; }

    if (NOT IsValid(ck::IsValid_Policy_IncludePendingKill{}))
    { return; }

#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING
    if (ck::Is_NOT_Valid(_Mapper, ck::IsValid_Policy_NullptrOnly{}))
    { _Mapper = &_Registry->AddOrGet<FEntity_FragmentMapper>(_Entity); }

    if (UCk_Utils_Ecs_Settings_UE::Get_HandleDebuggerBehavior() != ECk_Ecs_HandleDebuggerBehavior::EnableWithBlueprintDebugging)
    { return; }
#endif

#if WITH_EDITORONLY_DATA
    if (ck::Is_NOT_Valid(_Fragments))
    { _Fragments = UCk_Utils_HandleDebugger_Subsystem_UE::GetOrAdd_FragmentsDebug(*this); }

    if (ck::IsValid(_Fragments))
    { _Fragments->_Names = _Mapper->Get_FragmentNames(); }
#endif
}

auto
    FCk_Handle::
    Get_DebugName() const
    -> FName
{
#if NOT CK_ECS_DISABLE_HANDLE_DEBUGGING
    if (NOT IsValid(ck::IsValid_Policy_IncludePendingKill{}))
    {
        if (NOT ck::IsValid(_Registry))
        { return TEXT("no-debug-name-invalid-registry"); }

        return TEXT("no-debug-name-invalid-handle");
    }

    if (Has<DEBUG_NAME>())
    { return Get<DEBUG_NAME>().Get_Name(); }

    return TEXT("no-debug-name");
#else
    return TEXT("no-handle-debug");
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    GetTypeHash(
        const FCk_Handle& InHandle) -> uint32
{
    if (ck::Is_NOT_Valid(InHandle))
    { return GetTypeHash(InHandle.Get_Entity()); }

    return GetTypeHash(InHandle.Get_Entity()) + GetTypeHash(InHandle.Get_Registry());
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    auto
        MatchesEntityHandle::
        operator()(
            const FCk_Handle& InHandle) const
        -> bool
    {
        return InHandle == _EntityHandle;
    }

    auto
        IsValidEntityHandle::
        operator()(
            const FCk_Handle& InHandle) const
        -> bool
    {
        return ck::IsValid(InHandle);
    }

    auto
        IsValidEntityHandle_IncludePendingKill::
        operator()(
            const FCk_Handle& InHandle) const
        -> bool
    {
        return ck::IsValid(InHandle, ck::IsValid_Policy_IncludePendingKill{});
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{

    auto
        MakeHandle(
            FCk_Entity InEntity,
            const FCk_Registry& InValidHandle)
        -> FCk_Handle
    {
        return FCk_Handle{InEntity, InValidHandle};
    }

    auto
        MakeHandle(
            FCk_Entity InEntity,
            FCk_Handle InValidHandle)
        -> FCk_Handle
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InValidHandle, ck::IsValid_Policy_IncludePendingKill{}),
            TEXT("Unable to create handle for Entity [{}] because Handle [{}] is INVALID."), InEntity, InValidHandle)
        { return {}; }

        return FCk_Handle{InEntity, **InValidHandle};
    }

    auto
        MakeHandle(
            FCk_Handle InEntity,
            FCk_Handle InValidHandle)
        -> FCk_Handle
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InValidHandle, ck::IsValid_Policy_IncludePendingKill{}),
            TEXT("Unable to create handle for Entity [{}] because Handle [{}] is INVALID."), InEntity, InValidHandle)
        { return {}; }

        return FCk_Handle{InEntity.Get_Entity(), **InValidHandle};
    }

    auto
        IsValid(
            const FCk_Entity InEntity,
            FCk_Handle InValidHandle)
        -> bool
    {
        return InValidHandle->IsValid(InEntity);
    }

    auto
        IsValid(
            const FCk_Handle& InEntity,
            FCk_Handle InValidHandle)
        -> bool
    {
        return InValidHandle->IsValid(InEntity.Get_Entity());
    }

    auto
        GetEntity(
            const FCk_Entity InEntity)
        -> FCk_Entity
    {
        return InEntity;
    }

    auto
        GetEntity(
            const FCk_Handle& InEntity)
        -> FCk_Entity
    {
        return InEntity.Get_Entity();
    }
}

// --------------------------------------------------------------------------------------------------------------------

// ReSharper disable once CppInconsistentNaming
namespace UE::Net
{
    struct FCk_HandleNetSerializer
    {
        // ReSharper disable once CppVariableCanBeMadeConstexpr
        static const uint32 Version = 0;

        typedef FCk_Handle SourceType;
        typedef FNetObjectReference QuantizedType;

        typedef FCk_HandleSerializerConfig ConfigType;

        // ReSharper disable once CppInconsistentNaming
        static constexpr bool bHasSerialize = true;
        // ReSharper disable once CppInconsistentNaming
        static constexpr bool bHasDeserialize = true;
        // ReSharper disable once CppInconsistentNaming
        static constexpr bool bHasQuantize = true;
        // ReSharper disable once CppInconsistentNaming
        static constexpr bool bHasDequantize = true;
        // ReSharper disable once CppInconsistentNaming
        static constexpr bool bIsForwardingSerializer = true;
        // ReSharper disable once CppInconsistentNaming
        static constexpr bool bHasCustomNetReference = true;

        static const ConfigType DefaultConfig;

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
            const FNetSerializeDeltaArgs&) -> void;

        static auto
        DeserializeDelta(
            FNetSerializationContext&,
            const FNetDeserializeDeltaArgs&) -> void;

        static auto
        CloneDynamicState(
            FNetSerializationContext&,
            const FNetCloneDynamicStateArgs&) -> void;

        static auto
        FreeDynamicState(
            FNetSerializationContext&,
            const FNetFreeDynamicStateArgs&) -> void;

        static auto
        Quantize(
            FNetSerializationContext&,
           const FNetQuantizeArgs& Args) -> void;

        static auto
        Dequantize(
            FNetSerializationContext&,
            const FNetDequantizeArgs& Args) -> void;

        static auto
        CollectNetReferences(
            FNetSerializationContext&,
            const FNetCollectReferencesArgs&) -> void;

        static auto
        IsEqual(
            FNetSerializationContext&,
            const FNetIsEqualArgs& Args) -> bool;

        static auto
        Validate(
            FNetSerializationContext&,
            const FNetValidateArgs& Args) -> bool;

        inline static auto _WeakObjectNetSerializer = &UE_NET_GET_SERIALIZER(FWeakObjectNetSerializer);

        // --------------------------------------------------------------------------------------------------------------------

        class FNetSerializerRegistryDelegates final : private UE::Net::FNetSerializerRegistryDelegates
        {
        public:
            // ReSharper disable once CppEnforceOverridingDestructorStyle
            ~FNetSerializerRegistryDelegates() override;
        private:
            auto
            OnPreFreezeNetSerializerRegistry() -> void override;
        };

        static FNetSerializerRegistryDelegates _NetSerializerRegistryDelegates;
    };

    // --------------------------------------------------------------------------------------------------------------------

    static const FName PropertyNetSerializerRegistry_Name_Handle("Ck_Handle");
    FCk_HandleNetSerializer::FNetSerializerRegistryDelegates FCk_HandleNetSerializer::_NetSerializerRegistryDelegates;

    UE_NET_IMPLEMENT_NAMED_STRUCT_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_Name_Handle, FCk_HandleNetSerializer);
    UE_NET_IMPLEMENT_SERIALIZER(FCk_HandleNetSerializer);

    // --------------------------------------------------------------------------------------------------------------------

    const FCk_HandleNetSerializer::ConfigType FCk_HandleNetSerializer::DefaultConfig;

    FCk_HandleNetSerializer::FNetSerializerRegistryDelegates::
        ~FNetSerializerRegistryDelegates()
    {
        UE_NET_UNREGISTER_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_Name_Handle);
    }

    auto
        FCk_HandleNetSerializer::FNetSerializerRegistryDelegates::
        OnPreFreezeNetSerializerRegistry()
        -> void
    {
        UE_NET_REGISTER_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_Name_Handle);
    }

    // --------------------------------------------------------------------------------------------------------------------

    auto
        FCk_HandleNetSerializer::
        Serialize(
            FNetSerializationContext& Context,
            const FNetSerializeArgs& Args)
        -> void
    {
        _WeakObjectNetSerializer->Serialize(Context, Args);
    }

    auto
        FCk_HandleNetSerializer::
        Deserialize(
            FNetSerializationContext& Context,
            const FNetDeserializeArgs& Args)
        -> void
    {
        _WeakObjectNetSerializer->Deserialize(Context, Args);
    }

    auto
        FCk_HandleNetSerializer::
        CloneDynamicState(
            FNetSerializationContext& Context,
            const FNetCloneDynamicStateArgs& Args)
        -> void
    {
        _WeakObjectNetSerializer->CloneDynamicState(Context, Args);
    }

    auto
        FCk_HandleNetSerializer::
        FreeDynamicState(
            FNetSerializationContext& Context,
            const FNetFreeDynamicStateArgs& Args)
        -> void
    {
        _WeakObjectNetSerializer->FreeDynamicState(Context, Args);
    }

    auto
        FCk_HandleNetSerializer::
        SerializeDelta(
            FNetSerializationContext& Context,
            const FNetSerializeDeltaArgs& Args)
        -> void
    {
        // _WeakObjectNetSerializer->SerializeDelta(Context, Args);
        FNetIsEqualArgs EqualArgs;
	    EqualArgs.Version = 0;
	    EqualArgs.NetSerializerConfig = Args.NetSerializerConfig;
	    EqualArgs.Source0 = Args.Source;
	    EqualArgs.Source1 = Args.Prev;
	    EqualArgs.bStateIsQuantized = true;

	    if (Context.GetBitStreamWriter()->WriteBool(IsEqual(Context, EqualArgs)))
	    {
		    return;
	    }

	    Serialize(Context, Args);
    }

    auto
        FCk_HandleNetSerializer::
        DeserializeDelta(
            FNetSerializationContext& Context,
            const FNetDeserializeDeltaArgs& Args)
        -> void
    {
        _WeakObjectNetSerializer->DeserializeDelta(Context, Args);
    }

    auto
        FCk_HandleNetSerializer::
        Quantize(
            FNetSerializationContext& Context,
            const FNetQuantizeArgs& Args)
        -> void
    {
        auto& Source = *reinterpret_cast<FCk_Handle*>(Args.Source);
        auto NewArgs = FNetQuantizeArgs{};

        NewArgs.NetSerializerConfig = Args.NetSerializerConfig;
        if (ck::Is_NOT_Valid(Source._ReplicationDriver) && Source.IsValid(ck::IsValid_Policy_IncludePendingKill{}) && UCk_Utils_ReplicatedObjects_UE::Has(Source))
        {
            // TODO: This is a temporary fix. We need to find a better way to handle the fact that sometimes the Handle does NOT have a replicated object
            UCk_Utils_ReplicatedObjects_UE::OnFirstValidReplicatedObject(Source, ECk_PendingKill_Policy::IncludePendingKill,
            [&](const TWeakObjectPtr<UCk_ReplicatedObject_UE>& InRO)
            {
                Source._ReplicationDriver = Cast<UCk_Ecs_ReplicatedObject_UE>(InRO.Get());
            });
        }

        ck::ecs::Log(TEXT("Handle [{}] Quantized"), Source);

        NewArgs.Source = reinterpret_cast<NetSerializerValuePointer>(&Source._ReplicationDriver);
        NewArgs.Target = Args.Target;

        _WeakObjectNetSerializer->Quantize(Context, NewArgs);
    }

    auto
        FCk_HandleNetSerializer::
        Dequantize(
            FNetSerializationContext& Context,
            const FNetDequantizeArgs& Args)
        -> void
    {
        auto NewArgs = FNetDequantizeArgs{};

        NewArgs.NetSerializerConfig = Args.NetSerializerConfig;
        NewArgs.Source = Args.Source;

        auto Target = TWeakObjectPtr<UCk_Ecs_ReplicatedObject_UE>{};

        NewArgs.Target = reinterpret_cast<NetSerializerValuePointer>(&Target);
        _WeakObjectNetSerializer->Dequantize(Context, NewArgs);

        const auto Handle = reinterpret_cast<FCk_Handle*>(Args.Target);
        if (ck::IsValid(Target))
        {
            Handle->_ReplicationDriver = Target;
            const auto& Entity = Target->Get_AssociatedEntity();
            CK_ENSURE(ck::IsValid(Entity),
                TEXT("ReplicationDriver [{}] does NOT have a valid associated entity. It's possible that this is a bug in the ReplicationDriver"),
                Target);

            *Handle = Entity;
        }

        ck::ecs::Log(TEXT("Handle [{}] De-Quantized"), *Handle);
    }

    auto
        FCk_HandleNetSerializer::
        CollectNetReferences(
            FNetSerializationContext& Context,
            const FNetCollectReferencesArgs& Args)
        -> void
    {
        if (const auto& Source = *reinterpret_cast<const QuantizedType*>(Args.Source);
            ck::Is_NOT_Valid(Source))
        { return; }

        const QuantizedType& Value = *reinterpret_cast<const QuantizedType*>(Args.Source);
        FNetReferenceCollector& Collector = *reinterpret_cast<FNetReferenceCollector*>(Args.Collector);

        const FNetReferenceInfo ReferenceInfo(FNetReferenceInfo::EResolveType::ResolveOnClient);
        Collector.Add(ReferenceInfo, Value, Args.ChangeMaskInfo);
    }

    auto
        UE::Net::FCk_HandleNetSerializer::
        IsEqual(
            FNetSerializationContext&,
            const FNetIsEqualArgs& Args)
        -> bool
    {
        if (Args.bStateIsQuantized)
        {
            const QuantizedType& Value0 = *reinterpret_cast<const QuantizedType*>(Args.Source0);
            const QuantizedType& Value1 = *reinterpret_cast<const QuantizedType*>(Args.Source1);

            ck::ecs::Log(TEXT("Handle IsEqual [{}] using QuantizedType"), Value0 == Value1);

            return Value0 == Value1;
        }
        else
        {
            const SourceType Value0 = *reinterpret_cast<const SourceType*>(Args.Source0);
            const SourceType Value1 = *reinterpret_cast<const SourceType*>(Args.Source1);

            ck::ecs::Log(TEXT("Handle IsEqual [{}] using Non-QuantizedType"), Value0 == Value1);

            return Value0 == Value1;
        }
    }

    auto
        FCk_HandleNetSerializer::
        Validate(
            FNetSerializationContext&,
            const FNetValidateArgs& Args)
        -> bool
    {
        const SourceType& Source = *reinterpret_cast<const SourceType*>(Args.Source);

        ck::ecs::Log(TEXT("Handle Validate [{}]"), Source.IsValid(ck::IsValid_Policy_IncludePendingKill{}));
        return Source.IsValid(ck::IsValid_Policy_IncludePendingKill{});
    }
}
