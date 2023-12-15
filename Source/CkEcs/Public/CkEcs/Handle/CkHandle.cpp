#include "CkHandle.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkEcs/EntityLifetime/CkEntityLifetime_Fragment_Utils.h"
#include "CkEcs/Handle/CkHandle_Debugging.h"
#include "CkEcs/Handle/CkHandle_Debugging_Data.h"
#include "CkEcs/Handle/CkHandle_Subsystem.h"
#include "CkEcs/Settings/CkEcs_Settings.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle::
    FCk_Handle(
        EntityType InEntity,
        const RegistryType& InRegistry)
    : _Entity(InEntity)
    , _Registry(InRegistry)
#if WITH_EDITORONLY_DATA
    , _EntityID(static_cast<int32>(_Entity.Get_ID()))
    , _EntityNumber(_Entity.Get_EntityNumber())
    , _EntityVersion(_Entity.Get_VersionNumber())
#endif
{
    DoUpdate_MapperAndFragments();
}

FCk_Handle::
    FCk_Handle(
        ThisType&& InOther) noexcept
    : _Entity(std::move(InOther._Entity))
    , _Registry(std::move(InOther._Registry))
    , _Mapper(std::move(InOther._Mapper))
#if WITH_EDITORONLY_DATA
    , _EntityID(std::move(InOther._EntityID))
    , _EntityNumber(std::move(InOther._EntityNumber))
    , _EntityVersion(std::move(InOther._EntityVersion))
    , _Fragments(std::move(InOther._Fragments))
#endif
{
    if (IsValid(ck::IsValid_Policy_Default{}) && UCk_Utils_Ecs_ProjectSettings_UE::Get_HandleDebuggerBehavior() == ECk_Ecs_HandleDebuggerBehavior::Enable)
    { std::ignore = UCk_Utils_HandleDebugger_Subsystem_UE::Get_Subsystem()->GetOrAdd_FragmentsDebug(*this); }
}

FCk_Handle::
    FCk_Handle(
        const ThisType& InOther)
    : _Entity(InOther._Entity)
    , _Registry(InOther._Registry)
    , _Mapper(InOther._Mapper)
#if WITH_EDITORONLY_DATA
    , _EntityID(InOther._EntityID)
    , _EntityNumber(InOther._EntityNumber)
    , _EntityVersion(InOther._EntityVersion)
    , _Fragments(InOther._Fragments)
#endif
{
    if (IsValid(ck::IsValid_Policy_Default{}) && UCk_Utils_Ecs_ProjectSettings_UE::Get_HandleDebuggerBehavior() == ECk_Ecs_HandleDebuggerBehavior::Enable)
    { std::ignore = UCk_Utils_HandleDebugger_Subsystem_UE::Get_Subsystem()->GetOrAdd_FragmentsDebug(*this); }
}

auto
    FCk_Handle::
    operator=(
        ThisType InOther) -> ThisType&
{
    Swap(InOther);
    DoUpdate_MapperAndFragments();
    return *this;
}

FCk_Handle::
    ~FCk_Handle()
{
#if WITH_EDITORONLY_DATA
    if (NOT IsValid(ck::IsValid_Policy_IncludePendingKill{}))
    { return; }

    if (IsValid(ck::IsValid_Policy_Default{}) && UCk_Utils_Ecs_ProjectSettings_UE::Get_HandleDebuggerBehavior() == ECk_Ecs_HandleDebuggerBehavior::Enable &&
        ck::IsValid(GEngine))
    { UCk_Utils_HandleDebugger_Subsystem_UE::Get_Subsystem()->Remove_FragmentsDebug(*this); }
#endif
}

auto
    FCk_Handle::Swap(
        ThisType& InOther) -> void
{
    ::Swap(_Entity, InOther._Entity);
    ::Swap(_Registry, InOther._Registry);
    ::Swap(_Mapper, InOther._Mapper);
#if WITH_EDITORONLY_DATA
    ::Swap(_EntityID, InOther._EntityID);
    ::Swap(_EntityNumber, InOther._EntityNumber);
    ::Swap(_EntityVersion, InOther._EntityVersion);
    ::Swap(_Fragments, InOther._Fragments);
#endif
}

auto
    FCk_Handle::
    operator==(
        const ThisType& InOther) const
    -> bool
{
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
    DoUpdate_MapperAndFragments()
    -> void
{
    if (UCk_Utils_Ecs_ProjectSettings_UE::Get_HandleDebuggerBehavior() == ECk_Ecs_HandleDebuggerBehavior::Disable)
    { return; }

    if (NOT IsValid(ck::IsValid_Policy_IncludePendingKill{}))
    { return; }

    if (Has<FEntity_FragmentMapper>())
    {
        _Mapper = &Get<FEntity_FragmentMapper>();

        [[maybe_unused]]
        const auto& Names = _Mapper->ProcessAll(*this);

#if WITH_EDITORONLY_DATA
        if (ck::Is_NOT_Valid(_Fragments))
        { _Fragments = UCk_Utils_HandleDebugger_Subsystem_UE::Get_Subsystem()->GetOrAdd_FragmentsDebug(*this); }

        _Fragments->_Names = Names;
#endif
    }
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
            FCk_Handle InValidHandle)
        -> FCk_Handle
    {
        CK_ENSURE_IF_NOT(ck::IsValid(InValidHandle),
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
        CK_ENSURE_IF_NOT(ck::IsValid(InValidHandle),
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
