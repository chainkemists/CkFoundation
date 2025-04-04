#include "CkEntityScript.h"

// -----------------------------------------------------------------------------------------------------------

auto
    UCk_EntityScript_UE::
    Construct(
        FCk_Handle& InHandle) const
    -> void
{
    DoConstruct(InHandle);
}

auto
    UCk_EntityScript_UE::
    BeginPlay()
    -> void
{
    DoBeginPlay();
}

auto
    UCk_EntityScript_UE::
    EndPlay()
    -> void
{
    DoEndPlay();
}

auto
    UCk_EntityScript_UE::
    DoGet_ScriptEntity() const
    -> FCk_Handle
{
    return _AssociatedEntity;
}

// -----------------------------------------------------------------------------------------------------------