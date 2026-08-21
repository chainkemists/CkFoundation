#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "CkQueue_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Queue_LayoutAlgorithm : uint8
{
    OrthogonalSnake,
    Linear
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Queue_LayoutAlgorithm);

// ReserveOnFormation preserves the original eager behavior. ClaimFirstAvailableOnReach exposes one movement offer
// per origin; the following reservation is not offered until the current mover reports Reached.
UENUM(BlueprintType)
enum class ECk_Queue_SlotClaimPolicy : uint8
{
    ReserveOnFormation,
    ClaimFirstAvailableOnReach
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Queue_SlotClaimPolicy);

UENUM(BlueprintType)
enum class ECk_Queue_State : uint8
{
    NeedsSetup,
    Ready,
    WaitingForFormation,
    WaitingForNavigationChange,
    Invalidated
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Queue_State);

UENUM(BlueprintType)
enum class ECk_Queue_MemberState : uint8
{
    None,
    PendingAdmission,
    Assigned,
    MovingToSlot,
    AtSlot,
    AtFront,
    MovementSuppressed,
    WaitingForNavigationChange,
    WaitingForMover,
    Serving,
    Rejected,
    Invalidated
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Queue_MemberState);

UENUM(BlueprintType)
enum class ECk_Queue_EventReason : uint8
{
    None,
    Joined,
    Rejoined,
    Left,
    Advanced,
    Reflowed,
    OriginAssigned,
    OriginReassigned,
    OriginsChanged,
    LayoutChanged,
    MovementSuppressed,
    MovementResumed,
    SlotReached,
    HardLimitReached,
    OriginHardLimitReached,
    SoftLimitEntered,
    SoftLimitExited,
    InvalidRequest,
    OwnerDestroyed,
    MemberDestroyed,
    NavigationUnavailable,
    NoViableFormation,
    NavigationRetryStarted,
    NavigationRetryExhausted,
    NavigationChanged,
    MovementFailed,
    MovementCancelled,
    SearchBudgetExhausted
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Queue_EventReason);

UENUM(BlueprintType)
enum class ECk_Queue_MovementOutcome : uint8
{
    Reached,
    Failed,
    Cancelled
};
CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Queue_MovementOutcome);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake, HasNativeBreak))
struct CKQUEUE_API FCk_Handle_Queue : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_Queue);
};
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_Queue);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Queue_Origin
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Queue_Origin);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FTransform _LocalTransform = FTransform::Identity;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _Weight = 1;

    // INDEX_NONE inherits the queue-wide limit; zero disables the per-origin limit.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "-1"))
    int32 _HardLimitOverride = INDEX_NONE;

public:
    CK_PROPERTY(_LocalTransform);
    CK_PROPERTY(_Weight);
    CK_PROPERTY(_HardLimitOverride);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Queue_Origin, _LocalTransform);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Fragment_Queue_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_Queue_ParamsData);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_Weight"))
    TArray<FCk_Queue_Origin> _Origins;

    // Optional service category. When set, Add also stamps this as the owner's CkEntityTag so
    // gameplay can discover queues such as Queue.Category.Checkout without retaining a handle.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Queue.Category"))
    FGameplayTag _Category;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1.0"))
    float _SlotSpacingUu = 120.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _SoftLimit = 5;

    // Zero means unlimited.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _HardLimit = 12;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Queue_LayoutAlgorithm _LayoutAlgorithm = ECk_Queue_LayoutAlgorithm::OrthogonalSnake;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Queue_SlotClaimPolicy _SlotClaimPolicy = ECk_Queue_SlotClaimPolicy::ReserveOnFormation;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _TransformEpsilonUu = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _RotationEpsilonDegrees = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0"))
    int32 _MaxNavigationRetries = 3;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _NavigationRetryDelaySeconds = 3.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "1"))
    int32 _MaxFormationSearchNodes = 4096;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _AgentRadiusUu = 42.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _AgentHalfHeightUu = 96.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, ClampMin = "0.0"))
    float _ClearanceMarginUu = 8.0f;

