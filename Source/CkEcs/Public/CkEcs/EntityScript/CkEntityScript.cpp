#include "CkEntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

#include <BlueprintTaskTemplate.h>

// -----------------------------------------------------------------------------------------------------------

auto
    UCk_EntityScript_UE::
    Construct(
        FCk_Handle& InHandle)
    -> ECk_EntityScript_ConstructionFlow
{
    return DoConstruct(InHandle);
}

auto
    UCk_EntityScript_UE::
    ContinueConstruction(
        FCk_Handle InHandle)
    -> void
{
    DoContinueConstruction(InHandle);
}

auto
    UCk_EntityScript_UE::
    BeginPlay()
    -> void
{
    const auto ScriptEntity = DoGet_ScriptEntity();
    DoBeginPlay(ScriptEntity);
}

auto
    UCk_EntityScript_UE::
    EndPlay()
    -> void
{
    const auto ScriptEntity = DoGet_ScriptEntity();

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
    DoFinishConstruction()
    -> void
{
    if (ck::Is_NOT_Valid(_AssociatedEntity))
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has_Any<ck::FTag_EntityScript_BeginPlay>()),
        TEXT("Called Finish Construction on EntityScript [{}] that has been ASKED to BeginPlay"), _AssociatedEntity)
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has_Any<ck::FTag_EntityScript_HasBegunPlay>()),
        TEXT("Called Finish Construction on EntityScript [{}] that has ALREADY BegunPlay"), _AssociatedEntity)
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has_Any<ck::FTag_EntityScript_ContinueConstruction>()),
        TEXT("Called Finish Construction on EntityScript [{}] that was NOT ONGOING Construction"), _AssociatedEntity)
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has_Any<ck::FTag_EntityScript_FinishConstruction>()),
        TEXT("Called Finish Construction on EntityScript [{}] that has already FINISHED Construction"), _AssociatedEntity)
    { return; }

    _AssociatedEntity.Add<ck::FTag_EntityScript_FinishConstruction>();
}

auto
    UCk_EntityScript_UE::
    DoRequest_DeactivateTaskOnEndPlay(
        class UBlueprintTaskTemplate* InTask)
    -> void
{
    _TasksToDeactivate.Emplace(InTask);
}

// -----------------------------------------------------------------------------------------------------------
