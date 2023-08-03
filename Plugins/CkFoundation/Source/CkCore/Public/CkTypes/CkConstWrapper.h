#pragma once
#include "CkTypeTraits/CkTypeTraits.h"

namespace ck
{
    // Intended to be used in another class to force const-correctness on types
    // that are stored as pointers
    template <typename T_ComplexPtrType>
    class TPtrWrapper
    {
    public:
        using ValueType = T_ComplexPtrType;
        using ThisType = TPtrWrapper<ValueType>;
        using StoredValueType = typename type_traits::extract_value_type<T_ComplexPtrType>::type;

    public:
        TPtrWrapper();
        TPtrWrapper(const ThisType& InOther);
        TPtrWrapper(ThisType&& InOther) = default;

        template <typename... T_Args>
        TPtrWrapper(T_Args&&... InArgs);

    public:
        auto operator=(const ThisType& InOther) -> ThisType& ;
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
    TPtrWrapper<T_ComplexPtrType>::TPtrWrapper()
        :  _Ptr(std::move(type_traits::make_new_ptr<ValueType>{}()))
    {
    }

    template <typename T_ComplexPtrType>
    TPtrWrapper<T_ComplexPtrType>::TPtrWrapper(const ThisType& InOther)
        : _Ptr(ck::type_traits::move_or_copy_ptr<ValueType>{}(InOther._Ptr))
    {
    }

    template <typename T_ComplexPtrType>
    template <typename ... T_Args>
    TPtrWrapper<T_ComplexPtrType>::TPtrWrapper(T_Args&&... InArgs)
        :  _Ptr(std::move(type_traits::make_new_ptr<ValueType>{}(std::forward<T_Args>(InArgs)...)))
    {
    }

    template <typename T_ComplexPtrType>
    auto TPtrWrapper<T_ComplexPtrType>::operator=(const ThisType& InOther) -> ThisType&
    {
        _Ptr = ck::type_traits::move_or_copy_ptr<ValueType>{}(InOther._Ptr);
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
}
