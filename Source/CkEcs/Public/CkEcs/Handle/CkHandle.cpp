#include "CkHandle.h"

#include "CkEcs/CkEcsLog.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Utils.h"
#include "CkEcs/Fragments/ReplicatedObjects/CkReplicatedObjects_Utils.h"
#include "CkEcs/Handle/CkHandle_Debugging.h"
#include "CkEcs/Handle/CkHandle_Debugging_Data.h"
#include "CkEcs/Handle/CkHandle_Subsystem.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

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
    FCk_Handle::operator->() const -> TOptional<FCk_Registry>
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
    NetSerialize(
        FArchive& Ar,
        UPackageMap* Map,
        bool& bOutSuccess)
    -> bool
{
    if (Ar.IsSaving())
    {
        if (ck::IsValid(*this) && ck::Is_NOT_Valid(_ReplicationDriver))
        {
            if (Has<TWeakObjectPtr<class UCk_Ecs_ReplicatedObject_UE>>())
            {
                _ReplicationDriver = Get<TWeakObjectPtr<class UCk_Ecs_ReplicatedObject_UE>>();
            }
        }

        Ar << _ReplicationDriver;
    }

    if (Ar.IsLoading())
    {
        Ar << _ReplicationDriver;
        if (ck::IsValid(_ReplicationDriver))
        {
            *this = _ReplicationDriver->Get_AssociatedEntity();
        }
        else
        {
            *this = {};
        }
    }

    bOutSuccess = true;
    return true;
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

// --------------------------------------------------------------------------------------------------------------------

auto
    GetTypeHash(
        const FCk_Handle& InHandle) -> uint32
{
    if (ck::Is_NOT_Valid(InHandle))
    { return GetTypeHash(InHandle.Get_Entity()); }

    const auto Hash = GetTypeHash(InHandle.Get_Entity()) + GetTypeHash(InHandle.Get_Registry());
    ck::ecs::Error(TEXT("Handle [{}] Hash is [{}]"), InHandle, Hash);
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
            FCk_Entity InEntity,
            FCk_Handle InValidHandle)
        -> bool
    {
        return InValidHandle->IsValid(InEntity);
    }

    auto
        IsValid(
            FCk_Handle InEntity,
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
            FCk_Handle InEntity)
        -> FCk_Entity
    {
        return InEntity.Get_Entity();
    }
}

// --------------------------------------------------------------------------------------------------------------------
