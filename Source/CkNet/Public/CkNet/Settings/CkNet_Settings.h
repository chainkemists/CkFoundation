#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkNet/EntityReplicationChannel/CkEntityReplicationChannel_Fragment_Data.h"

#include "CkSettings/ProjectSettings/CkProjectSettings.h"

#include "CkNet_Settings.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(meta = (DisplayName = "Net"))
class CKNET_API UCk_Net_ProjectSettings_UE : public UCk_Plugin_ProjectSettings_UE
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Net_ProjectSettings_UE);

private:
    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "ECS World",
              meta = (AllowPrivateAccess = true, AllowAbstract = false, MetaClass = "/Script/CkNet.Ck_EcsChannel_Actor_UE"))
    FSoftClassPath _EcsReplicationChannel;

    UPROPERTY(Config, EditDefaultsOnly, BlueprintReadOnly, Category = "ECS Replication",
          meta = (AllowPrivateAccess = true, ClampMin = 1, UIMin = 1))
    int32 _NumberOfEcsReplicationChannels = 10;

public:
    CK_PROPERTY_GET(_EcsReplicationChannel);
    CK_PROPERTY_GET(_NumberOfEcsReplicationChannels);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS()
class CKNET_API UCk_Utils_Net_Settings_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Net_Settings_UE);

public:
    static auto Get_EcsReplicationChannel() -> TSubclassOf<ACk_EcsChannel_Actor_UE>;
    static auto Get_NumberOfEcsReplicationChannels() -> int32;
};

// --------------------------------------------------------------------------------------------------------------------
