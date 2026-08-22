#pragma once

#include "CkAlgorithms.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Algo/Count.h>
#include <Algo/IsSorted.h>
#include <Algo/Sort.h>
#include <Algo/Find.h>
#include <Algo/RemoveIf.h>
#include <Algo/MaxElement.h>
#include <Algo/MinElement.h>

#include <algorithm>
#include <functional>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    // Output iterator adapter for TArray, replaces std::back_inserter(std::vector) pattern
    template <typename T_ValueType>
    struct TArrayBackInserter
    {
        using iterator_category = std::output_iterator_tag;
        using value_type        = void;
        using difference_type   = void;
        using pointer           = void;
        using reference         = void;

        explicit TArrayBackInserter(TArray<T_ValueType>& InArray) : _Array(&InArray) {}

        auto operator=(const T_ValueType& InValue) -> TArrayBackInserter& { _Array->Add(InValue); return *this; }
        auto operator=(T_ValueType&& InValue)      -> TArrayBackInserter& { _Array->Add(MoveTemp(InValue)); return *this; }
        auto operator*()                           -> TArrayBackInserter& { return *this; }
        auto operator++()                          -> TArrayBackInserter& { return *this; }
        auto operator++(int)                       -> TArrayBackInserter  { return *this; }

    private:
        TArray<T_ValueType>* _Array;
    };
}

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

    template <typename T_ContainerA, typename T_ContainerB>
    auto
        Compare(
            const T_ContainerA& InContainerA,
            const T_ContainerB& InContainerB)
        -> bool
    {
        return Algo::Compare(InContainerA, InContainerB);
    }

    template <typename T_ContainerA, typename T_ContainerB, typename T_PredicateFunction>
    auto
        Compare(
            const T_ContainerA& InContainerA,
            const T_ContainerB& InContainerB,
            T_PredicateFunction InFunc)
        -> bool
    {
        return Algo::Compare(InContainerA, InContainerB, InFunc);
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
        FilterInPlace(
            T_Container& InContainer,
            T_PredicateFunction InFunc)
        -> void
    {
        InContainer.RemoveAll([&InFunc](const auto& InElement) { return !InFunc(InElement); });
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

    // ---- Intersect ----

    template <typename T_ValueType>
    auto
        Intersect(
            const TArray<T_ValueType>& InContainerA,
            const TArray<T_ValueType>& InContainerB)
        -> TArray<T_ValueType>
    {
        const auto SortedContainerA = ck::algo::Sort(InContainerA);
        const auto SortedContainerB = ck::algo::Sort(InContainerB);

        auto Result = TArray<T_ValueType>{};
        Result.Reserve(FMath::Min(InContainerA.Num(), InContainerB.Num()));

        std::set_intersection
        (
            SortedContainerA.begin(),
            SortedContainerA.end(),
            SortedContainerB.begin(),
            SortedContainerB.end(),
            TArrayBackInserter(Result)
        );

        return Result;
    }

    template <typename T_ValueType, typename T_ProjectionFunc>
    auto
        Intersect(
            const TArray<T_ValueType>& InContainerA,
            const TArray<T_ValueType>& InContainerB,
            T_ProjectionFunc InProjection)
        -> TArray<T_ValueType>
    {
        const auto Comparator = [&InProjection](const T_ValueType& A, const T_ValueType& B)
        { return std::invoke(InProjection, A) < std::invoke(InProjection, B); };

        const auto SortedContainerA = ck::algo::Sort(InContainerA, Comparator);
        const auto SortedContainerB = ck::algo::Sort(InContainerB, Comparator);

        auto Result = TArray<T_ValueType>{};
        Result.Reserve(FMath::Min(InContainerA.Num(), InContainerB.Num()));

        std::set_intersection
        (
            SortedContainerA.begin(),
            SortedContainerA.end(),
            SortedContainerB.begin(),
            SortedContainerB.end(),
            TArrayBackInserter(Result),
            Comparator
        );

        return Result;
    }

    // ---- SymmetricDifference ----

    template <typename T_ValueType>
    auto
    SymmetricDifference(
        const TArray<T_ValueType>& InContainerA,
        const TArray<T_ValueType>& InContainerB)
    -> TArray<T_ValueType>
    {
        constexpr int32 Threshold = 16; // For small arrays, avoid TSet overhead

        TArray<T_ValueType> Result;

        if (InContainerA.Num() < Threshold && InContainerB.Num() < Threshold)
        {
            for (const auto& Item : InContainerA)
            {
                if (!InContainerB.Contains(Item))
                {
                    Result.Add(Item);
                }
            }

            for (const auto& Item : InContainerB)
            {
                if (!InContainerA.Contains(Item))
                {
                    Result.Add(Item);
                }
            }
        }
        else
        {
            TSet<T_ValueType> SetA(InContainerA);
            TSet<T_ValueType> SetB(InContainerB);

            for (const auto& Item : SetA)
            {
                if (!SetB.Contains(Item))
                {
                    Result.Add(Item);
                }
            }

            for (const auto& Item : SetB)
            {
                if (!SetA.Contains(Item))
                {
                    Result.Add(Item);
                }
            }
        }

        return Result;
    }

    // ---- Except ----

    template <typename T_ValueType>
    auto
        Except(
            const TArray<T_ValueType>& InContainerA,
            const TArray<T_ValueType>& InContainerB)
        -> TArray<T_ValueType>
    {
        const auto SortedContainerA = ck::algo::Sort(InContainerA);
        const auto SortedContainerB = ck::algo::Sort(InContainerB);

        auto Result = TArray<T_ValueType>{};
        Result.Reserve(InContainerA.Num());

        std::set_difference
        (
            SortedContainerA.begin(),
            SortedContainerA.end(),
            SortedContainerB.begin(),
            SortedContainerB.end(),
            TArrayBackInserter(Result)
        );

        return Result;
    }

    template <typename T_ValueType, typename T_ProjectionFunc>
    auto
        Except(
            const TArray<T_ValueType>& InContainerA,
            const TArray<T_ValueType>& InContainerB,
            T_ProjectionFunc InProjection)
        -> TArray<T_ValueType>
    {
        const auto Comparator = [&InProjection](const T_ValueType& A, const T_ValueType& B)
        { return std::invoke(InProjection, A) < std::invoke(InProjection, B); };

        const auto SortedContainerA = ck::algo::Sort(InContainerA, Comparator);
        const auto SortedContainerB = ck::algo::Sort(InContainerB, Comparator);

        auto Result = TArray<T_ValueType>{};
        Result.Reserve(InContainerA.Num());

        std::set_difference
        (
            SortedContainerA.begin(),
            SortedContainerA.end(),
            SortedContainerB.begin(),
            SortedContainerB.end(),
            TArrayBackInserter(Result),
            Comparator
        );

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

namespace ck::algo
{
    template <typename T_Container>
    auto
        Mean(
            const T_Container& InContainer)
        -> TOptional<double>
    {
        if (InContainer.Num() == 0)
        { return {}; }

        auto Sum = 0.0;

        for (const auto& Value : InContainer)
        {
            Sum += static_cast<double>(Value);
        }

        return Sum / static_cast<double>(InContainer.Num());
    }

    template <typename T_Container>
    auto
        Percentile(
            const T_Container& InContainer,
            double InPercentile)
        -> TOptional<double>
    {
        if (InContainer.Num() == 0)
        { return {}; }

        auto Sorted = TArray<double>{};
        Sorted.Reserve(InContainer.Num());

        for (const auto& Value : InContainer)
        {
            Sorted.Add(static_cast<double>(Value));
        }

        Algo::Sort(Sorted);

        // Linear interpolation between the two ranks straddling the requested fraction. With a
        // single sample both ranks collapse onto it, which is why no separate case is needed.
        const auto Clamped   = FMath::Clamp(InPercentile, 0.0, 1.0);
        const auto Rank      = Clamped * static_cast<double>(Sorted.Num() - 1);
        const auto LowerRank = FMath::FloorToInt32(Rank);
        const auto UpperRank = FMath::Min(LowerRank + 1, Sorted.Num() - 1);
        const auto Fraction  = Rank - static_cast<double>(LowerRank);

        return FMath::Lerp(Sorted[LowerRank], Sorted[UpperRank], Fraction);
    }

    template <typename T_Container>
    auto
        Median(
            const T_Container& InContainer)
        -> TOptional<double>
    {
        return Percentile(InContainer, 0.5);
    }

    template <typename T_Container>
    auto
        MedianAbsoluteDeviation(
            const T_Container& InContainer)
        -> TOptional<double>
    {
        const auto MedianValue = Median(InContainer);

        if (NOT MedianValue.IsSet())
        { return {}; }

        auto Deviations = TArray<double>{};
        Deviations.Reserve(InContainer.Num());

        for (const auto& Value : InContainer)
        {
            Deviations.Add(FMath::Abs(static_cast<double>(Value) - *MedianValue));
        }

        return Median(Deviations);
    }

    template <typename T_Container>
    auto
        MeanAbsoluteDeviation(
            const T_Container& InContainer)
        -> TOptional<double>
    {
        const auto MedianValue = Median(InContainer);

        if (NOT MedianValue.IsSet())
        { return {}; }

        auto Deviations = TArray<double>{};
        Deviations.Reserve(InContainer.Num());

        for (const auto& Value : InContainer)
        {
            Deviations.Add(FMath::Abs(static_cast<double>(Value) - *MedianValue));
        }

        return Mean(Deviations);
    }

    template <typename T_Container>
    auto
        Variance(
            const T_Container& InContainer)
        -> TOptional<double>
    {
        const auto MeanValue = Mean(InContainer);

        if (NOT MeanValue.IsSet())
        { return {}; }

        auto SquaredDeviations = TArray<double>{};
        SquaredDeviations.Reserve(InContainer.Num());

        for (const auto& Value : InContainer)
        {
            const auto Deviation = static_cast<double>(Value) - *MeanValue;
            SquaredDeviations.Add(Deviation * Deviation);
        }

        return Mean(SquaredDeviations);
    }

    template <typename T_Container>
    auto
        StandardDeviation(
            const T_Container& InContainer)
        -> TOptional<double>
    {
        const auto VarianceValue = Variance(InContainer);

        if (NOT VarianceValue.IsSet())
        { return {}; }

        return FMath::Sqrt(*VarianceValue);
    }

    template <typename T_Container>
    auto
        CoefficientOfVariation(
            const T_Container& InContainer)
        -> TOptional<double>
    {
        const auto MeanValue = Mean(InContainer);

        if (NOT MeanValue.IsSet() || FMath::IsNearlyZero(*MeanValue))
        { return {}; }

        const auto StandardDeviationValue = StandardDeviation(InContainer);

        if (NOT StandardDeviationValue.IsSet())
        { return {}; }

        return *StandardDeviationValue / *MeanValue;
    }

    template <typename T_Container, typename T_ProjectionFunction>
    auto
        SumBy(
            const T_Container& InContainer,
            T_ProjectionFunction InProjection)
        -> double
    {
        auto Sum = 0.0;

        for (const auto& Element : InContainer)
        {
            Sum += static_cast<double>(std::invoke(InProjection, Element));
        }

        return Sum;
    }

    template <typename T_Container>
    auto
        HarmonicMean(
            const T_Container& InContainer)
        -> TOptional<double>
    {
        if (InContainer.Num() == 0)
        { return {}; }

        // The reciprocal of zero is undefined, and a set containing "no headroom at all" harmonises
        // to exactly that. Answering zero is the limit of the function rather than a special case.
        if (AnyOf(InContainer, [](const auto& InValue) { return static_cast<double>(InValue) <= 0.0; }))
        { return 0.0; }

        const auto SumOfReciprocals = SumBy(InContainer, [](const auto& InValue)
        { return 1.0 / static_cast<double>(InValue); });

        return static_cast<double>(InContainer.Num()) / SumOfReciprocals;
    }
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    template <typename T_Container, typename T_PredicateFunction>
    auto
        IsSorted(
            const T_Container& InContainer,
            T_PredicateFunction InFunc)
        -> bool
    {
        return Algo::IsSorted(InContainer, InFunc);
    }

    template <typename T_Container>
    auto
        IsSorted(
            const T_Container& InContainer)
        -> bool
    {
        return Algo::IsSorted(InContainer);
    }

    template <typename T_Container>
    auto
        TakeLast(
            const T_Container& InContainer,
            int32 InCount)
        -> T_Container
    {
        const auto Count = FMath::Clamp(InCount, 0, InContainer.Num());

        return T_Container{InContainer.GetData() + (InContainer.Num() - Count), Count};
    }

    template <typename T_Container>
    auto
        TakeFirst(
            const T_Container& InContainer,
            int32 InCount)
        -> T_Container
    {
        return T_Container{InContainer.GetData(), FMath::Clamp(InCount, 0, InContainer.Num())};
    }

    template <typename T_Container, typename T_ProjectionFunction>
    auto
        CountBy(
            const T_Container& InContainer,
            T_ProjectionFunction InProjection)
        -> TMap<std::invoke_result_t<T_ProjectionFunction, typename T_Container::ElementType>, int32>
    {
        auto Counts = TMap<std::invoke_result_t<T_ProjectionFunction, typename T_Container::ElementType>, int32>{};

        for (const auto& Element : InContainer)
        {
            Counts.FindOrAdd(std::invoke(InProjection, Element)) += 1;
        }

        return Counts;
    }

    template <typename T_Container, typename T_ProjectionFunction>
    auto
        IndexBy(
            const T_Container& InContainer,
            T_ProjectionFunction InKeyProjection)
        -> TMap<std::invoke_result_t<T_ProjectionFunction, typename T_Container::ElementType>,
                typename T_Container::ElementType>
    {
        auto Index = TMap<std::invoke_result_t<T_ProjectionFunction, typename T_Container::ElementType>,
                          typename T_Container::ElementType>{};

        Index.Reserve(InContainer.Num());

        for (const auto& Element : InContainer)
        {
            Index.Add(std::invoke(InKeyProjection, Element), Element);
        }

        return Index;
    }

    template <class T_ReturnContainer, class T_Container, class T_PredicateFunction, class T_TransformFunc>
    auto
        TransformIf(
            const T_Container& InContainer,
            T_PredicateFunction InPredicate,
            T_TransformFunc InFunc)
        -> T_ReturnContainer
    {
        auto ToRet = T_ReturnContainer{};

        for (const auto& Element : InContainer)
        {
            if (NOT std::invoke(InPredicate, Element))
            { continue; }

            ToRet.Add(std::invoke(InFunc, Element));
        }

        return ToRet;
    }
}

// --------------------------------------------------------------------------------------------------------------------
