#include "CkTween_Fragment_Data.h"

#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

// --------------------------------------------------------------------------------------------------------------------

auto FCk_TweenValue::IsFloat() const -> bool
{
    return Is<float>();
}

auto FCk_TweenValue::IsVector() const -> bool
{
    return Is<FVector>();
}

auto FCk_TweenValue::IsRotator() const -> bool
{
    return Is<FRotator>();
}

auto FCk_TweenValue::IsLinearColor() const -> bool
{
    return Is<FLinearColor>();
}

auto FCk_TweenValue::IsTransformHandle() const -> bool
{
    return Is<FCk_Handle_Transform>();
}

// --------------------------------------------------------------------------------------------------------------------

auto FCk_TweenValue::GetAsFloat() const -> float
{
    return Get<float>();
}

auto FCk_TweenValue::GetAsVector() const -> FVector
{
    return Get<FVector>();
}

auto FCk_TweenValue::GetAsRotator() const -> FRotator
{
    return Get<FRotator>();
}

auto FCk_TweenValue::GetAsLinearColor() const -> FLinearColor
{
    return Get<FLinearColor>();
}

auto FCk_TweenValue::GetAsTransformHandle() const -> FCk_Handle_Transform
{
    return Get<FCk_Handle_Transform>();
}

// --------------------------------------------------------------------------------------------------------------------