public:
    CK_PROPERTY_GET(_Origins);
    CK_PROPERTY(_Category);
    CK_PROPERTY(_SlotSpacingUu);
    CK_PROPERTY(_SoftLimit);
    CK_PROPERTY(_HardLimit);
    CK_PROPERTY(_LayoutAlgorithm);
    CK_PROPERTY(_SlotClaimPolicy);
    CK_PROPERTY(_TransformEpsilonUu);
    CK_PROPERTY(_RotationEpsilonDegrees);
    CK_PROPERTY(_MaxNavigationRetries);
    CK_PROPERTY(_NavigationRetryDelaySeconds);
    CK_PROPERTY(_MaxFormationSearchNodes);
    CK_PROPERTY(_AgentRadiusUu);
    CK_PROPERTY(_AgentHalfHeightUu);
    CK_PROPERTY(_ClearanceMarginUu);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_Queue_ParamsData, _Origins);
};

// --------------------------------------------------------------------------------------------------------------------

// Detached debug data. These structures intentionally contain only copied values: they never retain an ECS handle,
// registry, fragment reference, or UObject. Consumers can cache one frame's snapshot safely across queue teardown.
USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Queue_DebugMemberSnapshot
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Queue_DebugMemberSnapshot);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int64 _MemberIdentity = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int64 _MoverIdentity = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FName _MemberDebugName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FName _MoverDebugName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _HasMoverWorldTransform = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FTransform _MoverWorldTransform = FTransform::Identity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int64 _Ticket = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _OriginIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _Rank = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FTransform _TargetWorldTransform = FTransform::Identity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _AssignmentRevision = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    bool _MovementSuppressed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_Queue_MemberState _State = ECk_Queue_MemberState::None;

public:
    CK_PROPERTY_GET(_MemberIdentity);
    CK_PROPERTY_GET(_MoverIdentity);
    CK_PROPERTY_GET(_MemberDebugName);
    CK_PROPERTY_GET(_MoverDebugName);
    CK_PROPERTY_GET(_HasMoverWorldTransform);
    CK_PROPERTY_GET(_MoverWorldTransform);
    CK_PROPERTY_GET(_Ticket);
    CK_PROPERTY_GET(_OriginIndex);
    CK_PROPERTY_GET(_Rank);
    CK_PROPERTY_GET(_TargetWorldTransform);
    CK_PROPERTY_GET(_AssignmentRevision);
    CK_PROPERTY_GET(_MovementSuppressed);
    CK_PROPERTY_GET(_State);

public:
    FCk_Queue_DebugMemberSnapshot() = default;

    FCk_Queue_DebugMemberSnapshot(
        int64 InMemberIdentity,
        int64 InMoverIdentity,
        FName InMemberDebugName,
        FName InMoverDebugName,
        bool InHasMoverWorldTransform,
        FTransform InMoverWorldTransform,
        int64 InTicket,
        int32 InOriginIndex,
        int32 InRank,
        FTransform InTargetWorldTransform,
        int32 InAssignmentRevision,
        bool InMovementSuppressed,
        ECk_Queue_MemberState InState)
        : _MemberIdentity(InMemberIdentity)
        , _MoverIdentity(InMoverIdentity)
        , _MemberDebugName(InMemberDebugName)
        , _MoverDebugName(InMoverDebugName)
        , _HasMoverWorldTransform(InHasMoverWorldTransform)
        , _MoverWorldTransform(MoveTemp(InMoverWorldTransform))
        , _Ticket(InTicket)
        , _OriginIndex(InOriginIndex)
        , _Rank(InRank)
        , _TargetWorldTransform(MoveTemp(InTargetWorldTransform))
        , _AssignmentRevision(InAssignmentRevision)
        , _MovementSuppressed(InMovementSuppressed)
        , _State(InState)
    {}
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Queue_MemberSnapshot
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Queue_MemberSnapshot);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Member;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Mover;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int64 _Ticket = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _OriginIndex = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _Rank = INDEX_NONE;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FTransform _TargetWorldTransform = FTransform::Identity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _AssignmentRevision = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    bool _MovementSuppressed = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Queue_MemberState _State = ECk_Queue_MemberState::None;

public:
    CK_PROPERTY_GET(_Member);
    CK_PROPERTY_GET(_Mover);
    CK_PROPERTY_GET(_Ticket);
    CK_PROPERTY_GET(_OriginIndex);
    CK_PROPERTY_GET(_Rank);
    CK_PROPERTY_GET(_TargetWorldTransform);
    CK_PROPERTY_GET(_AssignmentRevision);
    CK_PROPERTY_GET(_MovementSuppressed);
    CK_PROPERTY_GET(_State);

