#include "CkDebugDraw_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DebugDraw_UE::
    Create_ASCII_ProgressBar(
        const FCk_FloatRange_0to1& InProgressValue,
        int32                      InProgressBarCharacterLength)
    -> FString
{
    CK_ENSURE_IF_NOT(InProgressBarCharacterLength > 0, TEXT("ASCII ProgressBar character length needs to be > 0"))
    { return {}; }

    FStringBuilderBase StringBuilder;

    const auto& progressValue = InProgressValue.Get_Value();
    const auto& numberOfCharacters = FMath::RoundToInt(progressValue * static_cast<float>(InProgressBarCharacterLength));

    for (auto i = 0; i < InProgressBarCharacterLength; ++i)
    {
        if (i < numberOfCharacters)
        {
            StringBuilder.Append(TEXT("▯"));
        }
        else
        {
            StringBuilder.Append(TEXT(" "));
        }
    }

    return StringBuilder.ToString();
}

// --------------------------------------------------------------------------------------------------------------------
