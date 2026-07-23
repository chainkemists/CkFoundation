#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkDialog/Condition/CkDialogCondition_EntityScript.h"

#include "Engine/DataAsset.h"

#include <GameplayTags.h>

#include "CkDialogBank_DataAsset.generated.h"

// --------------------------------------------------------------------------------------------------------------------

// One authored dialogue line inside a bank. The module's only payload is _Text; everything else is
// identity / event routing / tag filtering / condition gating.
USTRUCT(BlueprintType)
struct CKDIALOG_API FCk_DialogBank_LineData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_DialogBank_LineData);

private:
    // Unique across ALL registered banks (duplicates are rejected at registration).
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FName _LineID;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, MultiLine = true))
    FText _Text;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Ck.Dialog.Event"))
    FGameplayTag _EventTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Ck.Dialog.Event"))
    FGameplayTag _LinkedEventTag;

    // Per-line additions on top of the owning bank's _BankTags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _ExtraTags;

    // Instanced per-line condition archetypes (e.g. RandomChance(0.3)); spawned once as long-lived child
    // condition entities of the line at registration. Works because UCk_EntityScript_UE is EditInlineNew.
    UPROPERTY(EditAnywhere, Instanced, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    TArray<TObjectPtr<UCk_DialogCondition_EntityScript>> _Conditions;

public:
    CK_PROPERTY_GET(_LineID);
    CK_PROPERTY(_Text);
    CK_PROPERTY_GET(_EventTag);
    CK_PROPERTY(_LinkedEventTag);
    CK_PROPERTY(_ExtraTags);
    CK_PROPERTY(_Conditions);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_DialogBank_LineData, _LineID, _EventTag);
};

// --------------------------------------------------------------------------------------------------------------------

// A hand-authored (or AS-authored via `asset MyBank of UCk_DialogBank_DataAsset`) set of dialogue lines that share
// a common tag set. Registered into the world's Dialog registry either from project settings or at runtime.
UCLASS(BlueprintType)
class CKDIALOG_API UCk_DialogBank_DataAsset : public UDataAsset
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_DialogBank_DataAsset);

private:
    // Applied (merged) to every line in this bank as filter tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _BankTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, TitleProperty = "_LineID"))
    TArray<FCk_DialogBank_LineData> _Lines;

public:
    CK_PROPERTY_GET(_BankTags);
    CK_PROPERTY_GET(_Lines);
};

// --------------------------------------------------------------------------------------------------------------------
