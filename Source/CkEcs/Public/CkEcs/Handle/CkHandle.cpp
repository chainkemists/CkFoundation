#include "CkHandle.h"

#include "CkCore/Object/CkObject_Utils.h"

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

//auto
//    FCk_Handle::
//    NetSerialize(
//        FArchive& Ar,
//        UPackageMap* Map,
//        bool& bOutSuccess)
//    -> bool
//{
//    if (Ar.IsSaving())
//    {
//        if (ck::IsValid(*this) && ck::Is_NOT_Valid(_ReplicationDriver))
//        {
//            if (Has<TWeakObjectPtr<class UCk_Ecs_ReplicatedObject_UE>>())
//            {
//                _ReplicationDriver = Get<TWeakObjectPtr<class UCk_Ecs_ReplicatedObject_UE>>();
//            }
//        }
//
//        Ar << _ReplicationDriver;
//    }
//
//    if (Ar.IsLoading())
//    {
//        Ar << _ReplicationDriver;
//        if (ck::IsValid(_ReplicationDriver))
//        {
//            *this = _ReplicationDriver->Get_AssociatedEntity();
//        }
//        else
//        {
//            *this = {};
//        }
//    }
//
//    bOutSuccess = true;
//    return true;
//}

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
    if (IsValid(ck::IsValid_Policy_IncludePendingKill{}) && Has<DEBUG_NAME>())
    { return Get<DEBUG_NAME>().Get_Name(); }
    else
    { return TEXT("no-debug-name"); }
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

namespace UE::Net
{

struct FCk_HandleNetSerializer
{
    static const uint32 Version = 0;

    typedef FCk_Handle SourceType;
    typedef FNetObjectReference QuantizedType;

    typedef FCk_HandleSerializerConfig ConfigType;

    static constexpr bool bHasSerialize = true;
    static constexpr bool bHasDeserialize = true;
    static constexpr bool bHasQuantize = true;
    static constexpr bool bHasDequantize = true;
    static constexpr bool bIsForwardingSerializer = true;
	static constexpr bool bHasCustomNetReference = true;

    static const ConfigType DefaultConfig;

    static void Serialize(FNetSerializationContext&, const FNetSerializeArgs&);
    static void Deserialize(FNetSerializationContext&, const FNetDeserializeArgs&);

    static void SerializeDelta(FNetSerializationContext&, const FNetSerializeDeltaArgs&);
    static void DeserializeDelta(FNetSerializationContext&, const FNetDeserializeDeltaArgs&);

	static void CloneDynamicState(FNetSerializationContext&, const FNetCloneDynamicStateArgs&);
	static void FreeDynamicState(FNetSerializationContext&, const FNetFreeDynamicStateArgs&);

    static void Quantize(FNetSerializationContext&, const FNetQuantizeArgs& Args);
    static void Dequantize(FNetSerializationContext&, const FNetDequantizeArgs& Args);

	static void CollectNetReferences(FNetSerializationContext&, const FNetCollectReferencesArgs&);

    static bool IsEqual(FNetSerializationContext&, const FNetIsEqualArgs& Args);
	static bool Validate(FNetSerializationContext&, const FNetValidateArgs& Args);

    inline static const FNetSerializer* _ObjectNetSerializer = &UE_NET_GET_SERIALIZER(FObjectNetSerializer);
    inline static const FNetSerializerConfig* _ObjectNetSerializerConfig = UE_NET_GET_SERIALIZER_DEFAULT_CONFIG(FObjectNetSerializer);

    class FNetSerializerRegistryDelegates final : private UE::Net::FNetSerializerRegistryDelegates
    {
    public:
        ~FNetSerializerRegistryDelegates() override;
    private:
        void OnPreFreezeNetSerializerRegistry() override;
    };

    static FNetSerializerRegistryDelegates _NetSerializerRegistryDelegates;
};

static const FName PropertyNetSerializerRegistry_NAME_Handle("Ck_Handle");
UE_NET_IMPLEMENT_NAMED_STRUCT_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_NAME_Handle, FCk_HandleNetSerializer);

FCk_HandleNetSerializer::FNetSerializerRegistryDelegates FCk_HandleNetSerializer::_NetSerializerRegistryDelegates;

FCk_HandleNetSerializer::FNetSerializerRegistryDelegates::~FNetSerializerRegistryDelegates()
{
    UE_NET_UNREGISTER_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_NAME_Handle);
}

