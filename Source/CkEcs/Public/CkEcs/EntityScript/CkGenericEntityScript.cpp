#include "CkGenericEntityScript.h"

// -----------------------------------------------------------------------------------------------------------

auto
    UCk_GenericEntityScript_UE::
    Construct(
        FCk_Handle& InHandle,
        const FInstancedStruct& InSpawnParams)
    -> ECk_EntityScript_ConstructionFlow
{
    return DoConstruct(InHandle);
}

auto
    UCk_GenericEntityScript_UE::
    ContinueConstruction(
        FCk_Handle InHandle)
    -> void
{
    DoContinueConstruction(InHandle);
}

auto
    UCk_GenericEntityScript_UE::
    BeginPlay()
    -> void
{
    const auto ScriptEntity = DoGet_ScriptEntity();
    DoBeginPlay(ScriptEntity);
}

auto
    UCk_GenericEntityScript_UE::
    EndPlay()
    -> void
{
    const auto ScriptEntity = DoGet_ScriptEntity();
    DoEndPlay(ScriptEntity);
}

// -----------------------------------------------------------------------------------------------------------
