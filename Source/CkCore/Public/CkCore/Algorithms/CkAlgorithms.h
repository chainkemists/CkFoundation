#pragma once

#include "CkCore/TypeTraits/CkTypeTraits.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    template <typename T>
    class BasicDereference { auto operator()(T In) { return *In; } };

    template <typename T_Container>
    struct TToTransform
    {
        T_Container& _Container;
    };

    template <typename T_Container>
    auto ToTransform(T_Container&& InContainer)
    {
        return TToTransform<std::_Remove_cvref_t<T_Container>>{InContainer};
    }
}

namespace ck::algo
{
    template <typename T_Container, typename T_PredicateFunction>
    [[nodiscard]]
    auto AllOf(const T_Container& InContainer, T_PredicateFunction InFunc) -> bool;

    template <typename T_ItrType, typename T_PredicateFunction>
    [[nodiscard]]
    auto AllOf(T_ItrType InItrBegin, T_ItrType InItrEnd, T_PredicateFunction InFunc) -> bool;

    template <typename T_Container, typename T_PredicateFunction>
    [[nodiscard]]
    auto AnyOf(const T_Container& InContainer, T_PredicateFunction InFunc) -> bool;

    template <typename T_ItrType, typename T_PredicateFunction>
    [[nodiscard]]
    auto AnyOf(T_ItrType InItrBegin, T_ItrType InItrEnd, T_PredicateFunction InFunc) -> bool;

    template <typename T_Container, typename T_PredicateFunction>
    [[nodiscard]]
    auto NoneOf(const T_Container& InContainer, T_PredicateFunction InFunc) -> bool;

    template <typename T_ItrType, typename T_PredicateFunction>
    [[nodiscard]]
    auto NoneOf(T_ItrType InItrBegin, T_ItrType InItrEnd, T_PredicateFunction InFunc) -> bool;

    template <typename T_Container, typename T_PredicateFunction>
    [[nodiscard]]
    auto CountIf(const T_Container& InContainer, T_PredicateFunction InFunc) -> int32;

    template <typename T_Container, typename T_PredicateFunction>
    [[nodiscard]]
    auto FindIndex(const T_Container& InContainer, T_PredicateFunction InFunc) -> int32;

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
    auto ForEachRequest(const TArray<T_ValueType>& InContainer, T_UnaryFunction InFunc, policy::DontResetContainer) -> void;

    template <typename T_ValueType, typename T_UnaryFunction>
    auto ForEachRequest(const TOptional<T_ValueType>& InContainer, T_UnaryFunction InFunc, policy::DontResetContainer) -> void;

    template <typename T_Array, typename T_UnaryFunction>
    auto ForEachReverse(T_Array& InArray, T_UnaryFunction InFunc) -> void;

    template <class T_ReturnContainer, class T_TransformFunc, class T_Container>
    [[nodiscard]]
    auto Transform(const T_Container& InContainer, T_TransformFunc InFunc) -> T_ReturnContainer;

    template <class T_ReturnContainer, class T_TransformFunc, class T_ItrType>
    [[nodiscard]]
    auto Transform(T_ItrType InItrBegin, T_ItrType InItrEnd, T_TransformFunc InFunc) -> T_ReturnContainer;

    template <class T_ReturnContainer, class T_TransformFunc, class T_Container>
    auto Transform(const T_Container& InContainer, TToTransform<T_ReturnContainer> InReturnContainer, T_TransformFunc InFunc) -> void;

    template <class T_ReturnContainer, class T_TransformFunc, class T_ItrType>
    auto Transform(T_ItrType InItrBegin, T_ItrType InItrEnd, TToTransform<T_ReturnContainer> InReturnContainer, T_TransformFunc InFunc) -> void;

    template <typename T_Container, typename T_PredicateFunction>
    [[nodiscard]]
    auto Filter(const T_Container& InContainer, T_PredicateFunction InFunc) -> T_Container;

    template <typename T_Container, typename T_PredicateFunction>
    auto Sort(T_Container& InContainer, T_PredicateFunction InFunc) -> void;

    template <typename T_Container, typename T_PredicateFunction>
    [[nodiscard]]
    auto Sort(const T_Container& InContainer, T_PredicateFunction InFunc) -> T_Container;