void
    FCk_HandleNetSerializer::FNetSerializerRegistryDelegates::OnPreFreezeNetSerializerRegistry()
{
    UE_NET_REGISTER_NETSERIALIZER_INFO(PropertyNetSerializerRegistry_NAME_Handle);
}

UE_NET_IMPLEMENT_SERIALIZER(FCk_HandleNetSerializer);

const FCk_HandleNetSerializer::ConfigType FCk_HandleNetSerializer::DefaultConfig;

void FCk_HandleNetSerializer::Serialize(FNetSerializationContext& Context, const FNetSerializeArgs& Args)
{
    _ObjectNetSerializer->Serialize(Context, Args);
}

void FCk_HandleNetSerializer::Deserialize(FNetSerializationContext& Context, const FNetDeserializeArgs& Args)
{
    _ObjectNetSerializer->Deserialize(Context, Args);
}

void
    FCk_HandleNetSerializer::CloneDynamicState(
        FNetSerializationContext& Context,
        const FNetCloneDynamicStateArgs& Args)
{
	const QuantizedType& Source = *reinterpret_cast<const QuantizedType*>(Args.Source);
	const QuantizedType& Target = *reinterpret_cast<const QuantizedType*>(Args.Target);
}

void
    FCk_HandleNetSerializer::FreeDynamicState(
        FNetSerializationContext& Context,
        const FNetFreeDynamicStateArgs& Args)
{
}

auto
    FCk_HandleNetSerializer::
    SerializeDelta(
        FNetSerializationContext& Context,
        const FNetSerializeDeltaArgs& Args)
    -> void
{
	const QuantizedType& Value = *reinterpret_cast<const QuantizedType*>(Args.Source);
	const QuantizedType& PrevValue = *reinterpret_cast<const QuantizedType*>(Args.Prev);

    auto& ValuePtr = *reinterpret_cast<FCk_Handle*>(Args.Source);
    auto& PrevValuePtr = *reinterpret_cast<FCk_Handle*>(Args.Prev);

    auto& ObjectPtr = *reinterpret_cast<UObject*>(Args.Source);
    auto& PrevObjectPtr = *reinterpret_cast<UObject*>(Args.Prev);

    if (Value != PrevValue)
    {
        //FNetSerializeDeltaArgs NewArgs{};
        //NewArgs.Source = Args.Source;
        //NewArgs. = Args.Source;
        //NewArgs.NetSerializerConfig = Args.NetSerializerConfig;
        //_ObjectNetSerializer->DeserializeDelta(Context, NewArgs);
    }
}

auto
    FCk_HandleNetSerializer::
    DeserializeDelta(
        FNetSerializationContext& Context,
        const FNetDeserializeDeltaArgs& Args)
    -> void
{
	const QuantizedType& Value = *reinterpret_cast<const QuantizedType*>(Args.Target);
	const QuantizedType& PrevValue = *reinterpret_cast<const QuantizedType*>(Args.Prev);

    auto& ValuePtr = *reinterpret_cast<FCk_Handle*>(Args.Target);
    auto& PrevValuePtr = *reinterpret_cast<FCk_Handle*>(Args.Prev);

    auto& ObjectPtr = *reinterpret_cast<UObject*>(Args.Target);
    auto& PrevObjectPtr = *reinterpret_cast<UObject*>(Args.Prev);

    if (Value != PrevValue)
    {
        //FNetDeserializeArgs NewArgs{};
        //NewArgs.Target = Args.Target;
        //NewArgs.NetSerializerConfig = Args.NetSerializerConfig;
        //_ObjectNetSerializer->Deserialize(Context, NewArgs);
    }
}

auto
    FCk_HandleNetSerializer::
    Quantize(
        FNetSerializationContext& Context,
        const FNetQuantizeArgs& Args)
    -> void
{
    //auto& Source = *reinterpret_cast<QuantizedType*>(Args.Source);
    //auto& Target = *reinterpret_cast<QuantizedType*>(Args.Target);

    //if (Source.IsValid())
    //{
        auto& Source = *reinterpret_cast<FCk_Handle*>(Args.Source);
        //if (Source._ReplicationDriver.IsValid())
        //{
            auto NewArgs = FNetQuantizeArgs{};
            NewArgs.NetSerializerConfig = Args.NetSerializerConfig;
            auto Object = Source._ReplicationDriver.Get();
            NewArgs.Source = reinterpret_cast<NetSerializerValuePointer>(&Object);
            NewArgs.Target = Args.Target;
            _ObjectNetSerializer->Quantize(Context, NewArgs);
        //}
    //}
    //else if (Target.IsValid())
    //{
    //    int i = 0;
    //}

    //_ObjectNetSerializer->Quantize(Context, Args);
}

