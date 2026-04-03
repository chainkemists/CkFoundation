#include "CkActorRelay_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_ActorRelay_Settings_UE::
    Get_PendingChannelTimeoutSeconds()
    -> float
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_ActorRelay_ProjectSettings_UE>();

    if (ck::Is_NOT_Valid(Settings))
    { return 10.0f; }

    return Settings->Get_PendingChannelTimeoutSeconds();
}

// --------------------------------------------------------------------------------------------------------------------
