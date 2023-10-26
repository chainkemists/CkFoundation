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

    virtual auto GetHash() -> IdType = 0;
    virtual auto SetFragmentPointer(const void* InFragmentPtr) -> void = 0;
};

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Fragment>
struct TCk_DebugWrapper : public FCk_DebugWrapper
{
public:
    explicit TCk_DebugWrapper(const T_Fragment* InPtr);

    auto GetHash() -> IdType override;
    auto SetFragmentPointer(const void* InFragmentPtr) -> void override;

    auto operator==(const TCk_DebugWrapper& InOther) -> bool;
    auto operator!=(const TCk_DebugWrapper& InOther) -> bool;

private:
    const T_Fragment* _Fragment = nullptr;
    const IdType _Hash = entt::type_id<T_Fragment>().hash();
};

// --------------------------------------------------------------------------------------------------------------------

struct FEntity_FragmentMapper
{
public:
    CK_GENERATED_BODY(FEntity_FragmentMapper);

public:
    // ReSharper disable once CppInconsistentNaming
    static constexpr auto in_place_delete = true;

    using DebugWrapperPtrType = TSharedPtr<FCk_DebugWrapper, ESPMode::NotThreadSafe>;

public:
    struct Concept_GetFragment : entt::type_list<DebugWrapperPtrType(const FCk_Handle&) const, FName()>
    {
        template <typename Base>
        struct type : Base
        {
            auto Get_Fragment(const FCk_Handle& InHandle) const -> DebugWrapperPtrType;
            auto Get_FragmentName() -> FName;
        };

        template <typename Type>
        using impl = entt::value_list<&Type::Get_Fragment, &Type::Get_FragmentName>;
    };

public:
    template <typename T_Fragment>
    struct ConceptImpl_GetFragment
    {
        using ValueType = T_Fragment;
        auto Get_Fragment(const FCk_Handle& InHandle) const -> DebugWrapperPtrType;
        auto Get_FragmentName() -> FName;
    };

public:
    template <typename T_Fragment>
    auto Add_FragmentGetter() const -> void;

    auto ProcessAll(const FCk_Handle& InHandle) const -> TArray<FName>;

public:
    using Concept_GetFragment_PolyType = entt::poly<Concept_GetFragment>;

private:
    mutable TArray<DebugWrapperPtrType> _AllFragments;
    mutable TArray<Concept_GetFragment_PolyType> _GetFragments;
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
    GetHash()
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
    operator==(const TCk_DebugWrapper<T_Fragment>& InOther)
    -> bool
{
    return GetHash() == InOther.GetHash();
}

template <typename T_Fragment>
auto
    TCk_DebugWrapper<T_Fragment>::
    operator!=(const TCk_DebugWrapper<T_Fragment>& InOther)
    -> bool
{
    return GetHash() != InOther.GetHash();
}

// --------------------------------------------------------------------------------------------------------------------

template <typename Base>
auto
    FEntity_FragmentMapper::Concept_GetFragment::type<Base>::
    Get_Fragment(
        const FCk_Handle& InHandle) const
    -> DebugWrapperPtrType
{
    return entt::poly_call<0>(*this, InHandle);
}

template <typename Base>
auto
    FEntity_FragmentMapper::Concept_GetFragment::type<Base>::
    Get_FragmentName()
    -> FName
{
    return entt::poly_call<1>(*this);
}

// --------------------------------------------------------------------------------------------------------------------

// Get_Fragment defined in CkHandle.h due to circular dependency

template <typename T_Fragment>
auto
    FEntity_FragmentMapper::
    ConceptImpl_GetFragment<T_Fragment>::Get_FragmentName()
    -> FName
{
    return FName{ck::TypeToString<T_Fragment>.begin(), ck::TypeToString<T_Fragment>.length()};
}

// --------------------------------------------------------------------------------------------------------------------

template <typename T_Fragment>
auto
    FEntity_FragmentMapper::
    Add_FragmentGetter() const
    -> void
{
    _GetFragments.Emplace(ConceptImpl_GetFragment<T_Fragment>{});
}

// --------------------------------------------------------------------------------------------------------------------
