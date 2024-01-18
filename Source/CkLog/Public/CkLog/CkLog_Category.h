#pragma once

#include "CkLog_Category.generated.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck::layout
{
    class FLogCategory_Details;
}

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, meta = (HasNativeMake))
struct CKLOG_API FCk_LogCategory
{
    GENERATED_BODY()

public:
    friend class ck::layout::FLogCategory_Details;

public:
    FCk_LogCategory() = default;
    explicit FCk_LogCategory(
        FName InName);

public:
    auto IsValid() const -> bool;

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
              Category = "Logging", meta = (AllowPrivateAccess))
    FName _Name;

public:
    auto Get_Name() const -> FName;
};

// --------------------------------------------------------------------------------------------------------------------
