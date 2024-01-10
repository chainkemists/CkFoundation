#pragma once

#include "CkAlgorithms.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/TypeTraits/CkTypeTraits.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Algo/Count.h>
#include <Algo/Sort.h>

#include <algorithm>

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    template <typename T_Container, typename T_PredicateFunction>
    auto
        AllOf(
            const T_Container& InContainer,
            T_PredicateFunction InFunc) -> bool
    {
        return AllOf(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_ItrType, typename T_PredicateFunction>
    auto
        AllOf(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            T_PredicateFunction InFunc) -> bool
    {
        return std::all_of(InItrBegin, InItrEnd, InFunc);
    }

    template <typename T_Container, typename T_PredicateFunction>
    auto
        AnyOf(
            const T_Container& InContainer,
            T_PredicateFunction InFunc) -> bool
    {
        return AnyOf(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_ItrType, typename T_PredicateFunction>
    auto
        AnyOf(
            T_ItrType InItrBegin,
            T_ItrType InItrEnd,
            T_PredicateFunction InFunc) -> bool
    {
        return std::any_of(InItrBegin, InItrEnd, InFunc);
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
        ForEachRequest(const TArray<T_ValueType>& InContainer, T_UnaryFunction InFunc, policy::DontResetContainer)
        -> void
    {
        ForEach(InContainer.begin(), InContainer.end(), InFunc);
    }

    template <typename T_ValueType, typename T_UnaryFunction>
    auto
        ForEachRequest(const TOptional<T_ValueType>& InContainer, T_UnaryFunction InFunc, policy::DontResetContainer)
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
        auto Index = InArray.Num();
        while (Index-- > 0)
        {
            InFunc(InArray[Index]);
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
            T_Container& InContainer,
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
}

// --------------------------------------------------------------------------------------------------------------------

namespace ck::algo
{
    template <typename T_Func, typename T_ContainerA, typename T_ContainerB>
    auto ForEachView(T_ContainerA& InContainerA, T_ContainerB& InContainerB, T_Func InFunc) -> void
    {
        CK_ENSURE_IF_NOT(InContainerA.Num() == InContainerB.Num(), TEXT("Non-Matching Container sizes"))
        { return; }

        for (int Index = 0; Index < InContainerA.Num(); ++Index)
        {
            InFunc(InContainerA[Index], InContainerB[Index]);
        }
    }

    template <typename T_Func, typename T_ContainerA, typename T_ContainerB, typename T_ContainerC>
    auto ForEachView(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_Func InFunc) -> void
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
    auto ForEachView(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_ContainerD& InContainerD,
        T_Func InFunc) -> void
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
    auto ForEachView(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_ContainerD& InContainerD,
        T_ContainerE& InContainerE,
        T_Func InFunc) -> void
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
    auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        TToTransform<T_ReturnContainer> InReturnContainer,
        T_TransformFunc InFunc) -> void
    {
        CK_ENSURE_IF_NOT(InContainerA.Num() == InContainerB.Num(), TEXT("Non-Matching Container sizes"))
        { return; }

        for (auto Index = 0; Index < InContainerA.Num(); ++Index)
        {
            InReturnContainer.Container.Add(InFunc(InContainerA[Index], InContainerB[Index]));
        }
    }

    template <class T_ReturnContainer, class T_TransformFunc, class T_ContainerA, typename T_ContainerB>
    auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_TransformFunc InFunc) -> T_ReturnContainer
    {
        auto ToRet = T_ReturnContainer{};
        ForEachViewTransform(InContainerA, InContainerB, ToTransform(ToRet), InFunc);
        return ToRet;
    }

    template <typename T_TransformFunc,	typename T_ContainerA,	typename T_ContainerB,	typename T_ContainerC,	typename T_ReturnContainer>
    auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        TToTransform<T_ReturnContainer> InReturnContainer,
        T_TransformFunc InFunc) -> void
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
    auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_TransformFunc InFunc) -> T_ReturnContainer
    {
        auto ToRet = T_ReturnContainer{};
        ForEachViewTransform(InContainerA, InContainerB, InContainerC, ToTransform(ToRet), InFunc);
        return ToRet;
    }

    template <typename T_TransformFunc,	typename T_ContainerA,	typename T_ContainerB,	typename T_ContainerC,	typename T_ContainerD,	typename T_ReturnContainer>
    auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_ContainerD& InContainerD,
        TToTransform<T_ReturnContainer> InReturnContainer,
        T_TransformFunc InFunc) -> void
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
    auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_ContainerD& InContainerD,
        T_TransformFunc InFunc) -> T_ReturnContainer
    {
        auto ToRet = T_ReturnContainer{};
        ForEachViewTransform(InContainerA, InContainerB, InContainerC, InContainerD, ToTransform(ToRet), InFunc);
        return ToRet;
    }

    template <typename T_TransformFunc,	typename T_ContainerA,	typename T_ContainerB,	typename T_ContainerC,	typename T_ContainerD,	typename T_ContainerE,	typename T_ReturnContainer>
    auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_ContainerD& InContainerD,
        T_ContainerE& InContainerE,
        TToTransform<T_ReturnContainer> InReturnContainer,
        T_TransformFunc InFunc) -> void
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
    auto ForEachViewTransform(
        T_ContainerA& InContainerA,
        T_ContainerB& InContainerB,
        T_ContainerC& InContainerC,
        T_ContainerD& InContainerD,
        T_ContainerE& InContainerE,
        T_TransformFunc InFunc) -> T_ReturnContainer
    {
        auto ToRet = T_ReturnContainer{};
        ForEachViewTransform(
            InContainerA, InContainerB, InContainerC, InContainerD, InContainerE, ToTransform(ToRet), InFunc);
        return ToRet;
    }
}

// --------------------------------------------------------------------------------------------------------------------
