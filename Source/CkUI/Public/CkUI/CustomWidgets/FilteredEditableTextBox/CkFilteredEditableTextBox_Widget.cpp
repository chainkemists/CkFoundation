#include "CkFilteredEditableTextBox_Widget.h"

#include "CkUI/UserWidget/CkUserWidget.h"

// --------------------------------------------------------------------------------------------------------------------

UCk_FilteredEditableTextBox::
    UCk_FilteredEditableTextBox()
{
    OnTextChanged.AddUniqueDynamic(this, &ThisClass::HandleTextChanged);
}

auto
    UCk_FilteredEditableTextBox::
    IsAllowedCharacter(
        TCHAR InChar) const
    -> bool
{
    if (OnValidateCharacter.IsBound())
    {
        if (!OnValidateCharacter.Execute(InChar))
        {
            return false;
        }
    }

    if (OnValidateCharacter_BP.IsBound())
    {
        return OnValidateCharacter_BP.Execute(FString::Chr(InChar));
    }

    return true;
}

void
    UCk_FilteredEditableTextBox::
    HandleTextChanged(
        const FText& InText)
{
    if (_IsFiltering)
    { return; }

    const auto& Original = InText.ToString();

    FString Filtered;
    FString Rejected;
    Filtered.Reserve(Original.Len());

    for (const TCHAR Char : Original)
    {
        if (_EnforceMaxLength && Filtered.Len() >= _MaxLength)
        {
            Rejected.AppendChar(Char);
        }
        else if (IsAllowedCharacter(Char))
        {
            Filtered.AppendChar(Char);
        }
        else
        {
            Rejected.AppendChar(Char);
        }
    }

    if (Rejected.Len() > 0)
    {
        _IsFiltering = true;
        SetText(FText::FromString(Filtered));
        _IsFiltering = false;

        OnTextFiltered.Broadcast(Rejected);
    }
}

void
    UCk_FilteredEditableTextBox::
    Set_MaxLength(
        int32 InMaxLength)
{
    if (InMaxLength > 0)
    {
        _EnforceMaxLength = true;
        _MaxLength = InMaxLength;
    }
    else
    {
        _EnforceMaxLength = false;
        _MaxLength = 0;
    }
}

auto
    UCk_FilteredEditableTextBox::
    Get_MaxLength() const
    -> TOptional<int32>
{
    if (_EnforceMaxLength)
    {
        return _MaxLength;
    }

    return {};
}

bool
    UCk_FilteredEditableTextBox::
    Get_MaxLength_BP(
        int32& OutMaxLength) const
{
    OutMaxLength = _MaxLength;
    return _EnforceMaxLength;
}

// --------------------------------------------------------------------------------------------------------------------

#if WITH_EDITOR
auto
    UCk_FilteredEditableTextBox::
    GetPaletteCategory()
    -> const FText
{
    return ck::widget_palette_categories::Default;
}
#endif

// --------------------------------------------------------------------------------------------------------------------