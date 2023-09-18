#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"

#include "CkComparison.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Comparison_Float
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Comparison_Float);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ComparisonOperators _Operator = ECk_ComparisonOperators::EqualTo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _RHS = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Tolerance = KINDA_SMALL_NUMBER;

public:
    CK_PROPERTY_GET(_Operator);
    CK_PROPERTY_GET(_RHS);
    CK_PROPERTY_GET(_Tolerance);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Comparison_Float, _Operator, _RHS, _Tolerance);
};

auto CKCORE_API GetTypeHash(const FCk_Comparison_Float& InA) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Comparison_FloatRange
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Comparison_FloatRange);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _LHS = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ComparisonOperators _OperatorLHS = ECk_ComparisonOperators::GreaterThanOrEqualTo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Logic_And_Or _Logic = ECk_Logic_And_Or::And;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ComparisonOperators _OperatorRHS = ECk_ComparisonOperators::GreaterThanOrEqualTo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _RHS = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    float _Tolerance = KINDA_SMALL_NUMBER;

public:
    CK_PROPERTY_GET(_LHS);
    CK_PROPERTY_GET(_OperatorLHS);
    CK_PROPERTY_GET(_Logic);
    CK_PROPERTY_GET(_OperatorRHS);
    CK_PROPERTY_GET(_RHS);
    CK_PROPERTY_GET(_Tolerance);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Comparison_FloatRange, _LHS, _OperatorLHS, _Logic, _OperatorRHS, _RHS, _Tolerance);
};

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Comparison_Int
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Comparison_Int);

public:
    auto operator==(const ThisType& InOther) const -> bool;
    CK_DECL_AND_DEF_OPERATOR_NOT_EQUAL(ThisType);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ComparisonOperators        _Operator = ECk_ComparisonOperators::EqualTo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32                          _RHS = 0;

public:
    CK_PROPERTY_GET(_Operator);
    CK_PROPERTY_GET(_RHS);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Comparison_Int, _Operator, _RHS);
};

auto CKCORE_API GetTypeHash(const FCk_Comparison_Int& InA) -> uint32;

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Comparison_IntRange
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(FCk_Comparison_IntRange);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _LHS = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ComparisonOperators _OperatorLHS = ECk_ComparisonOperators::GreaterThanOrEqualTo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_Logic_And_Or _Logic = ECk_Logic_And_Or::And;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    ECk_ComparisonOperators _OperatorRHS = ECk_ComparisonOperators::GreaterThanOrEqualTo;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
              meta = (AllowPrivateAccess = true))
    int32 _RHS = 0;

public:
    CK_PROPERTY_GET(_LHS);
    CK_PROPERTY_GET(_OperatorLHS);
    CK_PROPERTY_GET(_Logic);
    CK_PROPERTY_GET(_OperatorRHS);
    CK_PROPERTY_GET(_RHS);

public:
    CK_DEFINE_CONSTRUCTORS(FCk_Comparison_IntRange, _LHS, _OperatorLHS, _Logic, _OperatorRHS, _RHS);
};

// --------------------------------------------------------------------------------------------------------------------
