#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "GameplayTagContainer.h"

#include "CkStateMachine_NetContext.generated.h"

class UCk_SmState_EntityScript;

// --------------------------------------------------------------------------------------------------------------------

// Per-machine role context passed into every State / Task / Condition lifecycle method.
// Users switch on this to branch behavior by role.
UENUM(BlueprintType)
enum class ECk_Sm_NetContext : uint8
{
    Standalone,        // Single-player / no net session
    Server,            // Authority on a networked server
    OwningClient,      // Client owning this SM's actor (player on their own machine)
    NonOwningClient    // Client observing someone else's SM
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Sm_NetContext);

// --------------------------------------------------------------------------------------------------------------------

// Per-SM choice of who drives transitions. Immutable for the SM's lifetime.
// Single-authority-per-SM rule (spec §5.6): an SM is either ServerAuth or OwningClientAuth, never mixed.
//
// AutoDetect is the default: the effective model is resolved on demand from the SM host's net
// ownership (player-controlled pawn -> OwningClientAuthoritative, everything else -> ServerAuthoritative).
// Resolve via UCk_Utils_StateMachine_UE::Get_EffectiveAuthorityModel — it never returns AutoDetect.
// Explicit ServerAuthoritative / OwningClientAuthoritative override the auto-resolution (e.g. for debugging
// or to force server authority on a player pawn).
UENUM(BlueprintType)
enum class ECk_Sm_AuthorityModel : uint8
{
    AutoDetect,                  // Default. Resolved from host ownership at use-time (see Get_EffectiveAuthorityModel).
    ServerAuthoritative,         // Server evaluates conditions, drives transitions.
    OwningClientAuthoritative    // Owning client evaluates locally, pushes to server via RPC.
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Sm_AuthorityModel);

// --------------------------------------------------------------------------------------------------------------------

// Per-SM choice of replication payload shape. Immutable for the SM's lifetime.
UENUM(BlueprintType)
enum class ECk_Sm_ReplicationModel : uint8
{
    WithHistory,       // Default. Rolling window of transition events; replays A→B→C in order.
    WithoutHistory     // Latest state only. Snap-to-current. For high-frequency / cosmetic SMs.
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Sm_ReplicationModel);

// --------------------------------------------------------------------------------------------------------------------

// One transition event in the replicated history. Carries previous → new + monotonic seq +
// the new state's structural fingerprint (for spec §9 determinism check).
USTRUCT(BlueprintType)
struct CKSTATEMACHINE_API FCk_Sm_TransitionEvent
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Sm_TransitionEvent);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmState_EntityScript> _PreviousStateClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    TSubclassOf<UCk_SmState_EntityScript> _NewStateClass;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    int32 _Seq = 0;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    int32 _NewStateFingerprint = 0;

    // Cross-machine address of the sub-SM this event belongs to: its FFragment_Sm_ParentHierarchy
    // (root->leaf state-tag path to its hosting state). EMPTY means a root-level event (the common
    // case — unchanged behavior). Non-empty means the event was relayed through the root SM's
    // transport on behalf of a non-replicated sub-SM, and the receiver must dispatch it to the local
    // sub-SM resolved by this path (see UCk_Utils_StateMachine_UE::TryFind_ActiveSubSm_ByParentHierarchy).
    // Not in CK_DEFINE_CONSTRUCTORS: set via Set_SubSmIdentity so the 4-arg construction sites and the
    // root-level path stay untouched.
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true))
    TArray<FGameplayTag> _SubSmIdentity;

public:
    CK_PROPERTY(_PreviousStateClass);
    CK_PROPERTY(_NewStateClass);
    CK_PROPERTY(_Seq);
    CK_PROPERTY(_NewStateFingerprint);
    CK_PROPERTY(_SubSmIdentity);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sm_TransitionEvent, _PreviousStateClass, _NewStateClass, _Seq, _NewStateFingerprint);
};

// --------------------------------------------------------------------------------------------------------------------
