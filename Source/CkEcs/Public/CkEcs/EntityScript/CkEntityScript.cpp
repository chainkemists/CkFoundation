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
    auto ScriptEntity = DoGet_ScriptEntity();
    DoBeginPlay(ScriptEntity);
}

auto
    UCk_EntityScript_UE::
    EndPlay()
    -> void
{
    auto ScriptEntity = DoGet_ScriptEntity();
    DoEndPlay(ScriptEntity);
}

auto
    UCk_EntityScript_UE::
    DoGet_ScriptEntity() const
    -> FCk_Handle
{
    return _AssociatedEntity;
}

// -----------------------------------------------------------------------------------------------------------