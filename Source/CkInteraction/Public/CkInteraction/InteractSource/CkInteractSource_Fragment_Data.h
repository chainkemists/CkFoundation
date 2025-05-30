#pragma once

#include "CkEcs/Handle/CkHandle.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/EntityConstructionScript/CkEntity_ConstructionScript.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkInteraction/Interaction/CkInteraction_Fragment_Data.h"

#include <GameplayTagContainer.h>

#include "CkInteractSource_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    class FProcessor_InteractSource_HandleRequests;
}

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_InteractionSource_ConcurrentInteractionsPolicy : uint8
{
    SingleInteraction,
    MultipleInteractions
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InteractionSource_ConcurrentInteractionsPolicy);

UENUM(BlueprintType)
enum class ECk_InteractionSource_SortingPolicy : uint8
{
    NoSorting,
    ClosestToFarthest,
    FarthestToClosest
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_InteractionSource_SortingPolicy);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKINTERACTION_API FCk_Handle_InteractSource : public FCk_Handle_TypeSafe { GENERATED_BODY() CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_InteractSource); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_InteractSource);

//--------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Fragment_InteractSource_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_InteractSource_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "InteractionChannel"))
    FGameplayTag _InteractionChannel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_InteractionSource_ConcurrentInteractionsPolicy _ConcurrentInteractionsPolicy = ECk_InteractionSource_ConcurrentInteractionsPolicy::MultipleInteractions;

public:
    CK_PROPERTY_GET(_InteractionChannel)
    CK_PROPERTY_GET(_ConcurrentInteractionsPolicy)
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Fragment_MultipleInteractSource_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_MultipleInteractSource_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_InteractionChannel"))
    TArray<FCk_Fragment_InteractSource_ParamsData> _InteractSourceParams;

public:
    CK_PROPERTY_GET(_InteractSourceParams)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_MultipleInteractSource_ParamsData, _InteractSourceParams);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Request_InteractSource_StartInteraction : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_InteractSource_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_InteractSource_StartInteraction);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InteractSource_StartInteraction);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Interaction _Interaction;

public:
    CK_PROPERTY(_Interaction)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InteractSource_StartInteraction, _Interaction);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKINTERACTION_API FCk_Request_InteractSource_CancelInteraction : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    friend class ck::FProcessor_InteractSource_HandleRequests;

public:
    CK_GENERATED_BODY(FCk_Request_InteractSource_CancelInteraction);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_InteractSource_CancelInteraction);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _InteractTarget;

public:
    CK_PROPERTY(_InteractTarget)

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_InteractSource_CancelInteraction, _InteractTarget);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_InteractSource_OnNewInteraction,
    FCk_Handle_InteractSource, InSource,
    FCk_Handle_Interaction, InInteraction);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(
    FCk_Delegate_InteractSource_OnNewInteraction_MC,
    FCk_Handle_InteractSource, InSource,
    FCk_Handle_Interaction, InInteraction);

DECLARE_DYNAMIC_DELEGATE_ThreeParams(
    FCk_Delegate_InteractSource_OnInteractionFinished,
    FCk_Handle_InteractSource, InSource,
    FCk_Handle_Interaction, InInteraction,
    ECk_SucceededFailed, SucceededFailed);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(
    FCk_Delegate_InteractSource_OnInteractionFinished_MC,
    FCk_Handle_InteractSource, InSource,
    FCk_Handle_Interaction, InInteraction,
    ECk_SucceededFailed, SucceededFailed);

// --------------------------------------------------------------------------------------------------------------------