public:
    CK_DEFINE_CONSTRUCTORS(
        FCk_Queue_MemberSnapshot,
        _Member,
        _Mover,
        _Ticket,
        _OriginIndex,
        _Rank,
        _TargetWorldTransform,
        _AssignmentRevision,
        _MovementSuppressed,
        _State);
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Queue_MemberEvent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Queue_MemberEvent);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Queue;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Queue_MemberSnapshot _Member;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Queue_MemberState _PreviousState = ECk_Queue_MemberState::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Queue_EventReason _Reason = ECk_Queue_EventReason::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _QueueRevision = 0;

public:
    CK_PROPERTY_GET(_Queue);
    CK_PROPERTY_GET(_Member);
    CK_PROPERTY_GET(_PreviousState);
    CK_PROPERTY_GET(_Reason);
    CK_PROPERTY_GET(_QueueRevision);

public:
    CK_DEFINE_CONSTRUCTORS(
        FCk_Queue_MemberEvent,
        _Queue,
        _Member,
        _PreviousState,
        _Reason,
        _QueueRevision);
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Queue_Pressure
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Queue_Pressure);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _MemberCount = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _SoftLimit = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _HardLimit = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    bool _IsSoftLimited = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    bool _IsHardLimited = false;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<int32> _OriginMemberCounts;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _QueueRevision = 0;

public:
    CK_PROPERTY_GET(_MemberCount);
    CK_PROPERTY_GET(_SoftLimit);
    CK_PROPERTY_GET(_HardLimit);
    CK_PROPERTY_GET(_IsSoftLimited);
    CK_PROPERTY_GET(_IsHardLimited);
    CK_PROPERTY_GET(_OriginMemberCounts);
    CK_PROPERTY_GET(_QueueRevision);

public:
    CK_DEFINE_CONSTRUCTORS(
        FCk_Queue_Pressure,
        _MemberCount,
        _SoftLimit,
        _HardLimit,
        _IsSoftLimited,
        _IsHardLimited,
        _OriginMemberCounts,
        _QueueRevision);
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Queue_FormationState
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Queue_FormationState);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Queue_State _State = ECk_Queue_State::NeedsSetup;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_Queue_EventReason _Reason = ECk_Queue_EventReason::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _QueueRevision = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _RetryEpisode = 0;

public:
    CK_PROPERTY_GET(_State);
    CK_PROPERTY_GET(_Reason);
    CK_PROPERTY_GET(_QueueRevision);
    CK_PROPERTY_GET(_RetryEpisode);

public:
    CK_DEFINE_CONSTRUCTORS(
        FCk_Queue_FormationState,
        _State,
        _Reason,
        _QueueRevision,
        _RetryEpisode);
};

// Detached debug data. These structures intentionally contain only copied values: they never retain an ECS handle,
// registry, fragment reference, or UObject. Consumers can cache one frame's snapshot safely across queue teardown.
USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Queue_DebugSnapshot
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Queue_DebugSnapshot);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int64 _QueueIdentity = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FName _QueueDebugName;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FGameplayTag _Category;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FTransform _OwnerWorldTransform = FTransform::Identity;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_Queue_State _State = ECk_Queue_State::NeedsSetup;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _Revision = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    int32 _RetryEpisode = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    ECk_Queue_LayoutAlgorithm _LayoutAlgorithm = ECk_Queue_LayoutAlgorithm::OrthogonalSnake;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    FCk_Queue_Pressure _Pressure;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FTransform> _OriginWorldTransforms;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = true))
    TArray<FCk_Queue_DebugMemberSnapshot> _Members;

public:
    CK_PROPERTY_GET(_QueueIdentity);
    CK_PROPERTY_GET(_QueueDebugName);
    CK_PROPERTY_GET(_Category);
    CK_PROPERTY_GET(_OwnerWorldTransform);
    CK_PROPERTY_GET(_State);
    CK_PROPERTY_GET(_Revision);
    CK_PROPERTY_GET(_RetryEpisode);
    CK_PROPERTY_GET(_LayoutAlgorithm);
    CK_PROPERTY_GET(_Pressure);
    CK_PROPERTY_GET(_OriginWorldTransforms);
    CK_PROPERTY_GET(_Members);

