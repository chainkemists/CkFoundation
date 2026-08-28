#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "UObject/WeakObjectPtr.h"

#include "CkSnapshot_PersistentEntityMutation.generated.h"

// --------------------------------------------------------------------------------------------------------------------

class UCk_Snapshot_Subsystem_UE;
class UCk_Utils_Snapshot_UE;

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_PersistentEntityMutationResult : uint8
{
    Succeeded,
    Failed_InvalidSource,
    Failed_InvalidDestination,
    Failed_InvalidTicket,
    Failed_InvalidSaveKey,
    Failed_StaleTicket,
    Failed_WorldUnavailable,
    Failed_SnapshotBusy,
    Failed_NotAuthority,
    Failed_WrongWorld,
    Failed_WrongOperationKind,
    Failed_AlreadyTerminal,
    Failed_SourceAlreadyReserved,
    Failed_DestinationAlreadyReserved,
    Failed_SourceIdentityChanged,
    Failed_KeyedNonAuthoredSource,
    Failed_RelocationRequiresAuthoredSource,
    Failed_SharedSaveKeySource,
    Failed_DuplicateLivePublisher,
    Failed_DestinationAlreadyKeyed,
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_PersistentEntityMutationResult);

// --------------------------------------------------------------------------------------------------------------------

/**
 * One-shot cross-authority reference for an entity that may participate in a persistent mutation.
 *
 * The ordinary network handle remains the fast path. A private SaveKey fallback lets a level-authored entity
 * resolve in the authority's world when that entity has no directly serializable replication driver. Gameplay
 * never receives either representation and must resolve the value through UCk_Utils_Snapshot_UE.
 *
 * This is transport state, not durable snapshot state. Do not store it in a snapshotable fragment.
 */
USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_PersistentEntityAuthorityReference
{
    GENERATED_BODY()

private:
    UPROPERTY()
    FCk_Handle _NetworkHandle;

    UPROPERTY()
    FGuid _AuthoredSaveKey;

    friend UCk_Utils_Snapshot_UE;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKSNAPSHOT_API FCk_PersistentEntityMutationTicket
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_PersistentEntityMutationTicket);

    CK_PROPERTY_GET(_BeginResult);

private:
    UPROPERTY(Transient)
    FGuid _OperationId;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Transient,
              Category = "Ck|Snapshot", meta = (AllowPrivateAccess = "true"))
    ECk_PersistentEntityMutationResult _BeginResult = ECk_PersistentEntityMutationResult::Failed_InvalidTicket;

    UPROPERTY(Transient)
    TWeakObjectPtr<UCk_Snapshot_Subsystem_UE> _OwningSubsystem;

    friend UCk_Snapshot_Subsystem_UE;
    friend UCk_Utils_Snapshot_UE;
};
