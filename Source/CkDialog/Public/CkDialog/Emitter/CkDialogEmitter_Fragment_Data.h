#pragma once

#include "CkCore/Format/CkFormat.h"
#include "CkCore/Time/CkTime.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_Typesafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkDialog/Line/CkDialogLine_Fragment_Data.h"

#include <GameplayTags.h>

#include "CkDialogEmitter_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Dialog_QuerySortPolicy : uint8
{
    // Registration order (stable). Also the request-level sentinel for "inherit the emitter's default policy".
    None,
    ConditionCountAscending,
    ConditionCountDescending
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Dialog_QuerySortPolicy);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_DialogLine_QueryResult : uint8
{
    Passed,
    Fail_LineCondition,
    Fail_EmitterCondition
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_DialogLine_QueryResult);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_Dialog_CooldownDuration : uint8
{
    Timed,
    Forever UMETA(DisplayName = "Forever (Play Once Ever)")
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_Dialog_CooldownDuration);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta=(HasNativeMake, HasNativeBreak))
struct CKDIALOG_API FCk_Handle_DialogEmitter : public FCk_Handle_TypeSafe { GENERATED_BODY()  CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_DialogEmitter); };
CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_DialogEmitter);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKDIALOG_API FCk_Fragment_DialogEmitter_ParamsData
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Fragment_DialogEmitter_ParamsData);

private:
    // The emitter's identity tags; ANY-overlap with a line's filter tags makes that line visible to this emitter.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _EmitterTags;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Dialog_QuerySortPolicy _DefaultSortPolicy = ECk_Dialog_QuerySortPolicy::None;

public:
    CK_PROPERTY(_EmitterTags);
    CK_PROPERTY(_DefaultSortPolicy);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Fragment_DialogEmitter_ParamsData, _EmitterTags);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKDIALOG_API FCk_Request_DialogEmitter_Query : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_DialogEmitter_Query);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_DialogEmitter_Query);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories = "Ck.Dialog.Event"))
    FGameplayTag _EventTag;

    // None (default) => inherit the emitter's DefaultSortPolicy.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Dialog_QuerySortPolicy _SortPolicy = ECk_Dialog_QuerySortPolicy::None;

    // Optional per-request narrowing: when non-empty, a visible line must ALSO overlap these tags.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FGameplayTagContainer _ExtraFilterTags;

public:
    CK_PROPERTY_GET(_EventTag);
    CK_PROPERTY(_SortPolicy);
    CK_PROPERTY(_ExtraFilterTags);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_DialogEmitter_Query, _EventTag);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKDIALOG_API FCk_Request_DialogEmitter_StartCooldown : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_DialogEmitter_StartCooldown);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_DialogEmitter_StartCooldown);

private:
    // The cooldown key is the line ENTITY handle (per design), NOT its LineID.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_DialogLine _Line;

    // Used when _DurationMode == Timed; ignored for Forever.
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Time _Duration;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Dialog_CooldownDuration _DurationMode = ECk_Dialog_CooldownDuration::Timed;

public:
    CK_PROPERTY_GET(_Line);
    CK_PROPERTY_GET(_Duration);
    CK_PROPERTY(_DurationMode);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_DialogEmitter_StartCooldown, _Line, _Duration);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKDIALOG_API FCk_Request_DialogEmitter_ClearCooldown : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_DialogEmitter_ClearCooldown);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_DialogEmitter_ClearCooldown);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_DialogLine _Line;

public:
    CK_PROPERTY_GET(_Line);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Request_DialogEmitter_ClearCooldown, _Line);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKDIALOG_API FCk_Request_DialogEmitter_ClearAllCooldowns : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_DialogEmitter_ClearAllCooldowns);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_DialogEmitter_ClearAllCooldowns);
};

// --------------------------------------------------------------------------------------------------------------------

// One matched line + its per-emitter classification. Denormalized (LineID/Text/LinkedEventTag/NumConditions) so BP/AS
// consumers read the result without a second round-trip through the line handle.
USTRUCT(BlueprintType)
struct CKDIALOG_API FCk_DialogLine_QueryEntry
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_DialogLine_QueryEntry);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FCk_Handle_DialogLine _Line;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    ECk_DialogLine_QueryResult _Result = ECk_DialogLine_QueryResult::Passed;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FName _LineID;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FText _Text;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _LinkedEventTag;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    int32 _NumConditions = 0;

public:
    CK_PROPERTY(_Line);
    CK_PROPERTY(_Result);
    CK_PROPERTY(_LineID);
    CK_PROPERTY(_Text);
    CK_PROPERTY(_LinkedEventTag);
    CK_PROPERTY(_NumConditions);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_DialogLine_QueryEntry, _Line, _Result);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKDIALOG_API FCk_DialogEmitter_QueryResult
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_DialogEmitter_QueryResult);

private:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    FGameplayTag _EventTag;

    // ALL matched (visible) lines, each classified. Empty is a valid answer (the query matched nothing visible).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly,
              meta = (AllowPrivateAccess = true))
    TArray<FCk_DialogLine_QueryEntry> _Entries;

public:
    CK_PROPERTY(_EventTag);
    CK_PROPERTY(_Entries);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_DialogEmitter_QueryResult, _EventTag);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_DialogEmitter_OnQueryCompleted,
    FCk_Handle_DialogEmitter, InEmitter,
    FCk_DialogEmitter_QueryResult, InResult);

// --------------------------------------------------------------------------------------------------------------------
