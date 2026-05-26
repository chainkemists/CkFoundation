#pragma once

#include "CkCore/Macros/CkMacros.h"

#include "CkEcs/Handle/CkHandle.h"
#include "CkEcs/Handle/CkHandle_TypeSafe.h"
#include "CkEcs/Request/CkRequest_Data.h"

#include "CkEntityTagQuery_Fragment_Data.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYTAG_API FCk_Handle_EntityTagQuery : public FCk_Handle_TypeSafe
{
    GENERATED_BODY()
    CK_GENERATED_BODY_HANDLE_TYPESAFE(FCk_Handle_EntityTagQuery);
};

CK_DEFINE_CUSTOM_ISVALID_AND_FORMATTER_HANDLE_TYPESAFE(FCk_Handle_EntityTagQuery);

// --------------------------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class ECk_EntityTagQuery_CountMode : uint8
{
    SingleOnly,
    Count,
    All
};

CK_DEFINE_CUSTOM_FORMATTER_ENUM(ECk_EntityTagQuery_CountMode);

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYTAG_API FCk_EntityTagQuery_Requirement
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityTagQuery_Requirement);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories="EntityTag"))
    FName _Tag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    ECk_EntityTagQuery_CountMode _Mode = ECk_EntityTagQuery_CountMode::Count;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true,
                     EditCondition = "_Mode == ECk_EntityTagQuery_CountMode::Count"))
    int32 _Count = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    int32 _MaxAllowedEnsure = 0;

public:
    CK_PROPERTY_GET(_Tag);
    CK_PROPERTY_GET(_Mode);
    CK_PROPERTY_GET(_Count);
    CK_PROPERTY_GET(_MaxAllowedEnsure);

    // Sentinel for `_MaxAllowedEnsure` meaning "no ensure". Pass to `WithEnsure`
    // to explicitly opt out, or rely on it as the default in the factories.
    static constexpr int32 NoEnsure = 0;

    // Hand-rolled (not CK_DEFINE_CONSTRUCTORS) so we can also expose named factories.
    FCk_EntityTagQuery_Requirement() = default;
    FCk_EntityTagQuery_Requirement(
        FName InTag,
        ECk_EntityTagQuery_CountMode InMode,
        int32 InCount,
        int32 InMaxAllowedEnsure);

    static auto
    Single(
        FName InTag) -> FCk_EntityTagQuery_Requirement;

    static auto
    Of(
        FName InTag,
        int32 InCount) -> FCk_EntityTagQuery_Requirement;

    static auto
    All(
        FName InTag) -> FCk_EntityTagQuery_Requirement;

    // Chainable mutator: `Of(n"A", 3).WithEnsure(5)`
    auto
    WithEnsure(
        int32 InMaxAllowed) & -> FCk_EntityTagQuery_Requirement&;

    auto
    WithEnsure(
        int32 InMaxAllowed) && -> FCk_EntityTagQuery_Requirement;
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYTAG_API FCk_EntityTagQuery_Result
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_EntityTagQuery_Result);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FName _Tag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    TArray<FCk_Handle> _Handles;

public:
    CK_PROPERTY_GET(_Tag);
    CK_PROPERTY_GET(_Handles);
    CK_DEFINE_CONSTRUCTORS(FCk_EntityTagQuery_Result, _Tag, _Handles);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYTAG_API FCk_Request_EntityTagQuery_AddRequirement : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityTagQuery_AddRequirement);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_EntityTagQuery_AddRequirement);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = true))
    FCk_EntityTagQuery_Requirement _Requirement;

public:
    CK_PROPERTY_GET(_Requirement);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityTagQuery_AddRequirement, _Requirement);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKENTITYTAG_API FCk_Request_EntityTagQuery_RemoveRequirement : public FCk_Request_Base
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Request_EntityTagQuery_RemoveRequirement);
    CK_REQUEST_DEFINE_DEBUG_NAME(FCk_Request_EntityTagQuery_RemoveRequirement);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true, Categories="EntityTag"))
    FName _Tag;

public:
    CK_PROPERTY_GET(_Tag);
    CK_DEFINE_CONSTRUCTORS(FCk_Request_EntityTagQuery_RemoveRequirement, _Tag);
};

// --------------------------------------------------------------------------------------------------------------------

DECLARE_DYNAMIC_DELEGATE_TwoParams(
    FCk_Delegate_EntityTagQuery_OnSatisfied,
    FCk_Handle_EntityTagQuery, InQuery,
    const TArray<FCk_EntityTagQuery_Result>&, InResults);

// --------------------------------------------------------------------------------------------------------------------
