#include "CkDebugDraw_Utils.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Validation/CkIsValid.h"

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_DebugDraw_UE::
    Create_ASCII_ProgressBar(
        const FCk_FloatRange_0to1&  InProgressValue,
        const int32                 InProgressBarCharacterLength,
        ECk_ASCII_ProgressBar_Style InStyle)
    -> FString
{
    CK_ENSURE_IF_NOT(InProgressBarCharacterLength > 0, TEXT("ASCII ProgressBar character length needs to be > 0"))
    { return {}; }

    FStringBuilderBase StringBuilder;

    const auto& ProgressValue = InProgressValue.Get_Value();
    const auto& NumberOfCharacters = FMath::RoundToInt(ProgressValue * static_cast<float>(InProgressBarCharacterLength));

    const auto& ProgressCharacter = [&]() -> FString
    {
        switch (InStyle)
        {
            case ECk_ASCII_ProgressBar_Style::Equal_Symbol:
            {
                return TEXT("=");
            }
            case ECk_ASCII_ProgressBar_Style::HashTag_Symbol:
            {
                return TEXT("#");
            }
            case ECk_ASCII_ProgressBar_Style::FilledBlock_Symbol:
            {
                return TEXT("|");
            }
            default:
            {
                CK_INVALID_ENUM(InStyle);
                return TEXT("|");
            }
        }
    }();

    for (auto Index = 0; Index < InProgressBarCharacterLength; ++Index)
    {
        if (Index < NumberOfCharacters)
        {
            StringBuilder.Append(ProgressCharacter);
        }
        else
        {
            StringBuilder.Append(TEXT(" "));
        }
    }

    return StringBuilder.ToString();
}

// --------------------------------------------------------------------------------------------------------------------
