#include "CkNet_Settings.h"

#include "CkCore/Object/CkObject_Utils.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_Net_Settings_UE::
    Get_EcsReplicationChannel()
    -> TSubclassOf<ACk_EcsChannel_Actor_UE>
{
    const auto& Settings = UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Net_ProjectSettings_UE>();
    const auto EcsReplicationChannel = TSubclassOf<ACk_EcsChannel_Actor_UE>{Settings->Get_EcsReplicationChannel().TryLoadClass<ACk_EcsChannel_Actor_UE>()};

    CK_ENSURE_IF_NOT(ck::IsValid(EcsReplicationChannel),
        TEXT("Could not load EcsReplicationChannel from [{}] defined in project settings"), EcsReplicationChannel)
    { return {}; }

    return EcsReplicationChannel;
}

auto
    UCk_Utils_Net_Settings_UE::
    Get_NumberOfEcsReplicationChannels()
    -> int32
{
    return UCk_Utils_Object_UE::Get_ClassDefaultObject<UCk_Net_ProjectSettings_UE>()->Get_NumberOfEcsReplicationChannels();
}

// --------------------------------------------------------------------------------------------------------------------