auto
    FCk_HandleNetSerializer::
    Dequantize(
        FNetSerializationContext& Context,
        const FNetDequantizeArgs& Args)
    -> void
{
    //auto& Source = *reinterpret_cast<QuantizedType*>(Args.Source);
    //auto& Target = *reinterpret_cast<QuantizedType*>(Args.Target);

    //if (Target.IsValid())
    //{
        auto NewArgs = FNetDequantizeArgs{};
        NewArgs.NetSerializerConfig = Args.NetSerializerConfig;
        NewArgs.Source = Args.Source;
        UObject* Target = nullptr;
        NewArgs.Target = reinterpret_cast<NetSerializerValuePointer>(&Target);
        _ObjectNetSerializer->Dequantize(Context, NewArgs);

        auto RepObj = Cast<UCk_Ecs_ReplicatedObject_UE>(Target);
        //if (ck::IsValid(RepObj))
        //{
            auto Handle = reinterpret_cast<FCk_Handle*>(Args.Target);
            *Handle = ck::IsValid(RepObj) ? RepObj->Get_AssociatedEntity() : FCk_Handle{};
            Handle->_ReplicationDriver = RepObj;
        //}
    //}
    //else if (Target.IsValid())
    //{
    //}

    //auto NewArgs = FNetDequantizeArgs{};
    //NewArgs.Source = Args.Source;
    //NewArgs.Target = ;

    //_ObjectNetSerializer->Dequantize(Context, Args);
    //UObject* RepObj2 = reinterpret_cast<UObject*>(Args.Source);
    //UObject* RepObj = reinterpret_cast<UObject*>(Args.Target);

    //if (ck::Is_NOT_Valid(RepObj))
    //{ return; }

    //auto& Target = *reinterpret_cast<FCk_Handle*>(ExistingTarget);
    //Target._ReplicationDriver = Cast<UCk_Ecs_ReplicatedObject_UE>(RepObj);
    //Target = Target._ReplicationDriver->Get_AssociatedEntity();
}

void
    FCk_HandleNetSerializer::CollectNetReferences(
        FNetSerializationContext& Context,
        const FNetCollectReferencesArgs& Args)
{
	const QuantizedType& Source = *reinterpret_cast<const QuantizedType*>(Args.Source);
	const UObject& Object = *reinterpret_cast<const UObject*>(Args.Source);
	const FCk_Handle& Handle = *reinterpret_cast<const FCk_Handle*>(Args.Source);

    // Check if the WeakObjectPtr is valid
	if (Source.IsValid())
	{
		const QuantizedType& Value = *reinterpret_cast<const QuantizedType*>(Args.Source);
		FNetReferenceCollector& Collector = *reinterpret_cast<FNetReferenceCollector*>(Args.Collector);

		const FNetReferenceInfo ReferenceInfo(FNetReferenceInfo::EResolveType::ResolveOnClient);
		Collector.Add(ReferenceInfo, Value, Args.ChangeMaskInfo);
	}
}

auto
    UE::Net::FCk_HandleNetSerializer::
    IsEqual(
        FNetSerializationContext&,
        const FNetIsEqualArgs& Args)
    -> bool
{
    return false;
    //if (Args.bStateIsQuantized)
    //{
    //    const QuantizedType& Value0 = *reinterpret_cast<const QuantizedType*>(Args.Source0);
    //    const QuantizedType& Value1 = *reinterpret_cast<const QuantizedType*>(Args.Source1);

    //    return Value0 == Value1;
    //}
    //else
    //{
    //    const SourceType Value0 = *reinterpret_cast<const SourceType*>(Args.Source0);
    //    const SourceType Value1 = *reinterpret_cast<const SourceType*>(Args.Source1);

    //    return Value0 == Value1;
    //}
}

bool
    FCk_HandleNetSerializer::Validate(
        FNetSerializationContext&,
        const FNetValidateArgs& Args)
{
	const SourceType& Source = *reinterpret_cast<const SourceType*>(Args.Source);
    return Source.IsValid(ck::IsValid_Policy_IncludePendingKill{});
}
}
