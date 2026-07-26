#pragma once

#include "CkCore/Format/CkFormat.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"

#include <GameplayTags.h>

#include "CkDialogLine_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Dialog_ConditionResult : uint8
{
    Pass,
    Fail
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Dialog_ConditionResult);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKDIALOG_API FCk_Handle_DialogLine : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_DialogLine); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_DialogLine);

// --------------------------------------------------------------------------------------------------------------------

// Built by the Dialog registry subsystem from bank rows (see UCk_DialogRegistry_Subsystem_UE) — not hand-authored on
// an entity. The module's only line payload is _Text; everything else is identity/routing/filtering.
USTRUCT(BlueprintType)
struct CKDIALOG_API FCk_Fragment_DialogLine_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_DialogLine_ParamsData);

private:
    // Identity + debugger label; unique across all registered banks (duplicate LineIDs are rejected at registration).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _LineID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FText _Text;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Ck.Dialog.Event"))
    FGameplayTag _EventTag;

    // Optional link to the next event this line chains to (via Request_QueryFollowUp); invalid/empty == "no link".
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Ck.Dialog.Event"))
    FGameplayTag _LinkedEventTag;

    // Merged bank tags + per-line extra tags. Empty == a global line (visible to every emitter).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _FilterTags;

    // Cached at registration for the query sort policy (avoids re-counting the condition record per query).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _NumConditions = 0;

public:
    CK_PROPERTY_GET(_LineID);
    CK_PROPERTY(_Text);
    CK_PROPERTY_GET(_EventTag);
    CK_PROPERTY(_LinkedEventTag);
    CK_PROPERTY(_FilterTags);
    CK_PROPERTY(_NumConditions);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_DialogLine_ParamsData, _LineID, _EventTag);
};

// --------------------------------------------------------------------------------------------------------------------
