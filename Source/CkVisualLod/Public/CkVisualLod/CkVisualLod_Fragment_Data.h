#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkIskmRenderer/Proxy/CkIskmProxy_Fragment_Data.h"

#include "CkVisualLod/CkVisualLodArbiter_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkVisualLod_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_IskmRenderer_Data;
class ACk_Iskm_BatchedCrowd_Actor;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_VisualLod_Representation : uint8
{
    // Managed, but holding neither a crowd member nor a proxy (e.g. hidden, or pool-exhausted
    // under the Unrendered policy)
    None,

    FarMember,

    PromotedProxy
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VisualLod_Representation);

UENUM(BlueprintType)
enum class ECk_VisualLod_ShowHide : uint8
{
    Show,
    Hide
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VisualLod_ShowHide);

UENUM(BlueprintType)
enum class ECk_VisualLod_PromotionMode : uint8
{
    // The arbiter decides the representation (distance + ranking + budgets)
    Managed,

    // Never takes a crowd member; promoted on first update and never demoted. For entities whose
    // body mesh is not any crowd's shared mesh — a member slot would render the wrong body at range
    AlwaysPromoted
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VisualLod_PromotionMode);

UENUM(BlueprintType)
enum class ECk_VisualLod_FarAnimMode : uint8
{
    // Idle below the crowd config's speed threshold; move sequence rate-scaled by speed above it
    SpeedDriven,

    // A fixed sequence index + rate (ambient set-dressing, full-body overrides)
    Fixed
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_VisualLod_FarAnimMode);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKVISUALLOD_API FCk_Handle_VisualLod : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_VisualLod); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_VisualLod);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_VisualLod_FarAnim
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_VisualLod_FarAnim);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_VisualLod_FarAnimMode _Mode = ECk_VisualLod_FarAnimMode::SpeedDriven;

    // Index into the crowd's anim collection. Fixed mode only
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_Mode == ECk_VisualLod_FarAnimMode::Fixed", EditConditionHides))
    int32 _FixedSequenceIndex = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, EditCondition = "_Mode == ECk_VisualLod_FarAnimMode::Fixed", EditConditionHides))
    float _FixedRate = 1.0f;

public:
    CK_PROPERTY_GET(_Mode);
    CK_PROPERTY(_FixedSequenceIndex);
    CK_PROPERTY(_FixedRate);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_VisualLod_FarAnim, _Mode);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_Fragment_VisualLod_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_VisualLod_ParamsData);

private:
    // Which arbiter (LOD domain) manages this entity. Resolved lazily against the arbiter whose
    // config carries the same tag; an explicit Request_SetArbiter overrides the tag
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "VisualLod"))
    FGameplayTag _ArbiterTag;

    // Index into the arbiter config's CrowdConfigs — which batched crowd renders this entity at range
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = 0))
    int32 _CrowdIndex = 0;

    // Renderer for the promoted proxy (pooled SKMC + shared AnimBP)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCk_IskmRenderer_Data> _Renderer;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_VisualLod_PromotionMode _PromotionMode = ECk_VisualLod_PromotionMode::Managed;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_VisualLod_FarAnim _InitialFarAnim;

public:
    CK_PROPERTY_GET(_ArbiterTag);
    CK_PROPERTY(_CrowdIndex);
    CK_PROPERTY(_Renderer);
    CK_PROPERTY(_PromotionMode);
    CK_PROPERTY(_InitialFarAnim);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_VisualLod_ParamsData, _ArbiterTag);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_Request_VisualLod_SetArbiter : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VisualLod_SetArbiter);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VisualLod_SetArbiter);

private:
    // Overrides the Params tag resolution. Switching a managed entity between arbiters releases
    // its slot from the old domain first (fail-closed, same path as Resume)
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_VisualLodArbiter _Arbiter;

public:
    CK_PROPERTY_GET(_Arbiter);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VisualLod_SetArbiter, _Arbiter);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_Request_VisualLod_SetVisibility : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VisualLod_SetVisibility);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VisualLod_SetVisibility);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_VisualLod_ShowHide _ShowHide = ECk_VisualLod_ShowHide::Show;

public:
    CK_PROPERTY_GET(_ShowHide);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VisualLod_SetVisibility, _ShowHide);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_Request_VisualLod_SetFarAnim : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VisualLod_SetFarAnim);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VisualLod_SetFarAnim);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_VisualLod_FarAnim _FarAnim;

public:
    CK_PROPERTY_GET(_FarAnim);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VisualLod_SetFarAnim, _FarAnim);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_Request_VisualLod_SetRenderer : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VisualLod_SetRenderer);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VisualLod_SetRenderer);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TSoftObjectPtr<UCk_IskmRenderer_Data> _Renderer;

public:
    CK_PROPERTY_GET(_Renderer);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_VisualLod_SetRenderer, _Renderer);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_Request_VisualLod_Suspend : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VisualLod_Suspend);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VisualLod_Suspend);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKVISUALLOD_API FCk_Request_VisualLod_Resume : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_VisualLod_Resume);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_VisualLod_Resume);
};

// --------------------------------------------------------------------------------------------------------------------

// The crowd the member index addresses is read off the handle at handler time
// (UCk_Utils_VisualLod_UE::Get_Crowd) — an actor pointer must not ride a replayable signal payload
DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_VisualLod_MemberEvent,
    FCk_Handle_VisualLod, InHandle,
    int32, InMemberIndex);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_VisualLod_Promoted,
    FCk_Handle_VisualLod, InHandle,
    FCk_Handle_IskmProxy, InProxy);

// --------------------------------------------------------------------------------------------------------------------
