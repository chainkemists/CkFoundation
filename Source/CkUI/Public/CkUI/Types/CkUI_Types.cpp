// Copyright 2025 CkFoundation. All Rights Reserved.

#include "CkUI/Types/CkUI_Types.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkUI/Subsystem/CkUI_Subsystem.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_Handle_InputSuspension::
    Create(
        uint32 InId,
        FName InReason,
        FName InToken,
        const ULocalPlayer* InLocalPlayer)
    -> FCk_Handle_InputSuspension
{
    FCk_Handle_InputSuspension Handle;
    Handle._Id = InId;
    Handle._Reason = InReason;
    Handle._Token = InToken;
    Handle._LocalPlayer = InLocalPlayer;
    return Handle;
}

auto
    FCk_Handle_InputSuspension::
    IsValid() const
    -> bool
{
    return _Id != 0
        && _Token != NAME_None
        && _LocalPlayer.IsValid();
}

auto
    FCk_Handle_InputSuspension::
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
    FCk_Handle_InputSuspension::
    Get_LocalPlayer() const
    -> const ULocalPlayer*
{
    return _LocalPlayer.Get();
}

auto
    FCk_Handle_InputSuspension::
    DoMarkInvalid()
    -> void
{
    _Id = 0;
    _Token = NAME_None;
    _LocalPlayer.Reset();
}

auto
    FCk_Handle_InputSuspension::
    operator==(const ThisType& Other) const
    -> bool
{
    return _Id == Other._Id && _Token == Other._Token;
}

// --------------------------------------------------------------------------------------------------------------------
