#pragma once

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Enums/CkEnums.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"
#include "CkEcsExt/Transform/CkTransform_Fragment_Data.h"

#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "CkChain_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Chain_Solver : uint8
{
    PathHistory,
    DistanceConstraint
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Chain_Solver);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Chain_LinkOrientation : uint8
{
    FollowPath,
    CopyHead,
    KeepOwn
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Chain_LinkOrientation);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Chain_HistorySeed : uint8
{
    StraightBehindHead,
    HoldUntilCovered
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Chain_HistorySeed);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Chain_NetPolicy : uint8
{
    AuthorityOnly,
    Everywhere
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Chain_NetPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Chain_LinkDetachReason : uint8
{
    Requested,
    LinkDestroyed,
    ChainDestroyed,
    MovedBySplit
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Chain_LinkDetachReason);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCHAIN_API FCk_Handle_Chain : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Chain); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Chain);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKCHAIN_API FCk_Handle_ChainLink : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_ChainLink); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_ChainLink);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_Chain_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Chain_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FGameplayTag _ChainName;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Chain_Solver _Solver = ECk_Chain_Solver::PathHistory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "1.0", UIMin = "1.0"))
    float _SampleSpacingCm = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Chain_HistorySeed _HistorySeed = ECk_Chain_HistorySeed::StraightBehindHead;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "0.0"))
    float _TeleportDistanceCm = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _UpVector = FVector::UpVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Chain_NetPolicy _NetPolicy = ECk_Chain_NetPolicy::AuthorityOnly;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _StartingState = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY(_ChainName);
    CK_PROPERTY(_Solver);
    CK_PROPERTY(_SampleSpacingCm);
    CK_PROPERTY(_HistorySeed);
    CK_PROPERTY(_TeleportDistanceCm);
    CK_PROPERTY(_UpVector);
    CK_PROPERTY(_NetPolicy);
    CK_PROPERTY(_StartingState);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Chain_Spec, _Solver);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_ChainLink_Spec
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_ChainLink_Spec);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true, ClampMin = "0.0", UIMin = "0.0"))
    float _DistanceFromHeadCm = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FTransform _LocalOffset = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Chain_LinkOrientation _Orientation = ECk_Chain_LinkOrientation::FollowPath;

public:
    CK_PROPERTY_GET(_DistanceFromHeadCm);
    CK_PROPERTY(_LocalOffset);
    CK_PROPERTY(_Orientation);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_ChainLink_Spec, _DistanceFromHeadCm);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_Chain_PathSample
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Chain_PathSample);

private:
    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FVector _Location = FVector::ZeroVector;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FQuat _Rotation = FQuat::Identity;

    UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    float _ArcDistanceCm = 0.0f;

public:
    CK_PROPERTY_GET(_Location);
    CK_PROPERTY_GET(_Rotation);
    CK_PROPERTY_GET(_ArcDistanceCm);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Chain_PathSample, _Location, _Rotation, _ArcDistanceCm);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_Request_Chain_AttachLink : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Chain_AttachLink);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Chain_AttachLink);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_Transform _Link;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_ChainLink_Spec _LinkSpec;

public:
    CK_PROPERTY_GET(_Link);
    CK_PROPERTY_GET(_LinkSpec);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Chain_AttachLink, _Link, _LinkSpec);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_Request_Chain_DetachLink : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Chain_DetachLink);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Chain_DetachLink);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_ChainLink _Link;

public:
    CK_PROPERTY_GET(_Link);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Chain_DetachLink, _Link);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_Request_Chain_SetLinkDistance : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Chain_SetLinkDistance);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Chain_SetLinkDistance);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_ChainLink _Link;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    float _DistanceFromHeadCm = 0.0f;

public:
    CK_PROPERTY_GET(_Link);
    CK_PROPERTY_GET(_DistanceFromHeadCm);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Chain_SetLinkDistance, _Link, _DistanceFromHeadCm);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_Request_Chain_EnableDisable : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Chain_EnableDisable);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Chain_EnableDisable);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _EnableDisable = ECk_EnableDisable::Enable;

public:
    CK_PROPERTY_GET(_EnableDisable);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Chain_EnableDisable, _EnableDisable);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_Request_Chain_ReseedHistory : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Chain_ReseedHistory);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Chain_ReseedHistory);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_Request_Chain_Split : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Chain_Split);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Chain_Split);

public:
    friend class UCk_Utils_Chain_UE;

private:
    FCk_Handle_Chain _NewChain;

public:
    CK_PROPERTY_GET(_NewChain);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_ChainLink _AtLink;

public:
    CK_PROPERTY_GET(_AtLink);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Chain_Split, _AtLink);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_Chain_Payload_LinkAttached
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Chain_Payload_LinkAttached);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_ChainLink _Link;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _Index = INDEX_NONE;

public:
    CK_PROPERTY_GET(_Link);
    CK_PROPERTY_GET(_Index);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Chain_Payload_LinkAttached, _Link, _Index);
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCk_Delegate_Chain_OnLinkAttached, FCk_Handle_Chain, InChain, FCk_Chain_Payload_LinkAttached, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_Chain_Payload_LinkDetached
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Chain_Payload_LinkDetached);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_ChainLink _Link;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_Chain_LinkDetachReason _Reason = ECk_Chain_LinkDetachReason::Requested;

public:
    CK_PROPERTY_GET(_Link);
    CK_PROPERTY_GET(_Reason);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Chain_Payload_LinkDetached, _Link, _Reason);
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCk_Delegate_Chain_OnLinkDetached, FCk_Handle_Chain, InChain, FCk_Chain_Payload_LinkDetached, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_Chain_Payload_Split
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Chain_Payload_Split);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_Handle_Chain _NewChain;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _AtIndex = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _MovedLinkCount = 0;

public:
    CK_PROPERTY_GET(_NewChain);
    CK_PROPERTY_GET(_AtIndex);
    CK_PROPERTY_GET(_MovedLinkCount);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Chain_Payload_Split, _NewChain, _AtIndex, _MovedLinkCount);
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCk_Delegate_Chain_OnSplit, FCk_Handle_Chain, InChain, FCk_Chain_Payload_Split, InPayload);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCHAIN_API FCk_Chain_Payload_HeadTeleported
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Chain_Payload_HeadTeleported);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _From = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FVector _To = FVector::ZeroVector;

public:
    CK_PROPERTY_GET(_From);
    CK_PROPERTY_GET(_To);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Chain_Payload_HeadTeleported, _From, _To);
};

DECLARE_DYNAMIC_DELEGATE_TwoParams(FCk_Delegate_Chain_OnHeadTeleported, FCk_Handle_Chain, InChain, FCk_Chain_Payload_HeadTeleported, InPayload);


