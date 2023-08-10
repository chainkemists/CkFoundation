#pragma once
#include "CkFormat/CkFormat.h"

#include "CkMacros/CkMacros.h"

#include "CkTime.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Time
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Time);

public:
    FCk_Time() = default;
    explicit FCk_Time(float InSeconds);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    auto operator<(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATORS(ThisType);

    auto operator+(ThisType InOther) const -> ThisType;
    auto operator-(ThisType InOther) const -> ThisType;
    auto operator*(ThisType InOther) const -> ThisType;
    auto operator/(ThisType InOther) const -> ThisType;
    CK_DECL_AND_DEF_ADD_SUBTRACT_ASSIGNMENT_OPERATORS(ThisType);

public:
    auto Get_Milliseconds() const -> float;

public:
    static const FCk_Time Zero;

private:
    UPROPERTY(EditAnywhere, BlueprintReadOnly,
        meta = (AllowPrivateAccess = true, Units = "Seconds"))
    float _Seconds = 0.0f;

public:
    CK_PROPERTY_GET(_Seconds);
};

// --------------------------------------------------------------------------------------------------------------------

CK_DEFINE_CUSTOM_FORMATTER(FCk_Time, [&]()
{
    return ck::Format(TEXT("{.3f}s"), InObj.Get_Seconds());
});

// --------------------------------------------------------------------------------------------------------------------
