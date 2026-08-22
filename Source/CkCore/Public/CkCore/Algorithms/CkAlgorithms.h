#pragma once

#include "CkCore/Policy/CkPolicy.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Algo/Compare.h>
#include <Algo/MaxElement.h>
#include <Algo/MinElement.h>

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

    // for usage with std::visit and defining the handlers inline
    template<class... Ts> struct Overload : Ts... { using Ts::operator()...; };
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

    // True when both containers are the same size AND operator== (or the predicate) holds for
    // every aligned pair. A size mismatch is a plain false, not an error.
    template <typename T_ContainerA, typename T_ContainerB>
    [[nodiscard]]
    auto Compare(const T_ContainerA& InContainerA, const T_ContainerB& InContainerB) -> bool;

    template <typename T_ContainerA, typename T_ContainerB, typename T_PredicateFunction>
    [[nodiscard]]
    auto Compare(const T_ContainerA& InContainerA, const T_ContainerB& InContainerB, T_PredicateFunction InFunc) -> bool;

    template <typename T_ItrType, typename T_UnaryFunction>
    [[nodiscard]]
    auto FindIf(T_ItrType InItrBegin, T_ItrType InItrEnd, T_UnaryFunction InFunc) -> T_ItrType;

    template <typename T_ItrType, typename T_UnaryFunction>
    [[nodiscard]]
    auto FindIf(T_ItrType InItrBegin, T_ItrType InItrEnd, T_UnaryFunction InFunc) -> TOptional<decltype(*InItrBegin)>;

    template <typename T_ValueType, typename T_UnaryFunction>
    [[nodiscard]]
    auto FindIf(const TArray<T_ValueType>& InArray, T_UnaryFunction InFunc) -> TOptional<typename TArray<T_ValueType>::ElementType>;

    template <typename T_Container, typename T_ProjectionType>
    [[nodiscard]]
    auto MaxElement(T_Container& InContainer, T_ProjectionType InProj) -> TOptional<std::remove_reference_t<decltype(*Algo::MaxElement(InContainer, InProj))>>;

    template <typename T_Container, typename T_ProjectionType>
    [[nodiscard]]
    auto MinElement(T_Container& InContainer, T_ProjectionType InProj) -> TOptional<std::remove_reference_t<decltype(*Algo::MinElement(InContainer, InProj))>>;

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

    template <typename T_Container, typename T_UnaryFunction, ck::IsValidPolicy T_IsValidPolicy>
    auto ForEachIsValid(T_Container& InContainer, T_UnaryFunction InFunc, T_IsValidPolicy) -> void;

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
    auto FilterInPlace(T_Container& InContainer, T_PredicateFunction InFunc) -> void;

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

    template <typename T_ValueType, typename T_ProjectionFunc>
    [[nodiscard]]
    auto Intersect(const TArray<T_ValueType>& InContainerA, const TArray<T_ValueType>& InContainerB, T_ProjectionFunc InProjection) -> TArray<T_ValueType>;

    template <typename T_ValueType>
    [[nodiscard]]
    auto SymmetricDifference(const TArray<T_ValueType>& InContainerA, const TArray<T_ValueType>& InContainerB) -> TArray<T_ValueType>;

    template <typename T_ValueType>
    [[nodiscard]]
    auto Except(const TArray<T_ValueType>& InContainerA, const TArray<T_ValueType>& InContainerB) -> TArray<T_ValueType>;

    template <typename T_ValueType, typename T_ProjectionFunc>
    [[nodiscard]]
    auto Except(const TArray<T_ValueType>& InContainerA, const TArray<T_ValueType>& InContainerB, T_ProjectionFunc InProjection) -> TArray<T_ValueType>;

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

// Summary statistics over a numeric container. Each returns an unset TOptional for an empty input
// rather than a zero, because zero is a legitimate statistic and "nothing to measure" is not. Each
// copies and sorts internally, so the caller's container is never reordered.
namespace ck::algo
{
    template <typename T_Container>
    [[nodiscard]]
    auto Mean(const T_Container& InContainer) -> TOptional<double>;

    template <typename T_Container>
    [[nodiscard]]
    auto Median(const T_Container& InContainer) -> TOptional<double>;

    // Linear-interpolated percentile. InPercentile is a fraction in [0, 1]; values outside are
    // clamped. Percentile(c, 0.5) agrees with Median(c).
    template <typename T_Container>
    [[nodiscard]]
    auto Percentile(const T_Container& InContainer, double InPercentile) -> TOptional<double>;

    // Median of |value - median|. A robust spread estimator that, unlike standard deviation, is not
    // dragged around by the very outliers it is used to identify.
    //
    // Caveat worth knowing before relying on it: MAD collapses to zero whenever more than half the
    // values are identical, which is common in tightly clustered data. Callers using it as an
    // outlier threshold should fall back to MeanAbsoluteDeviation when it returns zero.
    template <typename T_Container>
    [[nodiscard]]
    auto MedianAbsoluteDeviation(const T_Container& InContainer) -> TOptional<double>;

    // Mean of |value - median|. Less robust than the median form — outliers do move it — but it does
    // not collapse when most values agree, which makes it the natural fallback for exactly the case
    // MedianAbsoluteDeviation cannot describe.
    template <typename T_Container>
    [[nodiscard]]
    auto MeanAbsoluteDeviation(const T_Container& InContainer) -> TOptional<double>;
}

// --------------------------------------------------------------------------------------------------------------------

#include "CkAlgorithms.inl.h"

// --------------------------------------------------------------------------------------------------------------------
