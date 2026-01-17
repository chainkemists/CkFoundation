// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Types/CkUI_Types.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkUI/Subsystem/CkUI_Subsystem.h"

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

auto
    FCk_UI_InputSuspensionToken::
    Create(
        uint32 InId,
        FName InReason,
        FName InToken,
        const ULocalPlayer* InLocalPlayer)
    -> FCk_UI_InputSuspensionToken
{
    FCk_UI_InputSuspensionToken Handle;
    Handle._Id = InId;
    Handle._Reason = InReason;
    Handle._Token = InToken;
    Handle._LocalPlayer = InLocalPlayer;
    return Handle;
}

auto
    FCk_UI_InputSuspensionToken::
    IsValid() const
    -> bool
{
    return _Id != 0
        && _Token != NAME_None
        && _LocalPlayer.IsValid();
}

auto
    FCk_UI_InputSuspensionToken::
    Resume()
    -> void
{
    if (NOT IsValid())
    {
        return;
    }

    auto* LocalPlayer = const_cast<ULocalPlayer*>(_LocalPlayer.Get());

    if (ck::Is_NOT_Valid(LocalPlayer))
    {
        return;
    }

    auto* Subsystem = LocalPlayer->GetSubsystem<UCk_UI_Subsystem_UE>();

    if (ck::Is_NOT_Valid(Subsystem))
    {
        return;
    }

    Subsystem->ResumeInput(*this);
}

auto
    FCk_UI_InputSuspensionToken::
    Get_LocalPlayer() const
    -> const ULocalPlayer*
{
    return _LocalPlayer.Get();
}

auto
    FCk_UI_InputSuspensionToken::
    DoMarkInvalid()
    -> void
{
    _Id = 0;
    _Token = NAME_None;
    _LocalPlayer.Reset();
}

auto
    FCk_UI_InputSuspensionToken::
    operator==(const ThisType& Other) const
    -> bool
{
    return _Id == Other._Id && _Token == Other._Token;
}

// --------------------------------------------------------------------------------------------------------------------
