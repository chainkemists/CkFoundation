// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Types/CkUI_Types.h"

#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------
// FCk_UI_Context
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_UI_Context::
    MakeFromEntity(
        FCk_Handle InEntity)
    -> FCk_UI_Context
{
    FCk_UI_Context Context;
    Context._Entity = InEntity;
    return Context;
}

auto
    FCk_UI_Context::
    MakeFromActor(
        AActor* InActor)
    -> FCk_UI_Context
{
    FCk_UI_Context Context;
    Context._Actor = InActor;
    return Context;
}

auto
    FCk_UI_Context::
    MakeFromObject(
        UObject* InObject)
    -> FCk_UI_Context
{
    FCk_UI_Context Context;
    Context._Payload = InObject;
    return Context;
}

auto
    FCk_UI_Context::
    MakeFromEntityAndActor(
        FCk_Handle InEntity,
        AActor* InActor)
    -> FCk_UI_Context
{
    FCk_UI_Context Context;
    Context._Entity = InEntity;
    Context._Actor = InActor;
    return Context;
}

auto
    FCk_UI_Context::
    IsValid() const
    -> bool
{
    return HasEntity() || HasActor() || HasPayload();
}

auto
    FCk_UI_Context::
    HasEntity() const
    -> bool
{
    return ck::IsValid(_Entity);
}

auto
    FCk_UI_Context::
    HasActor() const
    -> bool
{
    return _Actor.IsValid();
}

auto
    FCk_UI_Context::
    HasPayload() const
    -> bool
{
    return _Payload.IsValid();
}

auto
    FCk_UI_Context::
    Reset()
    -> void
{
    _Entity = FCk_Handle{};
    _Actor.Reset();
    _Payload.Reset();
}

auto
    FCk_UI_Context::
    operator==(
        const ThisType& Other) const
    -> bool
{
    return _Entity == Other._Entity
        && _Actor == Other._Actor
        && _Payload == Other._Payload;
}

// --------------------------------------------------------------------------------------------------------------------