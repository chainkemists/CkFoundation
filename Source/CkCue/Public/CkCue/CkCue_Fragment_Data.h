#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkCue_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCUE_API FCk_Handle_Cue : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Cue); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Cue);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCUE_API FCk_Request_Cue_Execute : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Cue_Execute);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Cue_Execute);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FGameplayTag _CueName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess = true))
    FInstancedStruct _SpawnParams;

public:
    CK_PROPERTY_GET(_CueName);
    CK_PROPERTY_GET(_SpawnParams);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Cue_Execute, _CueName, _SpawnParams);
};

// --------------------------------------------------------------------------------------------------------------------
