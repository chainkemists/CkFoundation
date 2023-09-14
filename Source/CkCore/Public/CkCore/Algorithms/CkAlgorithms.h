#pragma once

#include "CkMacros/CkMacros.h"

#include "CkTypeTraits/CkTypeTraits.h"

#include "CkValidation/CkIsValid.h"

#include <algorithm>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    template <typename T>
    class BasicDereference { auto operator()(T In) { return *In; } };

    template <typename T_Container>
    struct TToTransform
    {
        T_Container& Container;
    };

    template <typename T_Container>
    auto ToTransform(T_Container&& InContainer)
    {
        return TToTransform<std::_Remove_cvref_t<T_Container>>{InContainer};
    }

    template <typename T_Container, typename T_UnaryFunction>
    auto ForEach(T_Container& InContainer, T_UnaryFunction InFunc) -> void;

    template <typename T_ItrType, typename T_UnaryFunction>
    auto ForEach(T_ItrType InItrBegin, T_ItrType InItrEnd, T_UnaryFunction InFunc) -> void;

    template <typename T_ValueType, typename T_UnaryFunction>
    auto ForEach(TArray<T_ValueType>& InContainer, T_UnaryFunction InFunc) -> void;

    template <typename T_ValueType, typename T_UnaryFunction>
    auto ForEach(const TArray<T_ValueType>& InContainer, T_UnaryFunction InFunc) -> void;

    template <typename T_Container, typename T_UnaryFunction>
    auto ForEachIsValid(T_Container& InContainer, T_UnaryFunction InFunc) -> void;

    template <typename T_ItrType, typename T_UnaryFunction, typename T_Validator>
    auto ForEachIsValid(T_ItrType InItrBegin, T_ItrType InItrEnd, T_UnaryFunction InFunc, T_Validator InValidator) -> void;

    template <typename T_ItrType, typename T_UnaryFunction>
    auto ForEachIsValid(T_ItrType InItrBegin, T_ItrType InItrEnd, T_UnaryFunction InFunc) -> void;

    template <typename T_ValueType, typename T_UnaryFunction>
    auto ForEachRequest(TArray<T_ValueType>& InContainer, T_UnaryFunction InFunc) -> void;

    template <typename T_ValueType, typename T_UnaryFunction>
    auto ForEachRequest(TOptional<T_ValueType>& InContainer, T_UnaryFunction InFunc) -> void;

    template <typename T_ValueType, typename T_UnaryFunction>
    auto ForEachRequest(TArray<T_ValueType>& InContainer, T_UnaryFunction InFunc, policy::DontResetContainer) -> void;

    template <typename T_ValueType, typename T_UnaryFunction>
    auto ForEachRequest(TOptional<T_ValueType>& InContainer, T_UnaryFunction InFunc, policy::DontResetContainer) -> void;

    template <typename T_Array, typename T_UnaryFunction>
    auto ForEachReverse(T_Array& InArray, T_UnaryFunction InFunc) -> void;

    template <class T_ReturnContainer, class T_TransformFunc, class T_Container>
    auto Transform(T_Container& InContainer, T_TransformFunc InFunc) -> T_ReturnContainer;

    template <typename T_Container, typename T_PredicateFunction>
    auto Filter(T_Container& InContainer, T_PredicateFunction InFunc) -> T_Container;
}

// --------------------------------------------------------------------------------------------------------------------
// Definitions

namespace ck::algo
{
    template <typename T_Container, typename T_UnaryFunction>
    auto
        ForEach(
            T_Container& InContainer,
            T_UnaryFunction InFunc)
        -> void
    {
        ForEach(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_ItrType, typename T_UnaryFunction>
    auto
        ForEach(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            T_UnaryFunction InFunc)
        -> void
    {
        std::for_each(InItrBegin, InItrEnd, InFunc);
    }

    template <typename T_ValueType, typename T_UnaryFunction>
    auto
        ForEach(
            TArray<T_ValueType>& InContainer,
            T_UnaryFunction InFunc)
        -> void
    {
        std::for_each(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_ValueType, typename T_UnaryFunction>
    auto
        ForEach(
            const TArray<T_ValueType>& InContainer,
            T_UnaryFunction InFunc)
    -> void
    {
        std::for_each(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_Container, typename T_UnaryFunction>
    auto
        ForEachIsValid(
            T_Container& InContainer,
            T_UnaryFunction InFunc) -> void
    {
        ForEachIsValid(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_ItrType, typename T_UnaryFunction, typename T_Validator>
    auto
        ForEachIsValid(T_ItrType InItrBegin, T_ItrType InItrEnd, T_UnaryFunction InFunc, T_Validator InValidator)
        -> void
    {
        for (; InItrBegin != InItrEnd; ++InItrBegin)
        {
            if (NOT InValidator(*InItrBegin))
            { continue; }

            InFunc(*InItrBegin);
        }
    }

    template <typename T_ItrType, typename T_UnaryFunction>
    auto
        ForEachIsValid(T_ItrType InItrBegin, T_ItrType InItrEnd, T_UnaryFunction InFunc)
        -> void
    {
        ForEachIsValid(InItrBegin, InItrEnd, InFunc, [](auto InObj) { return ck::IsValid(InObj); });
    }

    template <typename T_ValueType, typename T_UnaryFunction>
    auto
        ForEachRequest(TArray<T_ValueType>& InContainer, T_UnaryFunction InFunc)
        -> void
    {
        ForEach(InContainer.begin(), InContainer.end(), InFunc);
        InContainer.Reset();
    }

    template <typename T_ValueType, typename T_UnaryFunction>
    auto
        ForEachRequest(TOptional<T_ValueType>& InContainer, T_UnaryFunction InFunc)
        -> void
    {
        if (InContainer.IsSet())
        {
            InFunc(*InContainer);
            InContainer.Reset();
        }
    }

    template <typename T_ValueType, typename T_UnaryFunction>
    auto
        ForEachRequest(TArray<T_ValueType>& InContainer, T_UnaryFunction InFunc, policy::DontResetContainer)
        -> void
    {
        ForEach(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_ValueType, typename T_UnaryFunction>
    auto
        ForEachRequest(TOptional<T_ValueType>& InContainer, T_UnaryFunction InFunc, policy::DontResetContainer)
        -> void
    {
        if (InContainer.IsSet())
        {
            InFunc(*InContainer);
        }
    }

    template <typename T_Array, typename T_UnaryFunction>
    auto ForEachReverse(T_Array& InArray, T_UnaryFunction InFunc) -> void
    {
        auto i = InArray.Num();
        while (i-- > 0)
        {
            InFunc(InArray[i]);
        }
    }

    template <typename T_ReturnContainer, typename T_TransformFunc, typename T_Container>
    auto
        Transform(
            T_Container& InContainer,
            T_TransformFunc InFunc)
        -> T_ReturnContainer
    {
        auto ToRet = T_ReturnContainer{};
        Transform(InContainer, ToTransform(ToRet), InFunc);
        return ToRet;
    }

    template <typename T_Container, typename T_PredicateFunction>
    auto
        Filter(
            T_Container& InContainer,
            T_PredicateFunction InFunc)
        -> T_Container
    {
        return InContainer.FilterByPredicate(InFunc);
    }
}

// --------------------------------------------------------------------------------------------------------------------
