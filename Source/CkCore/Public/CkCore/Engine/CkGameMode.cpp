#include "CkGameMode.h"

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

// --------------------------------------------------------------------------------------------------------------------
