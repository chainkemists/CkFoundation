#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Entity/CkEntity.h"
#include "CkEcs/Registry/CkRegistry.h"

// --------------------------------------------------------------------------------------------------------------------

struct FCk_Handle;

struct FCk_DebugWrapper
{
    using IdType = entt::id_type;

    virtual ~FCk_DebugWrapper() = default;

    virtual auto GetHash() const -> IdType = 0;
    virtual auto SetFragmentPointer(const void* InFragmentPtr) -> void = 0;
    virtual auto Get_FragmentName(const FCk_Handle& InHandle) const -> FName = 0;
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Fragment>
struct TCk_DebugWrapper : public FCk_DebugWrapper
{
public:
    explicit TCk_DebugWrapper(const T_Fragment* InPtr);

    auto GetHash() const -> IdType override;
    auto SetFragmentPointer(const void* InFragmentPtr) -> void override;
    auto Get_FragmentName(const FCk_Handle& InHandle) const -> FName override;

    auto operator==(const TCk_DebugWrapper& InOther) const -> bool;
    auto operator!=(const TCk_DebugWrapper& InOther) const -> bool;

private:
    const T_Fragment* _Fragment = nullptr;
    const IdType _Hash = entt::type_id<T_Fragment>().hash();
};

// --------------------------------------------------------------------------------------------------------------------

struct CKECS_API FEntity_FragmentMapper
{
public:
    CK_GENERATED_BODY(FEntity_FragmentMapper);

public:
    // ReSharper disable once CppInconsistentNaming
    static constexpr auto in_place_delete = true;

    // using raw pointer instead of smart pointers due to natvis complexity limits
    using DebugWrapperPtrType = FCk_DebugWrapper*;

public:
    template <typename T_Fragment>
    auto Add_FragmentInfo(const FCk_Handle& InHandle) const -> void;

    template <typename T_Fragment>
    auto Remove_FragmentInfo() const -> void;

public:
    ~FEntity_FragmentMapper();

private:
    mutable bool _IsHost = false;
    mutable DebugWrapperPtrType _DebugNameFragment;
    mutable DebugWrapperPtrType _LifetimeTag;
    mutable TArray<DebugWrapperPtrType> _AllTags;
    mutable TArray<DebugWrapperPtrType> _AllFragments;

    mutable const FName* _DebugName = nullptr;
    mutable FName _LifetimeTagName = NAME_None;
    mutable TArray<FName> _TagNames;
    mutable TArray<FName> _FragmentNames;

public:
    CK_PROPERTY_GET(_IsHost);
    CK_PROPERTY_GET(_DebugName);
    CK_PROPERTY_GET(_LifetimeTagName);
    CK_PROPERTY_GET(_TagNames);
    CK_PROPERTY_GET(_FragmentNames);
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Fragment>
TCk_DebugWrapper<T_Fragment>::
    TCk_DebugWrapper(const T_Fragment* InPtr)
    : _Fragment(InPtr)
{
}

template <typename T_Fragment>
auto
    TCk_DebugWrapper<T_Fragment>::
    GetHash() const
    -> IdType
{
    return entt::type_id<T_Fragment>().hash();
}

template <typename T_Fragment>
auto
    TCk_DebugWrapper<T_Fragment>::
    SetFragmentPointer(
        const void* InFragmentPtr)
    -> void
{
    _Fragment = static_cast<const T_Fragment*>(InFragmentPtr);
}

template <typename T_Fragment>
auto
    TCk_DebugWrapper<T_Fragment>::
    Get_FragmentName(
        const FCk_Handle& InHandle) const
    -> FName
{
    constexpr auto TypeString = ck::TypeToString<T_Fragment>;
    return FName{TypeString.begin(), TypeString.length()};
}

template <typename T_Fragment>
auto
    TCk_DebugWrapper<T_Fragment>::
    operator==(
        const TCk_DebugWrapper<T_Fragment>& InOther) const
    -> bool
{
    return GetHash() == InOther.GetHash();
}

template <typename T_Fragment>
auto
    TCk_DebugWrapper<T_Fragment>::
    operator!=(
        const TCk_DebugWrapper<T_Fragment>& InOther) const
    -> bool
{
    return GetHash() != InOther.GetHash();
}

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Fragment>
auto
    FEntity_FragmentMapper::
    Remove_FragmentInfo() const
    -> void
{
    constexpr auto TypeString = ck::TypeToString<T_Fragment>;
    const auto NameToCompare = FName{TypeString.begin(), TypeString.length()};

    if constexpr (std::is_empty_v<T_Fragment>)
    {
        _AllTags.RemoveAll([&](const DebugWrapperPtrType& InDebugWrapper)
        {
            return InDebugWrapper->GetHash() == entt::type_id<T_Fragment>().hash();
        });
        _TagNames.RemoveAll([&](const FName InName)
        {
            return InName == NameToCompare;
        });
    }
    else
    {
        _AllFragments.RemoveAll([&](const DebugWrapperPtrType& InDebugWrapper)
        {
            return InDebugWrapper->GetHash() == entt::type_id<T_Fragment>().hash();
        });
        _FragmentNames.RemoveAll([&](const FName InName)
        {
            return InName == NameToCompare;
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------
