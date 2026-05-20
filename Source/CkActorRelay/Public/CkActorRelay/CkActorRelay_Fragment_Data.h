#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include <GameplayTagContainer.h>

#include "CkActorRelay_Fragment_Data.generated.h"

class ACk_ActorRelay_UE;
class APlayerState;
class UCk_ActorRelay_Group_Subsystem_Base_UE;

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
                       ACQUIRE KIND
------------------------------------------------------------------------------*/

UENUM(BlueprintType)
enum class ECk_ActorRelay_AcquireKind : uint8
{
    Server,
    ForPlayer,
    Any
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_ActorRelay_AcquireKind);

/*-----------------------------------------------------------------------------
                       ACQUIRE DELEGATE
------------------------------------------------------------------------------*/

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_ActorRelay_Acquired,
    FCk_ActorRelay_ChannelResult, InResult);

DECLARE_MULTICAST_DELEGATE_OneParam(
    FCk_Delegate_ActorRelay_Acquired_MC,
    FCk_ActorRelay_ChannelResult);

/*-----------------------------------------------------------------------------
                       PENDING ACTOR RELAY HANDLE
------------------------------------------------------------------------------*/

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKACTORRELAY_API FCk_Handle_PendingActorRelay
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Handle_PendingActorRelay);

    friend class UCk_ActorRelay_Group_Subsystem_Base_UE;
    friend class UCk_Utils_PendingActorRelay_UE;

private:
    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<UCk_ActorRelay_Group_Subsystem_Base_UE> _GroupSubsystem;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TWeakObjectPtr<APlayerState> _PlayerState;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_ActorRelay_AcquireKind _Kind = ECk_ActorRelay_AcquireKind::Server;

    UPROPERTY(Transient, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _Resolved = false;

    // Tracks the pending subscription on the subsystem's _OnChannelReadyChanged.
    // Kept for diagnostics / future cancellation; the lambda itself owns its
    // unbind via a shared FDelegateHandle, so this field can be dropped on the
    // floor without leaking.
    FDelegateHandle _SubscriptionHandle;

public:
    CK_PROPERTY_GET(_GroupSubsystem);
    CK_PROPERTY_GET(_PlayerState);
    CK_PROPERTY_GET(_Kind);
    CK_PROPERTY_GET(_Resolved);
};

CK_DECLARE_CUSTOM_IS_VALID(CKACTORRELAY_API, FCk_Handle_PendingActorRelay, IsValid_Policy_Default);

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
