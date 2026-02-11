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
        return OnValidateCharacter.Execute(InChar);
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
        if (IsAllowedCharacter(Char))
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