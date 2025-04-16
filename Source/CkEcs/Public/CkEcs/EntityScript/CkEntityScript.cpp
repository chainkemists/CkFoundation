#include "CkEntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include <BlueprintTaskTemplate.h>

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

    ck::algo::ForEachIsValid(_TasksToDeactivate, [](const TWeakObjectPtr<UBlueprintTaskTemplate>& InTask)
    {
        InTask->Deactivate();
    });

    DoEndPlay(ScriptEntity);
}

auto
    UCk_EntityScript_UE::
    DoGet_ScriptEntity() const
    -> FCk_Handle
{
    return _AssociatedEntity;
}

auto
    UCk_EntityScript_UE::
    DoRequest_AddTaskToDeactivateOnDeactivate(
        class UBlueprintTaskTemplate* InTask)
    -> void
{
    _TasksToDeactivate.Emplace(InTask);
}

// -----------------------------------------------------------------------------------------------------------
