#pragma once

#include "CkCore/Enums/CkEnums.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Math/ValueRange/CkValueRange.h"
#include "CkCore/Time/CkTime.h"

#include <Engine/CurveTable.h>
#include <Kismet/BlueprintFunctionLibrary.h>

#include "CkCurveTable_Utils.generated.h"

// --------------------------------------------------------------------------------------------------------------------

UCLASS(NotBlueprintable, Meta = (ScriptMixin = "FCurveTableRowHandle"))
class CKCORE_API UCk_Utils_CurveTable_UE : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    CK_GENERATED_BODY(UCk_Utils_CurveTable_UE);

public:
    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CurveTable",
              DisplayName = "[Ck] Get Value At Time (CurveTable)",
              meta = (ExpandEnumAsExecs="OutResult"))
    static float
    Get_ValueAtTime(
        const FCurveTableRowHandle& InCurveTableRowHandle,
        const FCk_Time& InTime,
        ECk_RowFoundOrNot& OutResult);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CurveTable",
              DisplayName = "[Ck] Get Time Range (CurveTable)",
              meta = (ExpandEnumAsExecs="OutResult"))
    static FCk_FloatRange
    Get_TimeRange(
        const FCurveTableRowHandle& InCurveTableRowHandle,
        ECk_RowFoundOrNot& OutResult);

    UFUNCTION(BlueprintCallable,
              Category = "Ck|Utils|CurveTable",
              DisplayName = "[Ck] Get Value Range (CurveTable)",
              meta = (ExpandEnumAsExecs="OutResult"))
    static FCk_FloatRange
    Get_ValueRange(
        const FCurveTableRowHandle& InCurveTableRowHandle,
        ECk_RowFoundOrNot& OutResult);

    UFUNCTION(BlueprintPure,
              Category = "Ck|Utils|CurveTable",
              DisplayName = "[Ck] Get Is Valid (CurveTableRowHandle)",
              meta = (CompactNodeTitle = "IsValid"))
    static bool
    Get_IsValid_RowHandle(
        const FCurveTableRowHandle& InCurveTableRowHandle);

public:
    static auto
    Get_ValueAtTime(
        const FCurveTableRowHandle& InCurveTableRowHandle,
        const FCk_Time& InTime) -> TOptional<float>;

    static auto
    Get_TimeRange(
        const FCurveTableRowHandle& InCurveTableRowHandle) -> TOptional<FCk_FloatRange>;

    static auto
    Get_ValueRange(
        const FCurveTableRowHandle& InCurveTableRowHandle) -> TOptional<FCk_FloatRange>;
};

// --------------------------------------------------------------------------------------------------------------------
