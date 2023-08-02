#pragma once
#include "CkTypeTraits/CkTypeTraits.h"

namespace ck
{
    // Intended to be used in another class to force const-correctness on types
    // that are stored as pointers
    template <typename T_ComplexPtrType>
    class FPtrWrapper
    {
    public:
        using ValueType = T_ComplexPtrType;
        using StoredValueType = typename type_traits::extract_value_type<T_ComplexPtrType>::type;

    public:
        FPtrWrapper();

        template <typename... T_Args>
        FPtrWrapper(T_Args&&... InArgs);

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
    FPtrWrapper<T_ComplexPtrType>::FPtrWrapper()
        :  _Ptr(std::move(type_traits::make_new_ptr<ValueType>{}()))
    {
    }

    template <typename T_ComplexPtrType>
    template <typename ... T_Args>
    FPtrWrapper<T_ComplexPtrType>::FPtrWrapper(T_Args&&... InArgs)
        :  _Ptr(std::move(type_traits::make_new_ptr<ValueType>{}(std::forward<T_Args>(InArgs)...)))
    {
    }

    template <typename T_ComplexPtrType>
    auto FPtrWrapper<T_ComplexPtrType>::operator->() -> StoredValueType*
    {
        return _Ptr.Get();
    }

    template <typename T_ComplexPtrType>
    auto FPtrWrapper<T_ComplexPtrType>::operator->() const -> const StoredValueType*
    {
        return _Ptr.Get();
    }

    template <typename T_ComplexPtrType>
    auto FPtrWrapper<T_ComplexPtrType>::operator*() -> StoredValueType&
    {
        return *_Ptr;
    }

    template <typename T_ComplexPtrType>
    auto FPtrWrapper<T_ComplexPtrType>::operator*() const -> const StoredValueType&
    {
        return *_Ptr;
    }

    // --------------------------------------------------------------------------------------------------------------------
}