    template <typename T_Container>
    auto Sort(T_Container& InContainer) -> void;

    template <typename T_Container>
    [[nodiscard]]
    auto Sort(const T_Container& InContainer) -> T_Container;

    template <typename T_ValueType>
    [[nodiscard]]
    auto Intersect(const TArray<T_ValueType>& InContainerA, const TArray<T_ValueType>& InContainerB) -> TArray<T_ValueType>;

    template <typename T_ValueType>
    [[nodiscard]]
    auto Except(const TArray<T_ValueType>& InContainerA, const TArray<T_ValueType>& InContainerB) -> TArray<T_ValueType>;

    template <typename T_ValueType>
    [[nodiscard]]
    auto PartialSum(const TArray<T_ValueType>& InWeights) -> TArray<T_ValueType>;
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    template <typename T_Func, typename T_ContainerA, typename T_ContainerB>
    static auto ForEachView(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_Func InFunc) -> void;

    template <typename T_Func, typename T_ContainerA, typename T_ContainerB, typename T_ContainerC>
    static auto ForEachView(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_Func InFunc) -> void;

    template <typename T_Func, typename T_ContainerA, typename T_ContainerB, typename T_ContainerC, typename T_ContainerD>
    static auto ForEachView(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_ContainerD& InContainerD,
        T_Func InFunc) -> void;

    template <typename T_Func, typename T_ContainerA, typename T_ContainerB, typename T_ContainerC, typename
              T_ContainerD, typename T_ContainerE>
    static auto ForEachView(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_ContainerD& InContainerD,
        T_ContainerE& InContainerE,
        T_Func InFunc) -> void;

    template <typename T_TransformFunc, typename T_ContainerA, typename T_ContainerB, typename T_ReturnContainer>
    static auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        TToTransform<T_ReturnContainer> InReturnContainer,
        T_TransformFunc InFunc) -> void;

    template <class T_ReturnContainer, class T_TransformFunc, class T_ContainerA, typename T_ContainerB>
    [[nodiscard]]
    static auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_TransformFunc InFunc) -> T_ReturnContainer;

    template <typename T_TransformFunc, typename T_ContainerA, typename T_ContainerB, typename T_ContainerC, typename T_ReturnContainer>
    static auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        TToTransform<T_ReturnContainer> InReturnContainer,
        T_TransformFunc InFunc) -> void;

    template <class T_ReturnContainer, class T_TransformFunc, class T_ContainerA, typename T_ContainerB, typename T_ContainerC>
    [[nodiscard]]
    static auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_TransformFunc InFunc) -> T_ReturnContainer;

    template <typename T_TransformFunc, typename T_ContainerA, typename T_ContainerB, typename T_ContainerC, typename
              T_ContainerD, typename T_ReturnContainer>
    static auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_ContainerD& InContainerD,
        TToTransform<T_ReturnContainer> InReturnContainer,
        T_TransformFunc InFunc) -> void;

    template <class T_ReturnContainer, class T_TransformFunc, class T_ContainerA, typename T_ContainerB, typename
              T_ContainerC, typename T_ContainerD>
    [[nodiscard]]
    static auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_ContainerD& InContainerD,
        T_TransformFunc InFunc) -> T_ReturnContainer;

    template <typename T_TransformFunc, typename T_ContainerA, typename T_ContainerB, typename T_ContainerC, typename
              T_ContainerD, typename T_ContainerE, typename T_ReturnContainer>
    static auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_ContainerD& InContainerD,
        T_ContainerE& InContainerE,
        TToTransform<T_ReturnContainer> InReturnContainer,
        T_TransformFunc InFunc) -> void;

    template <class T_ReturnContainer, class T_TransformFunc, class T_ContainerA, typename T_ContainerB, typename
              T_ContainerC, typename T_ContainerD, typename T_ContainerE>
    [[nodiscard]]
    static auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_ContainerD& InContainerD,
        T_ContainerE& InContainerE,
        T_TransformFunc InFunc) -> T_ReturnContainer;
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAlgorithms.inl.h"

// --------------------------------------------------------------------------------------------------------------------
