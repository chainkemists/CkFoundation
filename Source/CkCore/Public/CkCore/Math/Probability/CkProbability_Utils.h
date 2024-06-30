#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"

#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkProbability_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct CKCORE_API FCk_Utils_Probability_RandomIndexByWeight_Result
{
    GENERATED_BODY()

    CK_GENERATED_BODY(FCk_Utils_Probability_RandomIndexByWeight_Result);

private:
    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess))
    int32 _Index = INDEX_NONE;

    UPROPERTY(EditAnywhere, BlueprintReadWrite,
        meta = (AllowPrivateAccess))
    double _RandomNumber = -1.0f;

public:
    CK_PROPERTY_GET(_Index);
    CK_PROPERTY_GET(_RandomNumber);

    CK_DEFINE_CONSTRUCTORS(FCk_Utils_Probability_RandomIndexByWeight_Result, _Index, _RandomNumber);
};

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable)
class CKCORE_API UCk_Utils_Probability_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_Probability_UE);

public:
    UFUNCTION(BlueprintPure,
        Category = "Ck|Utils|Probability",
        DisplayName = "[Ck] Get Random Index by Weight")
    static FCk_Utils_Probability_RandomIndexByWeight_Result
    Get_RandomIndexByWeight(
        const TArray<double>& InWeights,
        const FRandomStream& InRandomStream);

};

// --------------------------------------------------------------------------------------------------------------------
