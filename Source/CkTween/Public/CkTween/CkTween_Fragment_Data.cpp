#include "CkTween_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

UE_DEFINE_GAMEPLAY_TAG(TAG_Tween, TEXT("Tween"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Tween_Location, TEXT("Tween.Location"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Tween_Rotation, TEXT("Tween.Rotation"));
UE_DEFINE_GAMEPLAY_TAG(TAG_Tween_Scale, TEXT("Tween.Scale"));

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_TweenValue::
    IsFloat() const
        -> bool
{
    return Is<float>();
}

auto
    FCk_TweenValue::
    IsVector() const
        -> bool
{
    return Is<FVector>();
}

auto
    FCk_TweenValue::
    IsRotator() const
        -> bool
{
    return Is<FRotator>();
}

auto
    FCk_TweenValue::
    IsLinearColor() const
        -> bool
{
    return Is<FLinearColor>();
}

auto
    FCk_TweenValue::
    GetAsFloat() const
        -> float
{
    return Get<float>();
}

FVector
    FCk_TweenValue::
    GetAsVector() const
{
    return Get<FVector>();
}

auto
    FCk_TweenValue::
    GetAsRotator() const
    -> FRotator
{
    return Get<FRotator>();
}

auto
    FCk_TweenValue::
    GetAsLinearColor() const
    -> FLinearColor
{
    return Get<FLinearColor>();
}

// --------------------------------------------------------------------------------------------------------------------
