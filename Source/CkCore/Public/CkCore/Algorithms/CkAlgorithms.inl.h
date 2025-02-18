#pragma once

#include "CkAlgorithms.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Algo/Count.h>
#include <Algo/Sort.h>
#include <Algo/Find.h>
#include <Algo/RemoveIf.h>
#include <Algo/MaxElement.h>
#include <Algo/MinElement.h>

#include <algorithm>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    template <typename T_Container, typename T_PredicateFunction>
    auto
        AllOf(
            const T_Container& InContainer,
            T_PredicateFunction InFunc)
        -> bool
    {
        return AllOf(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_ItrType, typename T_PredicateFunction>
    auto
        AllOf(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            T_PredicateFunction InFunc)
        -> bool
    {
        return std::all_of(InItrBegin, InItrEnd, InFunc);
    }

    template <typename T_Container, typename T_PredicateFunction>
    auto
        AnyOf(
            const T_Container& InContainer,
            T_PredicateFunction InFunc)
        -> bool
    {
        return AnyOf(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_ItrType, typename T_PredicateFunction>
    auto
        AnyOf(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            T_PredicateFunction InFunc)
        -> bool
    {
        return std::any_of(InItrBegin, InItrEnd, InFunc);
    }

    template <typename T_ItrType, typename T_UnaryFunction>
    auto
        FindIf(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            T_UnaryFunction InFunc)
        -> T_ItrType
    {
        return std::find_if(InItrBegin, InItrEnd, InFunc);
    }

    template <typename T_ItrType, typename T_UnaryFunction>
    auto
        FindIf(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            T_UnaryFunction InFunc)
        -> TOptional<decltype(*InItrBegin)>
    {
        auto Ret = std::find_if(InItrBegin, InItrEnd, InFunc);

        if (Ret == InItrEnd)
        { return {}; }

        return *Ret;
    }

    template <typename T_ValueType, typename T_UnaryFunction>
    auto
        FindIf(
            const TArray<T_ValueType>& InArray,
            T_UnaryFunction InFunc)
        -> TOptional<typename TArray<T_ValueType>::ElementType>
    {
        const auto* Found = InArray.FindByPredicate(InFunc);

        if (ck::Is_NOT_Valid(Found, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        return *Found;
    }

    template <typename T_ItrType, typename T_UnaryFunction>
    auto
        RemoveIf(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            T_UnaryFunction InFunc)
        -> void
    {
        std::remove_if(InItrBegin, InItrEnd, InFunc);
    }

    template <typename T_ValueType, typename T_UnaryFunction>
    auto
        RemoveIf(
            TArray<T_ValueType>& InArray,
            T_UnaryFunction InFunc)
        -> void
    {
        // Algo::RemoveIf is marked nodiscard but we don't need the return value
        [[maybe_unused]] const auto NumRemoved = Algo::RemoveIf(InArray, InFunc);
    }

    template <typename T_Container, typename T_ProjectionType>
    auto
        MaxElement(
            T_Container& InContainer,
            T_ProjectionType InProj)
        -> TOptional<std::remove_reference_t<decltype(*Algo::MaxElement(InContainer, InProj))>>
    {
        auto MaxElement = AlgoImpl::MaxElementBy(InContainer, MoveTemp(InProj), TLess<>());

        if (ck::Is_NOT_Valid(MaxElement, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        return *MaxElement;
    }

    template <typename T_Container, typename T_ProjectionType>
    auto
        MinElement(
            T_Container& InContainer,
            T_ProjectionType InProj)
        -> TOptional<std::remove_reference_t<decltype(*Algo::MinElement(InContainer, InProj))>>
    {
        auto MinElement = AlgoImpl::MinElementBy(InContainer, MoveTemp(InProj), TLess<>());

        if (ck::Is_NOT_Valid(MinElement, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        return *MinElement;
    }

    template <typename T_ValueType, typename T_ComparatorType>
    auto
        MinElement(
            TArray<T_ValueType>& InContainer,
            T_ComparatorType InFunc)
        -> TOptional<T_ValueType>
    {
        const auto& MinElement = Algo::MinElement(InContainer, InFunc);

        if (ck::Is_NOT_Valid(MinElement, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        return *MinElement;
    }

    template <typename T_ValueType, typename T_ComparatorType, typename T_ProjectionFunction>
    auto
        MinElement(
            TArray<T_ValueType>& InContainer,
            T_ComparatorType InFunc,
            T_ProjectionFunction InProj)
        -> TOptional<T_ValueType>
    {
        const auto& MinElement = Algo::MinElementBy(InContainer, InProj, InFunc);

        if (ck::Is_NOT_Valid(MinElement, ck::IsValid_Policy_NullptrOnly{}))
        { return {}; }

        return *MinElement;
    }

    template <typename T_Container, typename T_PredicateFunction>
    auto
        NoneOf(
            const T_Container& InContainer,
            T_PredicateFunction InFunc)
        -> bool
    {
        return NoneOf(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_ItrType, typename T_PredicateFunction>
    auto
        NoneOf(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            T_PredicateFunction InFunc)
        -> bool
    {
        return std::none_of(InItrBegin, InItrEnd, InFunc);
    }

    template <typename T_Container, typename T_PredicateFunction>
    auto
        CountIf(
            const T_Container& InContainer,
            T_PredicateFunction InFunc)
        -> int32
    {
       return Algo::CountIf(InContainer, InFunc);
    }

    template <typename T_Container, typename T_PredicateFunction>
    auto
        FindIndex(
            const T_Container& InContainer,
            T_PredicateFunction InFunc)
        -> int32
    {
        using ElementType = typename T_Container::ElementType;
        return InContainer.IndexOfByPredicate(InFunc);
    }

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
            T_UnaryFunction InFunc)
        -> void
    {
        ForEachIsValid(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_Container, typename T_UnaryFunction, ck::IsValidPolicy T_IsValidPolicy>
    auto
        ForEachIsValid(
            T_Container& InContainer,
            T_UnaryFunction InFunc,
            T_IsValidPolicy)
        -> void
    {
        ForEachIsValid(InContainer.begin(), InContainer.end(), InFunc, [](auto InObj) { return ck::IsValid(InObj, T_IsValidPolicy{}); });
    }

    template <typename T_ItrType, typename T_UnaryFunction, typename T_Validator>
    auto
        ForEachIsValid(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            T_UnaryFunction InFunc,
            T_Validator InValidator)
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
        ForEachIsValid(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            T_UnaryFunction InFunc)
        -> void
    {
        ForEachIsValid(InItrBegin, InItrEnd, InFunc, [](auto InObj) { return ck::IsValid(InObj); });
    }

    template <typename T_ValueType, typename T_UnaryFunction>
    auto
        ForEachRequest(
            TArray<T_ValueType>& InContainer,
            T_UnaryFunction InFunc)
        -> void
    {
        ForEach(InContainer.begin(), InContainer.end(), InFunc);
        InContainer.Reset();
    }

    template <typename T_ValueType, typename T_UnaryFunction>
    auto
        ForEachRequest(
            TOptional<T_ValueType>& InContainer,
            T_UnaryFunction InFunc)
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
        ForEachRequest(
            const TArray<T_ValueType>& InContainer,
            T_UnaryFunction InFunc,
            policy::DontResetContainer)
        -> void
    {
        ForEach(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_ValueType, typename T_UnaryFunction>
    auto
        ForEachRequest(
            const TOptional<T_ValueType>& InContainer,
            T_UnaryFunction InFunc,
            policy::DontResetContainer)
        -> void
    {
        if (InContainer.IsSet())
        {
            InFunc(*InContainer);
        }
    }

    template <typename T_Array, typename T_UnaryFunction>
    auto
        ForEachReverse(
            T_Array& InArray,
            T_UnaryFunction InFunc)
        -> void
    {
        auto Index = InArray.Num();
        while (Index-- > 0)
        {
            InFunc(InArray[Index]);
        }
    }

    template <typename T_ReturnContainer, typename T_TransformFunc, typename T_Container>
    auto
        Transform(
            const T_Container& InContainer,
            T_TransformFunc InFunc)
        -> T_ReturnContainer
    {
        auto ToRet = T_ReturnContainer{};
        Transform(InContainer, ToTransform(ToRet), InFunc);
        return ToRet;
    }

    template <class T_ReturnContainer, class T_TransformFunc, class T_ItrType>
    auto
        Transform(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            T_TransformFunc InFunc)
        -> T_ReturnContainer
    {
        auto ToRet = T_ReturnContainer{};
        Transform(InItrBegin, InItrEnd, ToTransform(ToRet), InFunc);
        return ToRet;
    }

    template <typename T_ReturnContainer, typename T_TransformFunc, typename T_Container>
    auto
        Transform(
            const T_Container& InContainer,
            TToTransform<T_ReturnContainer> InReturnContainer,
            T_TransformFunc InFunc)
        -> void
    {
        for (int Index = 0; Index < InContainer.Num(); ++Index)
        {
            InReturnContainer._Container.Add(InFunc(InContainer[Index]));
        }
    }

    template <class T_ReturnContainer, class T_TransformFunc, class T_ItrType>
    auto
        Transform(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            TToTransform<T_ReturnContainer> InReturnContainer,
            T_TransformFunc InFunc)
        -> void
    {
        for (; InItrBegin != InItrEnd; ++InItrBegin)
        {
            InReturnContainer._Container.Add(InFunc(*InItrBegin));
        }
    }

    template <typename T_Container, typename T_PredicateFunction>
    auto
        Filter(
            const T_Container& InContainer,
            T_PredicateFunction InFunc)
        -> T_Container
    {
        return InContainer.FilterByPredicate(InFunc);
    }

    template <typename T_Container, typename T_PredicateFunction>
    auto
        Sort(
            T_Container& InContainer,
            T_PredicateFunction InFunc)
        -> void
    {
        Algo::Sort(InContainer, InFunc);
    }

    template <typename T_Container, typename T_PredicateFunction>
    auto
        Sort(
            const T_Container& InContainer,
            T_PredicateFunction InFunc)
        -> T_Container
    {
        auto SortedContainer = InContainer;
        Sort(SortedContainer, InFunc);

        return SortedContainer;
    }

    template <typename T_Container>
    auto
        Sort(
            const T_Container& InContainer)
        -> T_Container
    {
        auto SortedContainer = InContainer;
        Sort(SortedContainer);

        return SortedContainer;
    }

    template <typename T_ValueType>
    auto
        Intersect(
            const TArray<T_ValueType>& InContainerA,
            const TArray<T_ValueType>& InContainerB)
        -> TArray<T_ValueType>
    {
        auto Intersection = std::vector<T_ValueType>{};
        Intersection.reserve(FMath::Min(InContainerA.Num(), InContainerB.Num()));

        const auto SortedContainerA = ck::algo::Sort(InContainerA);
        const auto SortedContainerB = ck::algo::Sort(InContainerB);

        std::set_intersection
        (
            SortedContainerA.begin(),
            SortedContainerA.end(),
            SortedContainerB.begin(),
            SortedContainerB.end(),
            std::back_inserter(Intersection)
        );

        auto Result = TArray<T_ValueType>{};
        Result.Reserve(Intersection.size());
        Result.Append(Intersection.data(), Intersection.size());

        return Result;
    }

    template <typename T_ValueType>
    auto
        Except(
            const TArray<T_ValueType>& InContainerA,
            const TArray<T_ValueType>& InContainerB)
        -> TArray<T_ValueType>
    {
        auto Difference = std::vector<T_ValueType>{};
        Difference.reserve(FMath::Min(InContainerA.Num(), InContainerB.Num()));

        const auto SortedContainerA = ck::algo::Sort(InContainerA);
        const auto SortedContainerB = ck::algo::Sort(InContainerB);

        std::set_difference
        (
            SortedContainerA.begin(),
            SortedContainerA.end(),
            SortedContainerB.begin(),
            SortedContainerB.end(),
            std::back_inserter(Difference)
        );

        auto Result = TArray<T_ValueType>{};
        Result.Reserve(Difference.size());
        Result.Append(Difference.data(), Difference.size());

        return Result;
    }

    template <typename T_ValueType>
    auto
        PartialSum(
            const TArray<T_ValueType>& InWeights)
        -> TArray<T_ValueType>
    {
        auto PartialSums = TArray<T_ValueType>{};
        PartialSums.AddUninitialized(InWeights.Num());

        auto CumulativeSum = 0.0f;
        for (auto Index = 0; Index < InWeights.Num(); ++Index)
        {
            CumulativeSum += InWeights[Index];
            PartialSums[Index] = CumulativeSum;
        }

        return PartialSums;
    }

    template <typename T_Container>
    auto
        Sort(
            T_Container& InContainer)
        -> void
    {
        Algo::Sort(InContainer);
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    template <typename T_Func, typename T_ContainerA, typename T_ContainerB>
    auto
        ForEachView(
            T_ContainerA& InContainerA,
            T_ContainerB& InContainerB,
            T_Func InFunc)
        -> void
    {
        CK_ENSURE_IF_NOT(InContainerA.Num() == InContainerB.Num(), TEXT("Non-Matching Container sizes"))
        { return; }

        for (int Index = 0; Index < InContainerA.Num(); ++Index)
        {
            InFunc(InContainerA[Index], InContainerB[Index]);
        }
    }

    template <typename T_Func, typename T_ContainerA, typename T_ContainerB, typename T_ContainerC>
    auto
        ForEachView(
            T_ContainerA& InContainerA,
            T_ContainerB& InContainerB,
            T_ContainerC& InContainerC,
            T_Func InFunc)
        -> void
    {
        CK_ENSURE_IF_NOT(
            InContainerA.Num() == InContainerB.Num() == InContainerC.Num(), TEXT("Non-Matching Container sizes"))
        { return; }

        for (int Index = 0; Index < InContainerA.Num(); ++Index)
        {
            InFunc(InContainerA[Index], InContainerB[Index], InContainerC[Index]);
        }
    }

    template <typename T_Func,	typename T_ContainerA,	typename T_ContainerB,	typename T_ContainerC,	typename T_ContainerD>
    auto
        ForEachView(
            T_ContainerA& InContainerA,
            T_ContainerB& InContainerB,
            T_ContainerC& InContainerC,
            T_ContainerD& InContainerD,
            T_Func InFunc)
        -> void
    {
        CK_ENSURE_IF_NOT(
            InContainerA.Num() == InContainerB.Num() == InContainerC.Num() == InContainerD.Num(),
            TEXT("Non-Matching Container sizes"))
        { return; }

        for (int Index = 0; Index < InContainerA.Num(); ++Index)
        {
            InFunc(InContainerA[Index], InContainerB[Index], InContainerC[Index], InContainerD[Index]);
        }
    }

    template <typename T_Func, typename T_ContainerA,	typename T_ContainerB,	typename T_ContainerC,	typename T_ContainerD,	typename T_ContainerE>
    auto
        ForEachView(
            T_ContainerA& InContainerA,
            T_ContainerB& InContainerB,
            T_ContainerC& InContainerC,
            T_ContainerD& InContainerD,
            T_ContainerE& InContainerE,
            T_Func InFunc)
        -> void
    {
        CK_ENSURE_IF_NOT(
            InContainerA.Num() == InContainerB.Num() == InContainerC.Num() == InContainerD.Num() == InContainerE.Num(),
            TEXT("Non-Matching Container sizes"))
        { return; }

        for (int Index = 0; Index < InContainerA.Num(); ++Index)
        {
            InFunc(InContainerA[Index], InContainerB[Index], InContainerC[Index], InContainerD[Index], InContainerE[Index]);
        }
    }

    template <typename T_TransformFunc, typename T_ContainerA, typename T_ContainerB, typename T_ReturnContainer>
    auto
        ForEachViewTransform(
            T_ContainerA& InContainerA,
            T_ContainerB& InContainerB,
            TToTransform<T_ReturnContainer> InReturnContainer,
            T_TransformFunc InFunc)
        -> void
    {
        CK_ENSURE_IF_NOT(InContainerA.Num() == InContainerB.Num(), TEXT("Non-Matching Container sizes"))
        { return; }

        for (auto Index = 0; Index < InContainerA.Num(); ++Index)
        {
            InReturnContainer.Container.Add(InFunc(InContainerA[Index], InContainerB[Index]));
        }
    }

    template <class T_ReturnContainer, class T_TransformFunc, class T_ContainerA, typename T_ContainerB>
    auto
        ForEachViewTransform(
            T_ContainerA& InContainerA,
            T_ContainerB& InContainerB,
            T_TransformFunc InFunc)
        -> T_ReturnContainer
    {
        auto ToRet = T_ReturnContainer{};
        ForEachViewTransform(InContainerA, InContainerB, ToTransform(ToRet), InFunc);
        return ToRet;
    }

    template <typename T_TransformFunc,	typename T_ContainerA,	typename T_ContainerB,	typename T_ContainerC,	typename T_ReturnContainer>
    auto
        ForEachViewTransform(
            T_ContainerA& InContainerA,
            T_ContainerB& InContainerB,
            T_ContainerC& InContainerC,
            TToTransform<T_ReturnContainer> InReturnContainer,
            T_TransformFunc InFunc)
        -> void
    {
        CK_ENSURE_IF_NOT(
            InContainerA.Num() == InContainerB.Num() == InContainerC.Num(), TEXT("Non-Matching Container sizes"))
        { return; }

        for (int Index = 0; Index < InContainerA.Num(); ++Index)
        {
            InReturnContainer.Container.Add(InFunc(InContainerA[Index], InContainerB[Index], InContainerC[Index]));
        }
    }

    template <class T_ReturnContainer, class T_TransformFunc, class T_ContainerA, typename T_ContainerB, typename T_ContainerC>
    auto
        ForEachViewTransform(
            T_ContainerA& InContainerA,
            T_ContainerB& InContainerB,
            T_ContainerC& InContainerC,
            T_TransformFunc InFunc)
        -> T_ReturnContainer
    {
        auto ToRet = T_ReturnContainer{};
        ForEachViewTransform(InContainerA, InContainerB, InContainerC, ToTransform(ToRet), InFunc);
        return ToRet;
    }

    template <typename T_TransformFunc,	typename T_ContainerA,	typename T_ContainerB,	typename T_ContainerC,	typename T_ContainerD,	typename T_ReturnContainer>
    auto
        ForEachViewTransform(
            T_ContainerA& InContainerA,
            T_ContainerB& InContainerB,
            T_ContainerC& InContainerC,
            T_ContainerD& InContainerD,
            TToTransform<T_ReturnContainer> InReturnContainer,
            T_TransformFunc InFunc)
        -> void
    {
        CK_ENSURE_IF_NOT(
            InContainerA.Num() == InContainerB.Num() == InContainerC.Num() == InContainerD.Num(),
            TEXT("Non-Matching Container sizes"))
        { return; }

        for (int Index = 0; Index < InContainerA.Num(); ++Index)
        {
            InReturnContainer.Container.Add(InFunc(InContainerA[Index], InContainerB[Index], InContainerC[Index], InContainerD[Index]));
        }
    }

    template <class T_ReturnContainer, class T_TransformFunc, class T_ContainerA, typename T_ContainerB, typename T_ContainerC, typename T_ContainerD>
    auto
        ForEachViewTransform(
            T_ContainerA& InContainerA,
            T_ContainerB& InContainerB,
            T_ContainerC& InContainerC,
            T_ContainerD& InContainerD,
            T_TransformFunc InFunc)
        -> T_ReturnContainer
    {
        auto ToRet = T_ReturnContainer{};
        ForEachViewTransform(InContainerA, InContainerB, InContainerC, InContainerD, ToTransform(ToRet), InFunc);
        return ToRet;
    }

    template <typename T_TransformFunc,	typename T_ContainerA,	typename T_ContainerB,	typename T_ContainerC,	typename T_ContainerD,	typename T_ContainerE,	typename T_ReturnContainer>
    auto
        ForEachViewTransform(
            T_ContainerA& InContainerA,
            T_ContainerB& InContainerB,
            T_ContainerC& InContainerC,
            T_ContainerD& InContainerD,
            T_ContainerE& InContainerE,
            TToTransform<T_ReturnContainer> InReturnContainer,
            T_TransformFunc InFunc)
        -> void
    {
        CK_ENSURE_IF_NOT(
            InContainerA.Num() == InContainerB.Num() == InContainerC.Num() == InContainerD.Num() == InContainerE.Num(),
            TEXT("Non-Matching Container sizes"))
        { return; }

        for (int Index = 0; Index < InContainerA.Num(); ++Index)
        {
            InReturnContainer.Container.Add(InFunc(
                InContainerA[Index], InContainerB[Index], InContainerC[Index], InContainerD[Index], InContainerE[Index]));
        }
    }

    template <class T_ReturnContainer, class T_TransformFunc, class T_ContainerA, typename T_ContainerB, typename T_ContainerC, typename T_ContainerD, typename T_ContainerE>
    auto
        ForEachViewTransform(
            T_ContainerA& InContainerA,
            T_ContainerB& InContainerB,
            T_ContainerC& InContainerC,
            T_ContainerD& InContainerD,
            T_ContainerE& InContainerE,
            T_TransformFunc InFunc)
        -> T_ReturnContainer
    {
        auto ToRet = T_ReturnContainer{};
        ForEachViewTransform(
            InContainerA, InContainerB, InContainerC, InContainerD, InContainerE, ToTransform(ToRet), InFunc);
        return ToRet;
    }
}

// --------------------------------------------------------------------------------------------------------------------
