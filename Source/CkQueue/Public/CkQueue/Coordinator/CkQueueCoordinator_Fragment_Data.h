#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkQueue/Queue/CkQueue_Fragment_Data.h"

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "CkQueueCoordinator_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_QueueCoordinator_SelectionPolicy : uint8
{
    LeastMembersThenDistance,
    NearestThenLeastMembers
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_QueueCoordinator_SelectionPolicy);

UENUM(BlueprintType)
enum class ECk_QueueCoordinator_SelectOutcome : uint8
{
    Selected,
    AlreadyQueued,
    NoRegisteredQueues,
    NoEligibleQueue,
    MemberInMultipleQueues,
    Cancelled
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_QueueCoordinator_SelectOutcome);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKQUEUE_API FCk_Handle_QueueCoordinator : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_QueueCoordinator);
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_QueueCoordinator);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Fragment_QueueCoordinator_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_QueueCoordinator_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Queue.Category"))
    FGameplayTag _RequiredQueueCategory;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_QueueCoordinator_SelectionPolicy _SelectionPolicy
        = ECk_QueueCoordinator_SelectionPolicy::LeastMembersThenDistance;

public:
    CK_PROPERTY(_RequiredQueueCategory);
    CK_PROPERTY(_SelectionPolicy);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_QueueCoordinator_Service
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_QueueCoordinator_Service);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Queue _Queue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _RegistrationOrdinal = 0;

public:
    CK_PROPERTY_GET(_Queue);
    CK_PROPERTY_GET(_RegistrationOrdinal);

public:
    CK_DEFINE_CONSTRUCTORS(
        FCk_QueueCoordinator_Service,
        _Queue,
        _RegistrationOrdinal);
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_QueueCoordinator_SelectResult
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_QueueCoordinator_SelectResult);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_QueueCoordinator_SelectOutcome _Outcome = ECk_QueueCoordinator_SelectOutcome::NoEligibleQueue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Member;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Queue _SelectedQueue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _CoordinatorRevision = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _RegistrationOrdinal = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _ProjectedMemberCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    float _DistanceUu = 0.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle_Queue> _EligibleFallbackQueues;

public:
    CK_PROPERTY_GET(_Outcome);
    CK_PROPERTY_GET(_Member);
    CK_PROPERTY_GET(_SelectedQueue);
    CK_PROPERTY_GET(_CoordinatorRevision);
    CK_PROPERTY_GET(_RegistrationOrdinal);
    CK_PROPERTY_GET(_ProjectedMemberCount);
    CK_PROPERTY_GET(_DistanceUu);
    CK_PROPERTY_GET(_EligibleFallbackQueues);

public:
    CK_DEFINE_CONSTRUCTORS(
        FCk_QueueCoordinator_SelectResult,
        _Outcome,
        _Member,
        _SelectedQueue,
        _CoordinatorRevision,
        _RegistrationOrdinal,
        _ProjectedMemberCount,
        _DistanceUu,
        _EligibleFallbackQueues);
};

DECLARE_DYNAMIC_DELEGATE_OneParam(
    FCk_Delegate_QueueCoordinator_OnSelected,
    FCk_QueueCoordinator_SelectResult, InResult);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Request_QueueCoordinator_RegisterQueue : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_QueueCoordinator_RegisterQueue);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_QueueCoordinator_RegisterQueue);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Queue _Queue;

public:
    CK_PROPERTY_GET(_Queue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_QueueCoordinator_RegisterQueue, _Queue);
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Request_QueueCoordinator_UnregisterQueue : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_QueueCoordinator_UnregisterQueue);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_QueueCoordinator_UnregisterQueue);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_Queue _Queue;

public:
    CK_PROPERTY_GET(_Queue);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_QueueCoordinator_UnregisterQueue, _Queue);
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Request_QueueCoordinator_SelectQueue : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_QueueCoordinator_SelectQueue);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_QueueCoordinator_SelectQueue);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Member;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FVector _WorldLocation = FVector::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle_Queue> _ExcludedQueues;

    UPROPERTY()
    FCk_Delegate_QueueCoordinator_OnSelected _ResultDelegate;

public:
    CK_PROPERTY_GET(_Member);
    CK_PROPERTY_GET(_WorldLocation);
    CK_PROPERTY(_ExcludedQueues);
    CK_PROPERTY(_ResultDelegate);

public:
    CK_DEFINE_CONSTRUCTORS(
        FCk_Request_QueueCoordinator_SelectQueue,
        _Member,
        _WorldLocation);
};

// --------------------------------------------------------------------------------------------------------------------
