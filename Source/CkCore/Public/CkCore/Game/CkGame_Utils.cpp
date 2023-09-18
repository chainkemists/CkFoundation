#include "CkGame_Utils.h"

#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <Kismet/GameplayStatics.h>

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_Game_UE::
Get_GameStatus(const UObject* InWorldContextObject, bool InEnsureWorldIsValid) -> ECk_GameStatus
{
    if (Get_IsPIE(InWorldContextObject, InEnsureWorldIsValid))
    { return ECk_GameStatus::InPIE; }

    if (Get_IsInGame(InWorldContextObject, InEnsureWorldIsValid))
    { return ECk_GameStatus::InGame; }

    return ECk_GameStatus::NotInGame;
}

auto UCk_Utils_Game_UE::
Get_IsInGame(const UObject* InWorldContextObject, bool InEnsureWorldIsValid) -> bool
{
    if (ck::Is_NOT_Valid(InWorldContextObject, ck::IsValid_Policy_NullptrOnly{}))
    {
        InWorldContextObject = UGameplayStatics::GetGameInstance(nullptr);
    }

    const auto& World = Get_WorldForObject(InWorldContextObject);

    if (ck::Is_NOT_Valid(World, ck::IsValid_Policy_NullptrOnly{}))
    {
        CK_TRIGGER_ENSURE_IF(InEnsureWorldIsValid, TEXT("Invalid world for WorldContextObject"));
        return false;
    }

    if (World->WorldType == EWorldType::Game || World->WorldType == EWorldType::PIE)
    {
        if (ck::IsValid(World->GetGameInstance()))
        { return true; }
    }

    return false;
}

auto UCk_Utils_Game_UE::
Get_IsPIE(const UObject* InWorldContextObject, bool InEnsureWorldIsValid) -> bool
{
#if WITH_EDITOR
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Unable to get the World. InContextObject is INVALID."))
    { return {}; }

    if (ck::Is_NOT_Valid(InWorldContextObject))
    {
        CK_TRIGGER_ENSURE_IF(InEnsureWorldIsValid, TEXT("Invalid world for WorldContextObject"));
        return false;
    }

    const UWorld* World = Get_WorldForObject(InWorldContextObject);

    if (ck::Is_NOT_Valid(World))
    { World = GWorld; }

    if (ck::IsValid(World))
    { return World->WorldType == EWorldType::PIE; }
#endif

    return false;
}

auto UCk_Utils_Game_UE::
Get_WorldForObject(const UObject* InContextObject) -> UWorld*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InContextObject, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Unable to get the World. InContextObject is INVALID."))
    { return {}; }

    auto* World = InContextObject->GetWorld();

    if (ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}))
    { return World; }

    if (const auto& ContextObjectOuter = InContextObject->GetOuter();
        ck::IsValid(ContextObjectOuter, ck::IsValid_Policy_NullptrOnly{}))
    { return ContextObjectOuter->GetWorld(); }

    return {};
}

auto UCk_Utils_Game_UE::
Get_GameInstance(const UObject* InWorldContextObject) -> UGameInstance*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InWorldContextObject, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Unable to get the GameInstance. InContextObject is INVALID."))
    { return {}; }

    if (const auto* World = ck::IsValid(InWorldContextObject)
                                ? Get_WorldForObject(InWorldContextObject)
                                : nullptr;
        ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}))
    {
        return World->GetGameInstance();
    }

    if (ck::Is_NOT_Valid(GEngine, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    for (auto& Context : GEngine->GetWorldContexts())
    {
        const auto& contextWorld = Context.World();

        if (ck::Is_NOT_Valid(contextWorld, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        auto* gameInstance = contextWorld->GetGameInstance();

        if (ck::IsValid(gameInstance))
        { return gameInstance; }
    }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------
