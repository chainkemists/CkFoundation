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
    UCk_Utils_SharedVec3_UE::
    Get(
        const FCk_SharedVec3& InShared)
    -> FVector
{
    return *InShared;
}

auto
    UCk_Utils_SharedVec3_UE::
    Set(
        FCk_SharedVec3& InShared,
        FVector InValue)
    -> void
{
    *InShared = InValue;
}

auto
    UCk_Utils_SharedVec3_UE::
    Make(
        FVector InValue)
    -> FCk_SharedVec3
{
    return FCk_SharedVec3{InValue};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SharedVec2_UE::
    Get(
        const FCk_SharedVec2& InShared)
    -> FVector2D
{
    return *InShared;
}

auto
    UCk_Utils_SharedVec2_UE::
    Set(
        FCk_SharedVec2& InShared,
        FVector2D InValue)
    -> void
{
    *InShared = InValue;
}

auto
    UCk_Utils_SharedVec2_UE::
    Make(
        FVector2D InValue)
    -> FCk_SharedVec2
{
    return FCk_SharedVec2{InValue};
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
    UCk_Utils_SharedName_UE::
    Get(
        const FCk_SharedName& InShared)
    -> FName
{
    return *InShared;
}

auto
    UCk_Utils_SharedName_UE::
    Set(
        FCk_SharedName& InShared,
        FName InValue)
    -> void
{
    *InShared = InValue;
}

auto
    UCk_Utils_SharedName_UE::
    Make(
        FName InValue)
    -> FCk_SharedName
{
    return FCk_SharedName{InValue};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SharedText_UE::
    Get(
        const FCk_SharedText& InShared)
    -> FText
{
    return *InShared;
}

auto
    UCk_Utils_SharedText_UE::
    Set(
        FCk_SharedText& InShared,
        FText InValue)
    -> void
{
    *InShared = InValue;
}

auto
    UCk_Utils_SharedText_UE::
    Make(
        FText InValue)
    -> FCk_SharedText
{
    return FCk_SharedText{InValue};
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

auto
    UCk_Utils_SharedTransform_UE::
    Get(
        const FCk_SharedTransform& InShared)
    -> FTransform
{
    return *InShared;
}

auto
    UCk_Utils_SharedTransform_UE::
    Set(
        FCk_SharedTransform& InShared,
        FTransform InValue)
    -> void
{
    *InShared = InValue;
}

auto
    UCk_Utils_SharedTransform_UE::
    Make(
        FTransform InValue)
    -> FCk_SharedTransform
{
    return FCk_SharedTransform{InValue};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_SharedInstancedStruct_UE::
    Get(
        const FCk_SharedInstancedStruct& InShared)
    -> FInstancedStruct
{
    return *InShared;
}

auto
    UCk_Utils_SharedInstancedStruct_UE::
    Set(
        FCk_SharedInstancedStruct& InShared,
        const FInstancedStruct& InValue)
    -> void
{
    *InShared = InValue;
}

auto
    UCk_Utils_SharedInstancedStruct_UE::
    Make(
        const FInstancedStruct& InValue)
    -> FCk_SharedInstancedStruct
{
    return FCk_SharedInstancedStruct{InValue};
}

// --------------------------------------------------------------------------------------------------------------------