#include "CkEntityScript.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include "CkEcs/EntityScript/CkEntityScript_Fragment.h"

// -----------------------------------------------------------------------------------------------------------

auto
    UCk_EntityScript_UE::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
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

    ck::algo::ForEachIsValid(_TasksToDeactivate, [](const TWeakObjectPtr<UObject>& InTask)
    {
        const auto Function = InTask->FindFunction(TEXT("Deactivate"));
        InTask->ProcessEvent(Function, nullptr);
    });

    DoEndPlay(ScriptEntity);
}

auto
    UCk_EntityScript_UE::
    DoGet_ScriptEntity() const
    -> FCk_Handle
{
    CK_ENSURE_IF_NOT(ck::IsValid(_AssociatedEntity),
        TEXT("ScriptEntity is [{}]. {}\n{}"),
        _AssociatedEntity,
        _AssociatedEntity.Has<ck::FTag_EntityScript_HasEndedPlay>()
        ? TEXT("The Script has ALREADY EndedPlay. Ensure that no lingering Latent Actions are still being performed")
        : TEXT("It's possible that this was not set correctly by the Processor that spawned the entity script"),
        ck::Context(this))
    { return _AssociatedEntity; }

    return _AssociatedEntity;
}

auto
    UCk_EntityScript_UE::
    DoFinishConstruction()
    -> void
{
    if (ck::Is_NOT_Valid(_AssociatedEntity))
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has<ck::FTag_EntityScript_BeginPlay>()),
        TEXT("Called Finish Construction on EntityScript [{}] that has been ASKED to BeginPlay"), _AssociatedEntity)
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has<ck::FTag_EntityScript_HasBegunPlay>()),
        TEXT("Called Finish Construction on EntityScript [{}] that has ALREADY BegunPlay"), _AssociatedEntity)
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has<ck::FTag_EntityScript_ContinueConstruction>()),
        TEXT("Called Finish Construction on EntityScript [{}] that was NOT ONGOING Construction"), _AssociatedEntity)
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has<ck::FTag_EntityScript_FinishConstruction>()),
        TEXT("Called Finish Construction on EntityScript [{}] that has already FINISHED Construction"), _AssociatedEntity)
    { return; }

    CK_ENSURE_IF_NOT(NOT (_AssociatedEntity.Has<ck::FTag_EntityScript_HasEndedPlay>()),
        TEXT("Called Finish Construction on EntityScript [{}] that has ALREADY EndedPlay"), _AssociatedEntity)
    { return; }

    _AssociatedEntity.Add<ck::FTag_EntityScript_FinishConstruction>();
}

auto
    UCk_EntityScript_UE::
    DoRequest_DeactivateTaskOnEndPlay(
        class UObject* InTask)
    -> void
{
    // TODO: we need a better solution for dealing with Task Nodes
    CK_ENSURE_IF_NOT(ck::IsValid(InTask->FindFunction(TEXT("Deactivate")), ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Could NOT find Deactivate on Task [{}]. Are you sure that's a Task Node?"))
    { return; }

    _TasksToDeactivate.Emplace(InTask);
}

// -----------------------------------------------------------------------------------------------------------
