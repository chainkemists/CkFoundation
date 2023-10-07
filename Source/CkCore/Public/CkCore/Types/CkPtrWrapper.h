#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"

namespace ck
{
    // Intended to be used in another class to force const-correctness on types
    // that are stored as pointers
    template <typename T_ComplexPtrType>
    class TPtrWrapper
    {
        CK_GENERATED_BODY(TPtrWrapper<T_ComplexPtrType>);

    public:
        using ValueType = T_ComplexPtrType;
        using StoredValueType = typename type_traits::ExtractValueType<T_ComplexPtrType>::type;

    public:
        TPtrWrapper();
        TPtrWrapper(const ThisType& InOther);
        TPtrWrapper(ThisType&& InOther) = default;

        template <typename... T_Args>
        TPtrWrapper(T_Args&&... InArgs);

    public:
        auto operator=(const ThisType& InOther) -> ThisType&;
        auto operator=(ThisType&& InOther) -> ThisType & = default;

    public:
        auto operator->() -> StoredValueType*;
        auto operator->() const -> const StoredValueType*;

        auto operator*() -> StoredValueType&;
        auto operator*() const -> const StoredValueType&;

    private:
        ValueType _Ptr;
    };

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_ComplexPtrType>
    auto GetTypeHash(const TPtrWrapper<T_ComplexPtrType>& InPtrWrapper) -> uint32;

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_ComplexPtrType>
    TPtrWrapper<T_ComplexPtrType>::TPtrWrapper()
        :  _Ptr(std::move(type_traits::MakeNewPtr<ValueType>{}()))
    {
    }

    template <typename T_ComplexPtrType>
    TPtrWrapper<T_ComplexPtrType>::TPtrWrapper(const ThisType& InOther)
        : _Ptr(ck::type_traits::MoveOrCopyPtr<ValueType>{}(InOther._Ptr))
    {
    }

    template <typename T_ComplexPtrType>
    template <typename ... T_Args>
    TPtrWrapper<T_ComplexPtrType>::TPtrWrapper(T_Args&&... InArgs)
        :  _Ptr(std::move(type_traits::MakeNewPtr<ValueType>{}(std::forward<T_Args>(InArgs)...)))
    {
    }

    template <typename T_ComplexPtrType>
    auto TPtrWrapper<T_ComplexPtrType>::operator=(const ThisType& InOther) -> ThisType&
    {
        _Ptr = ck::type_traits::MoveOrCopyPtr<ValueType>{}(InOther._Ptr);
        return *this;
    }

    template <typename T_ComplexPtrType>
    auto TPtrWrapper<T_ComplexPtrType>::operator->() -> StoredValueType*
    {
        return _Ptr.Get();
    }

    template <typename T_ComplexPtrType>
    auto TPtrWrapper<T_ComplexPtrType>::operator->() const -> const StoredValueType*
    {
        return _Ptr.Get();
    }

    template <typename T_ComplexPtrType>
    auto TPtrWrapper<T_ComplexPtrType>::operator*() -> StoredValueType&
    {
        return *_Ptr;
    }

    template <typename T_ComplexPtrType>
    auto TPtrWrapper<T_ComplexPtrType>::operator*() const -> const StoredValueType&
    {
        return *_Ptr;
    }

    // --------------------------------------------------------------------------------------------------------------------

    template <typename T_ComplexPtrType>
    auto
    GetTypeHash(const TPtrWrapper<T_ComplexPtrType>& InPtrWrapper) -> uint32
    {
        return PointerHash(&*InPtrWrapper);
    }

    // --------------------------------------------------------------------------------------------------------------------
}

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_IS_VALID_EXECUTOR_ISBASEOF_T(TPtrWrapper);
CK_DEFINE_CUSTOM_IS_VALID_T(T, ck::TPtrWrapper<T>, ck::IsValid_Policy_Default, [=](const ck::TPtrWrapper<T>& InPtr)
{
    return ck::IsValid(InPtr._Ptr);
});

// --------------------------------------------------------------------------------------------------------------------
