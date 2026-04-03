#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkActorRelay_Fragment_Data.generated.h"

class ACk_ActorRelay_UE;

/*-----------------------------------------------------------------------------
                               HANDLE
------------------------------------------------------------------------------*/

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKACTORRELAY_API FCk_Handle_ActorRelay : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ActorRelay); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ActorRelay);

/*-----------------------------------------------------------------------------
                            OWNERSHIP POLICY
------------------------------------------------------------------------------*/

UENUM(BlueprintType)
enum class ECk_ActorRelay_OwnershipPolicy : uint8
{
    ServerOwned,
    PlayerOwned
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ActorRelay_OwnershipPolicy);

/*-----------------------------------------------------------------------------
                           DISCONNECT POLICY
------------------------------------------------------------------------------*/

UENUM(BlueprintType)
enum class ECk_ActorRelay_DisconnectPolicy : uint8
{
    DestroyChannels,
    KeepAlive
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ActorRelay_DisconnectPolicy);

/*-----------------------------------------------------------------------------
                          SELECTION ALGORITHM
------------------------------------------------------------------------------*/

UENUM(BlueprintType)
enum class ECk_ActorRelay_SelectionAlgorithm : uint8
{
    RoundRobin,
    LeastLoaded
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ActorRelay_SelectionAlgorithm);

/*-----------------------------------------------------------------------------
                           CHANNEL RESULT
------------------------------------------------------------------------------*/

USTRUCT(BlueprintType)
struct CKACTORRELAY_API FCk_ActorRelay_ChannelResult
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ActorRelay_ChannelResult);

private:
    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<ACk_ActorRelay_UE> _ChannelActor;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Handle _ChannelEntity;

public:
    CK_PROPERTY_GET(_ChannelActor);
    CK_PROPERTY_GET(_ChannelEntity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ActorRelay_ChannelResult, _ChannelActor, _ChannelEntity);
};

CK_DECLARE_CUSTOM_IS_VALID(CKACTORRELAY_API, FCk_ActorRelay_ChannelResult, IsValid_Policy_Default);

/*-----------------------------------------------------------------------------
                       ACQUIRE CHANNEL REQUEST
------------------------------------------------------------------------------*/

USTRUCT(BlueprintType)
struct CKACTORRELAY_API FCk_Request_ActorRelay_AcquireChannel : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_ActorRelay_AcquireChannel);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_ActorRelay_AcquireChannel);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "ActorRelay"))
    FGameplayTag _GroupTag;

public:
    CK_PROPERTY_GET(_GroupTag);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_ActorRelay_AcquireChannel, _GroupTag);
};

// --------------------------------------------------------------------------------------------------------------------