public:
    FCk_Queue_DebugSnapshot() = default;

    FCk_Queue_DebugSnapshot(
        int64 InQueueIdentity,
        FName InQueueDebugName,
        FGameplayTag InCategory,
        FTransform InOwnerWorldTransform,
        ECk_Queue_State InState,
        int32 InRevision,
        int32 InRetryEpisode,
        ECk_Queue_LayoutAlgorithm InLayoutAlgorithm,
        FCk_Queue_Pressure InPressure,
        TArray<FTransform> InOriginWorldTransforms,
        TArray<FCk_Queue_DebugMemberSnapshot> InMembers)
        : _QueueIdentity(InQueueIdentity)
        , _QueueDebugName(InQueueDebugName)
        , _Category(InCategory)
        , _OwnerWorldTransform(MoveTemp(InOwnerWorldTransform))
        , _State(InState)
        , _Revision(InRevision)
        , _RetryEpisode(InRetryEpisode)
        , _LayoutAlgorithm(InLayoutAlgorithm)
        , _Pressure(MoveTemp(InPressure))
        , _OriginWorldTransforms(MoveTemp(InOriginWorldTransforms))
        , _Members(MoveTemp(InMembers))
    {}
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Request_Queue_Join : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Queue_Join);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Queue_Join);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Member;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Mover;

public:
    CK_PROPERTY_GET(_Member);
    CK_PROPERTY(_Mover);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Queue_Join, _Member);
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Request_Queue_Leave : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Queue_Leave);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Queue_Leave);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Member;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Queue_EventReason _Reason = ECk_Queue_EventReason::Left;

public:
    CK_PROPERTY_GET(_Member);
    CK_PROPERTY(_Reason);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Queue_Leave, _Member);
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Request_Queue_AdvanceOrigin : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Queue_AdvanceOrigin);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Queue_AdvanceOrigin);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _OriginIndex = INDEX_NONE;

public:
    CK_PROPERTY_GET(_OriginIndex);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Queue_AdvanceOrigin, _OriginIndex);
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Request_Queue_SetOrigins : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Queue_SetOrigins);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Queue_SetOrigins);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_Queue_Origin> _Origins;

public:
    CK_PROPERTY_GET(_Origins);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Queue_SetOrigins, _Origins);
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Request_Queue_SetLayout : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Queue_SetLayout);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Queue_SetLayout);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Queue_LayoutAlgorithm _LayoutAlgorithm = ECk_Queue_LayoutAlgorithm::OrthogonalSnake;

public:
    CK_PROPERTY_GET(_LayoutAlgorithm);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_Queue_SetLayout, _LayoutAlgorithm);
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Request_Queue_SetMovementSuppressed : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Queue_SetMovementSuppressed);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Queue_SetMovementSuppressed);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Member;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_EnableDisable _MovementSuppressed = ECk_EnableDisable::Disable;

public:
    CK_PROPERTY_GET(_Member);
    CK_PROPERTY_GET(_MovementSuppressed);

public:
    CK_DEFINE_CONSTRUCTORS(
        FCk_Request_Queue_SetMovementSuppressed,
        _Member,
        _MovementSuppressed);
};

USTRUCT(BlueprintType)
struct CKQUEUE_API FCk_Request_Queue_ReportMovementOutcome : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_Queue_ReportMovementOutcome);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_Queue_ReportMovementOutcome);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle _Member;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _AssignmentRevision = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Queue_MovementOutcome _Outcome = ECk_Queue_MovementOutcome::Reached;

public:
    CK_PROPERTY_GET(_Member);
    CK_PROPERTY_GET(_AssignmentRevision);
    CK_PROPERTY_GET(_Outcome);

public:
    CK_DEFINE_CONSTRUCTORS(
        FCk_Request_Queue_ReportMovementOutcome,
        _Member,
        _AssignmentRevision,
        _Outcome);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Queue_OnMemberStateChanged,
    FCk_Handle_Queue, InQueue,
    FCk_Queue_MemberEvent, InEvent);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Queue_OnPressureChanged,
    FCk_Handle_Queue, InQueue,
    FCk_Queue_Pressure, InPressure);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Queue_OnFormationStateChanged,
    FCk_Handle_Queue, InQueue,
    FCk_Queue_FormationState, InFormationState);

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_Queue_OnInvalidated,
    FCk_Handle_Queue, InQueue,
    FCk_Queue_FormationState, InFormationState);
