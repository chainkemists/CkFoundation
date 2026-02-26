// Copyright Saad Taame. All Rights Reserved.

#include "CkStateTree/CkStateTree_NativeCondition.h"

#include "CkCore/Macros/CkMacros.h"

#include "StateTreeExecutionContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CkStateTree_NativeCondition)

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_StateTree_NativeCondition::
    TestCondition(
        FStateTreeExecutionContext& Context) const
    -> bool
{
    auto Result = EvaluateCondition(Context);

    if (Invert)
    {
        Result = NOT Result;
    }

    return Result;
}

auto
    FCk_StateTree_NativeCondition::
        EvaluateCondition(
        FStateTreeExecutionContext& Context) const
    -> bool
{
    return false;
}

// --------------------------------------------------------------------------------------------------------------------
