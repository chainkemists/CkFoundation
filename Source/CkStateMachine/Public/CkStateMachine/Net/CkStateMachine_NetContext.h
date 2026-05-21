#pragma once

#include "CkCore/Macros/CkMacros.h"

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
UENUM(BlueprintType)
enum class ECk_Sm_AuthorityModel : uint8
{
    ServerAuthoritative,         // Default. Server evaluates conditions, drives transitions.
    OwningClientAuthoritative    // Opt-in. Owning client evaluates locally, pushes to server via RPC.
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

public:
    CK_PROPERTY(_PreviousStateClass);
    CK_PROPERTY(_NewStateClass);
    CK_PROPERTY(_Seq);
    CK_PROPERTY(_NewStateFingerprint);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Sm_TransitionEvent, _PreviousStateClass, _NewStateClass, _Seq, _NewStateFingerprint);
};

// --------------------------------------------------------------------------------------------------------------------
