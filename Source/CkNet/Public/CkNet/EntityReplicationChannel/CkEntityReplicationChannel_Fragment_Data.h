#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle_TypeSafe.h"

#include <GameFramework/Info.h>

#include "CkEntityReplicationChannel_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

CKNET_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Label_EntityReplicationChannel);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKNET_API FCk_Handle_EntityReplicationChannel : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_EntityReplicationChannel); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_EntityReplicationChannel);

// --------------------------------------------------------------------------------------------------------------------

UCLASS(Abstract, Blueprintable, NotBlueprintType, HideCategories=(Tick, Replication, Actor, Physics))
class CKNET_API ACk_EcsChannel_Actor_UE final : public AInfo
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(ACk_EcsChannel_Actor_UE);

public:
    ACk_EcsChannel_Actor_UE();
};

// --------------------------------------------------------------------------------------------------------------------
