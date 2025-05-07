#include "CkGame_Utils.h"

#include "CkCore/Engine/CkGameInstance.h"
#include "CkCore/Engine/CkPlayerState.h"
#include "CkCore/Validation/CkIsValid.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <Engine/Engine.h>
#include <Kismet/GameplayStatics.h>
#include <Settings/LevelEditorPlaySettings.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Game_UE::
    Get_GameStatus(
        const UObject* InWorldContextObject,
        bool InEnsureWorldIsValid)
    -> ECk_GameStatus
{
    if (Get_IsPIE(InWorldContextObject, InEnsureWorldIsValid))
    { return ECk_GameStatus::InPIE; }

    if (Get_IsInGame(InWorldContextObject, InEnsureWorldIsValid))
    { return ECk_GameStatus::InGame; }

    return ECk_GameStatus::NotInGame;
}

auto
    UCk_Utils_Game_UE::
    Get_IsInGame(
        const UObject* InWorldContextObject,
        bool InEnsureWorldIsValid)
    -> bool
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

auto
    UCk_Utils_Game_UE::
    Get_IsPIE(
        const UObject* InWorldContextObject,
        bool InEnsureWorldIsValid)
    -> bool
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

auto
    UCk_Utils_Game_UE::
    Get_IsPIE_UnderOneProcess(
        const UObject* InWorldContextObject,
        bool InEnsureWorldIsValid)
    -> bool
{
#if WITH_EDITOR
    const auto& IsPIE = Get_IsPIE(InWorldContextObject, InEnsureWorldIsValid);

    if (NOT IsPIE)
    { return {}; }

    const auto& PlayInSettings = GetDefault<ULevelEditorPlaySettings>();

    if (ck::Is_NOT_Valid(PlayInSettings))
    { return {}; }

    auto OutRunUnderOneProcess = false;
    std::ignore = PlayInSettings->GetRunUnderOneProcess(OutRunUnderOneProcess);

    return OutRunUnderOneProcess;
#else
    return false;
#endif
}

auto
    UCk_Utils_Game_UE::
    Get_WorldForObject(
        const UObject* InContextObject)
    -> UWorld*
{
    CK_ENSURE_IF_NOT(ck::IsValid(InContextObject, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Unable to get the World. InContextObject is INVALID."))
    { return {}; }

    if (auto* World = InContextObject->GetWorld();
        ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}))
    { return World; }

    if (const auto& ContextObjectOuter = InContextObject->GetOuter();
        ck::IsValid(ContextObjectOuter, ck::IsValid_Policy_NullptrOnly{}))
    { return ContextObjectOuter->GetWorld(); }

    return {};
}

auto
    UCk_Utils_Game_UE::
    Get_GameInstance(
        const UObject* InWorldContextObject)
    -> UGameInstance*
{
    const auto IsValidContextObject = ck::IsValid(InWorldContextObject);

    if (NOT IsValidContextObject)
    {
        if (auto* GameInstance = UCk_GameInstance_UE::Get_Instance(); ck::IsValid(GameInstance))
        { return GameInstance; }
    }

    CK_ENSURE_IF_NOT(IsValidContextObject,
        TEXT("INVALID InContextObject passed to Get_GameInstance! Attempting to retrieve GameInstance from the GEngine list of WorldContexts."))
    {}

    if (const auto* World = Get_WorldForObject(InWorldContextObject); ck::IsValid(World, ck::IsValid_Policy_NullptrOnly{}))
    {
        return World->GetGameInstance();
    }

    if (ck::Is_NOT_Valid(GEngine, ck::IsValid_Policy_NullptrOnly{}))
    { return {}; }

    for (auto& Context : GEngine->GetWorldContexts())
    {
        const auto& ContextWorld = Context.World();

        if (ck::Is_NOT_Valid(ContextWorld, ck::IsValid_Policy_NullptrOnly{}))
        { continue; }

        if (auto* GameInstance = ContextWorld->GetGameInstance();
            ck::IsValid(GameInstance))
        { return GameInstance; }
    }

    return {};
}

auto
    UCk_Utils_Game_UE::
    Get_PrimaryPlayerState_AsClient(
        const UObject* InWorldContextObject)
    -> APlayerState*
{
    const auto& PrimaryPlayerController = Get_PrimaryPlayerController(InWorldContextObject);

    CK_ENSURE_IF_NOT(ck::IsValid(PrimaryPlayerController), TEXT("Invalid Primary Player Controller!"))
    { return {}; }

    if (PrimaryPlayerController->HasAuthority())
    { return {}; }

    const auto& PlayerState = PrimaryPlayerController->PlayerState;

    return PlayerState;
}

auto
    UCk_Utils_Game_UE::
    Get_PrimaryPlayerController(
        const UObject* InWorldContextObject)
    -> APlayerController*
{
    const auto& GameInstance = Get_GameInstance(InWorldContextObject);

    if (ck::Is_NOT_Valid(GameInstance))
    { return {}; }

    return GameInstance->GetFirstLocalPlayerController();
}

auto
    UCk_Utils_Game_UE::
    FindFloor_WithLineTrace(
        const UObject* InWorldContextObject,
        FVector InStartLocation,
        FHitResult& OutHitResult)
    -> bool
{
    return FindFloor_WithLineTrace(InWorldContextObject, InStartLocation, FCollisionQueryParams::DefaultQueryParam, OutHitResult);
}

auto
    UCk_Utils_Game_UE::
    FindFloor_WithLineTrace(
        const UObject* InWorldContextObject,
        FVector InStartLocation,
        const FCollisionQueryParams& InQueryParams,
        FHitResult& OutHitResult)
    -> bool
{
    const auto& World = Get_WorldForObject(InWorldContextObject);

    if (ck::Is_NOT_Valid(World))
    { return {}; }

    constexpr auto TraceLength = 100000.0f;
    const auto TraceEnd = InStartLocation - (FVector::UpVector * TraceLength);

    auto ResponseParams = FCollisionResponseParams{ECR_Ignore};
    ResponseParams.CollisionResponse.SetResponse(ECC_WorldStatic, ECR_Block);

    return World->LineTraceSingleByChannel(OutHitResult, InStartLocation, TraceEnd, ECC_WorldDynamic, InQueryParams, ResponseParams);
}

// --------------------------------------------------------------------------------------------------------------------
