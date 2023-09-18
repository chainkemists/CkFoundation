#include "CkComparison.h"

// --------------------------------------------------------------------------------------------------------------------
auto
    FCk_Comparison_Float::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_RHS()       == InOther.Get_RHS() &&
           Get_Tolerance() == InOther.Get_Tolerance() &&
           Get_Operator()  == InOther.Get_Operator();
}

auto
    GetTypeHash(
        const FCk_Comparison_Float& InA)
    -> uint32
{
    return GetTypeHash(InA.Get_Operator()) + GetTypeHash(InA.Get_RHS()) + GetTypeHash(InA.Get_Tolerance());
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Comparison_Int::
    operator==(
        const ThisType& InOther) const
    -> bool
{
    return Get_RHS()      == InOther.Get_RHS() &&
           Get_Operator() == InOther.Get_Operator();
}

auto
    GetTypeHash(
        const FCk_Comparison_Int& InA)
    -> uint32
{
    return GetTypeHash(InA.Get_Operator()) + GetTypeHash(InA.Get_RHS());
}

// --------------------------------------------------------------------------------------------------------------------
