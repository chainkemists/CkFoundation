#include "CkStats_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Stats/StatsData.h>
#include <Engine/GameViewportClient.h>
#include <Engine/Engine.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Stats_UE::
    Get_FPS()
    -> float
{
    const auto& DiffTime = FApp::GetDeltaTime();
    const auto& FrameTime = DiffTime * 1000.0f;

    CK_ENSURE_IF_NOT(FrameTime > 0.0f, TEXT("The current frame time is invalid"))
    { return {}; }

    const auto& FPS = 1000.0f / FrameTime;
    return FPS;
}

auto
    UCk_Utils_Stats_UE::
    Get_IsStatEnabled(
        const FString& InStatName)
    -> bool
{
    CK_ENSURE_IF_NOT(ck::IsValid(GEngine), TEXT("Invalid GEngine"))
    { return {}; }

    const auto& GameViewport = GEngine->GameViewport;

    CK_ENSURE_IF_NOT(ck::IsValid(GameViewport), TEXT("Invalid GameViewport"))
    { return {}; }

    return GameViewport->IsStatEnabled(InStatName);
}

// --------------------------------------------------------------------------------------------------------------------
