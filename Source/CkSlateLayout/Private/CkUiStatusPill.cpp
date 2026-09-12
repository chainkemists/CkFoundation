#include "CkSlateLayout/CkUiStatusPill.h"

#include "Brushes/SlateRoundedBoxBrush.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Text/STextBlock.h"

namespace ck_ui_status_pill
{
    struct FConfiguration
    {
        TAttribute<FText> Label;
        TAttribute<FLinearColor> Foreground;
        TAttribute<FLinearColor> Outline;
        FSlateFontInfo Font;
        float Radius = 9.0f;
        float OutlineWidth = 1.0f;
        FMargin Padding = FMargin(6.0f, 2.0f);
    };

    auto ReadLength(const FCkUiStyle& InStyle, const TCHAR* InName, const float InDefault, float& OutValue, FString& OutFailure) -> bool
    {
        const FCkUiCustomStyleValue* Value = InStyle.CustomProperties.Find(InName);
        if (Value == nullptr)
        {
            OutValue = InDefault;
            return true;
        }
        if (!FMath::IsFinite(Value->Number) || Value->Number < 0.0f)
        {
            OutFailure = FString::Printf(TEXT("Status pill style '%s' must be a finite nonnegative length."), InName);
            return false;
        }
        OutValue = Value->Number;
        return true;
    }

    auto MakeFont(const FCkUiCustomWidgetArguments& InArguments, FSlateFontInfo& OutFont, FString& OutFailure) -> bool
    {
        OutFont = InArguments.BaseFont;
        float FontSize = static_cast<float>(OutFont.Size);
        if (!ReadLength(InArguments.Style, TEXT("-ck-status-pill-font-size"), FontSize, FontSize, OutFailure))
        {
            return false;
        }
        OutFont.Size = FMath::Max(1, FMath::RoundToInt(FontSize));
        return true;
    }

    auto MakeConfiguration(const FCkUiCustomWidgetArguments& InArguments, FConfiguration& OutConfiguration, FString& OutFailure) -> bool
    {
        const TAttribute<FText>* Label = InArguments.TextBindings.Find(TEXT("label"));
        const TAttribute<FLinearColor>* Foreground = InArguments.ColorBindings.Find(TEXT("foreground"));
        const TAttribute<FLinearColor>* Outline = InArguments.ColorBindings.Find(TEXT("outline"));
        if (Label == nullptr || !Label->IsSet() || Foreground == nullptr || !Foreground->IsSet() || Outline == nullptr || !Outline->IsSet())
        {
            OutFailure = TEXT("Status pill requires label, foreground, and outline bindings.");
            return false;
        }
        if (!MakeFont(InArguments, OutConfiguration.Font, OutFailure)) { return false; }
        if (!ReadLength(InArguments.Style, TEXT("-ck-status-pill-radius"), 9.0f, OutConfiguration.Radius, OutFailure)
            || !ReadLength(InArguments.Style, TEXT("-ck-status-pill-outline-width"), 1.0f, OutConfiguration.OutlineWidth, OutFailure))
        { return false; }
        float PaddingX = 6.0f;
        float PaddingY = 2.0f;
        if (!ReadLength(InArguments.Style, TEXT("-ck-status-pill-padding-x"), PaddingX, PaddingX, OutFailure)
            || !ReadLength(InArguments.Style, TEXT("-ck-status-pill-padding-y"), PaddingY, PaddingY, OutFailure))
        { return false; }
        OutConfiguration.Label = *Label;
        OutConfiguration.Foreground = *Foreground;
        OutConfiguration.Outline = *Outline;
        OutConfiguration.Padding = FMargin(PaddingX, PaddingY);
        return true;
    }

    auto MakeWidget(const FString& InId, const FConfiguration& InConfiguration) -> TSharedRef<SWidget>
    {
        const TSharedRef<FSlateRoundedBoxBrush> Brush = MakeShared<FSlateRoundedBoxBrush>(
            FLinearColor::Transparent, InConfiguration.Radius, FSlateColor(FLinearColor::White), InConfiguration.OutlineWidth);
        return SNew(SBorder)
            .Tag(FName(*InId))
            .BorderImage_Lambda([Brush, Outline = InConfiguration.Outline]()
            {
                Brush->OutlineSettings.Color = FSlateColor(Outline.Get(FLinearColor::White));
                return static_cast<const FSlateBrush*>(&Brush.Get());
            })
            .BorderBackgroundColor(FLinearColor::White)
            .Padding(InConfiguration.Padding)
            [
                SNew(STextBlock)
                .Text(InConfiguration.Label)
                .Font(InConfiguration.Font)
                .ColorAndOpacity_Lambda([Foreground = InConfiguration.Foreground]()
                { return FSlateColor(Foreground.Get(FLinearColor::White)); })
            ];
    }
}

auto FCkUiStatusPill::Register(FCkUiWidgetRegistry& InRegistry) -> FCkUiLoadResult
{
    auto Registration = FCkUiCustomWidgetRegistration{};
    Registration.Schema.Tag = TEXT("status-pill");
    Registration.Schema.StyleProperties = {
        {TEXT("-ck-status-pill-radius"), ECkUiCustomStyleKind::Length},
        {TEXT("-ck-status-pill-outline-width"), ECkUiCustomStyleKind::Length},
        {TEXT("-ck-status-pill-padding-x"), ECkUiCustomStyleKind::Length},
        {TEXT("-ck-status-pill-padding-y"), ECkUiCustomStyleKind::Length},
        {TEXT("-ck-status-pill-font-size"), ECkUiCustomStyleKind::Length}
    };
    Registration.Schema.Properties = {
        {TEXT("label"), ECkUiCustomPropertyKind::TextBinding, true},
        {TEXT("foreground"), ECkUiCustomPropertyKind::ColorBinding, true},
        {TEXT("outline"), ECkUiCustomPropertyKind::ColorBinding, true}
    };
    Registration.Factory = [](const FCkUiCustomWidgetArguments& InArguments, FString& OutFailure) -> TSharedPtr<SWidget>
    {
        auto Configuration = ck_ui_status_pill::FConfiguration{};
        if (!ck_ui_status_pill::MakeConfiguration(InArguments, Configuration, OutFailure)) { return {}; }
        return ck_ui_status_pill::MakeWidget(InArguments.Id, Configuration);
    };
    return InRegistry.Register(MoveTemp(Registration));
}
