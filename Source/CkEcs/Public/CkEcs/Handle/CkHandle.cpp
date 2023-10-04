#include "CkHandle.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

FCk_Handle::
    FCk_Handle(
        FEntityType InEntity,
        const FRegistryType& InRegistry)
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
    if (ck::Is_NOT_Valid(_Fragments))
    { return; }

    if (NOT _Fragments->IsRooted())
    { return; }

    _Fragments->RemoveFromRoot();
}

//auto
//    FCk_Handle::
//    operator=(
//        ThisType&& InOther)
//    -> ThisType&
//{
//    _Entity = std::move(InOther._Entity);
//    _Registry = std::move(InOther._Registry);
//    _Mapper = std::move(InOther._Mapper);
//#if WITH_EDITORONLY_DATA
//    _EntityID = std::move(InOther._EntityID);
//    _EntityNumber = std::move(InOther._EntityNumber);
//    _EntityVersion = std::move(InOther._EntityVersion);
//    _Fragments = std::move(InOther._Fragments);
//#endif
//
//    DoUpdate_MapperAndFragments();
//    return *this;
//}

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

auto FCk_Handle::operator*() -> TOptional<FCk_Registry>
{
    return _Registry;
}

auto FCk_Handle::operator*() const -> TOptional<FCk_Registry>
{
    return _Registry;
}

auto
    FCk_Handle::operator->() -> TOptional<FCk_Registry>
{
    return _Registry;
}

auto
    FCk_Handle::operator->() const -> TOptional<FCk_Registry>
{
    return _Registry;
}

auto FCk_Handle::IsValid() const -> bool
{
    return ck::IsValid(_Registry) && _Registry->IsValid(_Entity);
}

auto
    FCk_Handle::
    Get_ValidHandle(FEntityType::IdType InEntity) const
    -> ThisType
{
    CK_ENSURE_IF_NOT(IsValid(),
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
    if (NOT IsValid())
    { return; }

    if (Has<FEntity_FragmentMapper>())
    {
        _Mapper = &Get<FEntity_FragmentMapper>();

        if (ck::Is_NOT_Valid(_Fragments))
        {
            _Fragments = NewObject<UCk_Handle_FragmentsDebug>(GetTransientPackage());

            if (_Fragments->IsSafeForRootSet())
            {
                _Fragments->AddToRoot();
            }
        }

        _Fragments->_Names = _Mapper->ProcessAll(*this);
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    GetTypeHash(
        FCk_Handle InHandle) -> uint32
{
    return GetTypeHash(InHandle.Get_Entity()) + GetTypeHash(InHandle.Get_Registry());
}

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
}

// --------------------------------------------------------------------------------------------------------------------
