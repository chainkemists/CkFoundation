#include "CkGameMode.h"

#include "CkCore/CkCoreLog.h"
#include "CkCore/Build/CkBuild_Macros.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    ACk_GameMode_UE::
    AllowCheats(
        APlayerController* InPlayerController)
    -> bool
{
#if CK_BUILD_RELEASE
    return Super::AllowCheats(InPlayerController);
#else
    return _AllowMultiplayerCheats ? _AllowMultiplayerCheats : Super::AllowCheats(InPlayerController);
#endif
}

auto
    ACk_GameMode_UE::
    PostActorCreated()
    -> void
{
    TRACE_BOOKMARK(TEXT("Game Mode Created"));
    ck::core::Log(TEXT("Game Mode [{}] Created"), this);
    Super::PostActorCreated();
}

auto
    ACk_GameMode_UE::
    InitializeHUDForPlayer_Implementation(
        APlayerController* NewPlayer)
    -> void
{
    TRACE_BOOKMARK(TEXT("Game Mode Initialize HUD For Player"));
    ck::core::Log(TEXT("Game Mode [{}] Initialize HUD For Player"), this);
    Super::InitializeHUDForPlayer_Implementation(NewPlayer);
}

auto
    ACk_GameMode_UE::
    BeginPlay()
    -> void
{
    TRACE_BOOKMARK(TEXT("Game Mode Begin Play"));
    ck::core::Log(TEXT("Game Mode [{}] Begin Play"), this);
    Super::BeginPlay();
}

// --------------------------------------------------------------------------------------------------------------------
