#include "CkSharedValues_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SharedBool_UE::
    Get(
        const FCk_SharedBool& InShared)
    -> bool
{
    return *InShared;
}

auto
    UCk_Utils_SharedBool_UE::
    Set(
        FCk_SharedBool& InShared,
        bool InValue)
    -> void
{
    *InShared = InValue;
}

auto
    UCk_Utils_SharedBool_UE::
    Make(
        bool InValue)
    -> FCk_SharedBool
{
    return FCk_SharedBool{InValue};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SharedInt_UE::
    Get(
        const FCk_SharedInt& InShared)
    -> int32
{
    return *InShared;
}

auto
    UCk_Utils_SharedInt_UE::
    Set(
        FCk_SharedInt& InShared,
        int32 InValue)
    -> void
{
    *InShared = InValue;
}

auto
    UCk_Utils_SharedInt_UE::
    Make(
        int32 InValue)
    -> FCk_SharedInt
{
    return FCk_SharedInt{InValue};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SharedFloat_UE::
    Get(
        const FCk_SharedFloat& InShared)
    -> float
{
    return *InShared;
}

auto
    UCk_Utils_SharedFloat_UE::
    Set(
        FCk_SharedFloat& InShared,
        float InValue)
    -> void
{
    *InShared = InValue;
}


auto
    UCk_Utils_SharedFloat_UE::
    Make(
        float InValue)
    -> FCk_SharedFloat
{
    return FCk_SharedFloat{InValue};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SharedVector_UE::
    Get(
        const FCk_SharedVector& InShared)
    -> FVector
{
    return *InShared;
}

auto
    UCk_Utils_SharedVector_UE::
    Set(
        FCk_SharedVector& InShared,
        FVector InValue)
    -> void
{
    *InShared = InValue;
}

auto
    UCk_Utils_SharedVector_UE::
    Make(
        FVector InValue)
    -> FCk_SharedVector
{
    return FCk_SharedVector{InValue};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SharedString_UE::
    Get(
        const FCk_SharedString& InShared)
    -> FString
{
    return *InShared;
}

auto
    UCk_Utils_SharedString_UE::
    Set(
        FCk_SharedString& InShared,
        FString InValue)
    -> void
{
    *InShared = InValue;
}

auto
    UCk_Utils_SharedString_UE::
    Make(
        FString InValue)
    -> FCk_SharedString
{
    return FCk_SharedString{InValue};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SharedRotator_UE::
    Get(
        const FCk_SharedRotator& InShared)
    -> FRotator
{
    return *InShared;
}

auto
    UCk_Utils_SharedRotator_UE::
    Set(
        FCk_SharedRotator& InShared,
        FRotator InValue)
    -> void
{
    *InShared = InValue;
}

auto
    UCk_Utils_SharedRotator_UE::
    Make(
        FRotator InValue)
    -> FCk_SharedRotator
{
    return FCk_SharedRotator{InValue};
}

// --------------------------------------------------------------------------------------------------------------------