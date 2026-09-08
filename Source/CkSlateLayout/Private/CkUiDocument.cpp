#include "CkSlateLayout/CkUiDocument.h"
#include "CkSlateLayout/CkUiWidgetRegistry.h"
#include "CkUiWidgetRegistry.h"

#include "Misc/Char.h"
#include "XmlFile.h"

#include <cfloat>

namespace ck_ui_document
{
    constexpr int32 MaxSourceChars = 1024 * 1024;
    constexpr int32 MaxNodes = 512;
    constexpr int32 MaxDepth = 32;

    struct FStyleRule
    {
        FString ClassName;
        TArray<TPair<FString, FString>> Declarations;
    };

    struct FParseState
    {
        const TMap<FString, FString>& Tokens;
        const FString& Source;
        TArray<FString>& Errors;
        const TSharedPtr<const FCkUiWidgetRegistrySnapshot>& CustomRegistry;
        TSet<FString> Ids;
        TSet<FString> Bindings;
        int32 NodeCount = 0;

        auto Error(const FString& InMessage) -> void
        {
            Errors.Add(FString::Printf(TEXT("%s: %s"), *Source, *InMessage));
        }
    };

    auto IsName(const FString& InValue) -> bool
    {
        if (InValue.IsEmpty() || !(FChar::IsAlpha(InValue[0]) || InValue[0] == TEXT('_') || InValue[0] == TEXT('-')))
        { return false; }
        for (const TCHAR Character : InValue)
        {
            if (!(FChar::IsAlnum(Character) || Character == TEXT('_') || Character == TEXT('-')))
            { return false; }
        }
        return true;
    }

    auto IsOnlyWhitespace(const FString& InValue) -> bool
    {
        for (const TCHAR Character : InValue)
        { if (!FChar::IsWhitespace(Character)) { return false; } }
        return true;
    }

    auto ParseFloat(const FString& InRaw, const bool bAllowPx, float& OutValue) -> bool
    {
        auto Value = InRaw.TrimStartAndEnd();
        if (bAllowPx && Value.EndsWith(TEXT("px"), ESearchCase::CaseSensitive))
        { Value.LeftChopInline(2, EAllowShrinking::No); }
        if (Value.IsEmpty()) { return false; }
        auto Position = 0;
        if (Value[Position] == TEXT('+') || Value[Position] == TEXT('-')) { ++Position; }
        auto DigitCount = 0;
        while (Position < Value.Len() && FChar::IsDigit(Value[Position])) { ++Position; ++DigitCount; }
        if (Position < Value.Len() && Value[Position] == TEXT('.'))
        {
            ++Position;
            while (Position < Value.Len() && FChar::IsDigit(Value[Position])) { ++Position; ++DigitCount; }
        }
        if (DigitCount == 0) { return false; }
        if (Position < Value.Len() && (Value[Position] == TEXT('e') || Value[Position] == TEXT('E')))
        {
            ++Position;
            if (Position < Value.Len() && (Value[Position] == TEXT('+') || Value[Position] == TEXT('-'))) { ++Position; }
            const auto ExponentStart = Position;
            while (Position < Value.Len() && FChar::IsDigit(Value[Position])) { ++Position; }
            if (Position == ExponentStart) { return false; }
        }
        if (Position != Value.Len()) { return false; }
        const double Parsed = FCString::Atod(*Value);
        if (!FMath::IsFinite(Parsed) || FMath::Abs(Parsed) > static_cast<double>(FLT_MAX)) { return false; }
        OutValue = static_cast<float>(Parsed);
        return FMath::IsFinite(OutValue);
    }

    auto ResolveToken(const FString& InValue, const TMap<FString, FString>& InTokens, TSet<FString>& InResolving, FString& OutValue) -> bool
    {
        const auto Value = InValue.TrimStartAndEnd();
        if (!Value.StartsWith(TEXT("var(")) || !Value.EndsWith(TEXT(")")))
        { OutValue = Value; return true; }
        const auto Name = Value.Mid(4, Value.Len() - 5).TrimStartAndEnd();
        if (!Name.StartsWith(TEXT("--")) || InResolving.Contains(Name) || InResolving.Num() >= MaxDepth) { return false; }
        const auto* Token = InTokens.Find(Name);
        if (Token == nullptr) { return false; }
        InResolving.Add(Name);
        const auto Result = ResolveToken(*Token, InTokens, InResolving, OutValue);
        InResolving.Remove(Name);
        return Result;
    }

    auto ResolveValue(const FString& InValue, const TMap<FString, FString>& InTokens, FString& OutValue) -> bool
    {
        auto Resolving = TSet<FString>{};
        return ResolveToken(InValue, InTokens, Resolving, OutValue);
    }

    /** Padding is the only supported shorthand whose components may independently reference tokens. */
    auto ResolvePaddingValue(const FString& InValue, const TMap<FString, FString>& InTokens, FString& OutValue) -> bool
    {
        auto Parts = TArray<FString>{};
        auto Current = FString{};
        auto ParenthesisDepth = 0;
        const auto FlushPart = [&Parts, &Current]() -> bool
        {
            if (Current.IsEmpty()) { return true; }
            if (Parts.Num() >= 4) { return false; }
            Parts.Add(MoveTemp(Current));
            Current.Reset();
            return true;
        };
        for (const TCHAR Character : InValue)
        {
            if (Character == TEXT('(')) { ++ParenthesisDepth; Current.AppendChar(Character); continue; }
            if (Character == TEXT(')'))
            {
                if (ParenthesisDepth <= 0) { return false; }
                --ParenthesisDepth;
                Current.AppendChar(Character);
                continue;
            }
            if (FChar::IsWhitespace(Character) && ParenthesisDepth == 0)
            {
                if (!FlushPart()) { return false; }
                continue;
            }
            Current.AppendChar(Character);
        }
        if (ParenthesisDepth != 0 || !FlushPart()) { return false; }
        if (Parts.IsEmpty()) { return false; }
        auto ResolvedParts = TArray<FString>{};
        ResolvedParts.Reserve(Parts.Num());
        auto ResolvedLength = 0;
        for (const FString& Part : Parts)
        {
            FString Resolved;
            if (!ResolveValue(Part, InTokens, Resolved)) { return false; }
            if (Resolved.Len() > MaxSourceChars - ResolvedLength - (ResolvedParts.IsEmpty() ? 0 : 1)) { return false; }
            ResolvedLength += Resolved.Len() + (ResolvedParts.IsEmpty() ? 0 : 1);
            ResolvedParts.Add(MoveTemp(Resolved));
        }
        OutValue = FString::Join(ResolvedParts, TEXT(" "));
        return true;
    }

    auto ParseColor(const FString& InRaw, FLinearColor& OutColor) -> bool
    {
        auto Value = InRaw.TrimStartAndEnd();
        if (!Value.StartsWith(TEXT("#")) || (Value.Len() != 7 && Value.Len() != 9)) { return false; }
        const auto Hex = [](const TCHAR Character) -> int32
        {
            if (Character >= TEXT('0') && Character <= TEXT('9')) { return Character - TEXT('0'); }
            if (Character >= TEXT('a') && Character <= TEXT('f')) { return Character - TEXT('a') + 10; }
            if (Character >= TEXT('A') && Character <= TEXT('F')) { return Character - TEXT('A') + 10; }
            return -1;
        };
        const auto ByteAt = [&Value, &Hex](const int32 Index) -> int32
        {
            const auto High = Hex(Value[Index]);
            const auto Low = Hex(Value[Index + 1]);
            return High < 0 || Low < 0 ? -1 : High * 16 + Low;
        };
        const auto Red = ByteAt(1); const auto Green = ByteAt(3); const auto Blue = ByteAt(5);
        const auto Alpha = Value.Len() == 9 ? ByteAt(7) : 255;
        if (Red < 0 || Green < 0 || Blue < 0 || Alpha < 0) { return false; }
        OutColor = FLinearColor(FColor(Red, Green, Blue, Alpha));
        return true;
    }

    auto ParseCustomStyleValue(const FString& InRaw, const ECkUiCustomStyleKind InKind, FCkUiCustomStyleValue& OutValue) -> bool
    {
        OutValue.Kind = InKind;
        switch (InKind)
        {
        case ECkUiCustomStyleKind::Number:
            return ParseFloat(InRaw, false, OutValue.Number);
        case ECkUiCustomStyleKind::Length:
            return ParseFloat(InRaw, true, OutValue.Number) && OutValue.Number >= 0.0f;
        case ECkUiCustomStyleKind::Color:
            return ParseColor(InRaw, OutValue.Color);
        default:
            return false;
        }
    }

    auto ParsePadding(const FString& InRaw, FMargin& OutPadding) -> bool
    {
        auto Parts = TArray<FString>{};
        InRaw.ParseIntoArrayWS(Parts);
        if (Parts.Num() != 1 && Parts.Num() != 2 && Parts.Num() != 4) { return false; }
        auto Values = TArray<float>{}; Values.Reserve(Parts.Num());
        for (const auto& Part : Parts)
        {
            float Value = 0.0f;
            if (!ParseFloat(Part, true, Value) || Value < 0.0f) { return false; }
            Values.Add(Value);
        }
        if (Values.Num() == 1) { OutPadding = FMargin(Values[0]); }
        else if (Values.Num() == 2) { OutPadding = FMargin(Values[1], Values[0], Values[1], Values[0]); }
        else { OutPadding = FMargin(Values[3], Values[0], Values[1], Values[2]); }
        return true;
    }

    auto IsSupportedProperty(const FString& InProperty) -> bool
    {
        return InProperty == TEXT("gap") || InProperty == TEXT("padding")
            || InProperty == TEXT("flex-wrap")
            || InProperty == TEXT("flex-grow") || InProperty == TEXT("flex-shrink")
            || InProperty == TEXT("min-width") || InProperty == TEXT("min-height")
            || InProperty == TEXT("max-width") || InProperty == TEXT("max-height")
            || InProperty == TEXT("width") || InProperty == TEXT("height")
            || InProperty == TEXT("font-size") || InProperty == TEXT("font-weight")
            || InProperty == TEXT("text-wrap") || InProperty == TEXT("overflow-wrap") || InProperty == TEXT("text-overflow")
            || InProperty == TEXT("color") || InProperty == TEXT("background-color")
            || InProperty == TEXT("horizontal-align") || InProperty == TEXT("vertical-align");
    }

    auto ParseStylesheet(const FString& InStylesheet, TArray<FStyleRule>& OutRules, TArray<FString>& OutErrors, const FString& InSource) -> bool
    {
        auto Source = InStylesheet;
        int32 CommentStart = Source.Find(TEXT("/*"));
        while (CommentStart != INDEX_NONE)
        {
            const auto CommentEnd = Source.Find(TEXT("*/"), ESearchCase::CaseSensitive, ESearchDir::FromStart, CommentStart + 2);
            if (CommentEnd == INDEX_NONE) { OutErrors.Add(InSource + TEXT(": unterminated CSS comment")); return false; }
            Source.RemoveAt(CommentStart, CommentEnd + 2 - CommentStart);
            CommentStart = Source.Find(TEXT("/*"));
        }
        int32 Position = 0;
        const auto SkipSpace = [&Source, &Position]() -> void { while (Position < Source.Len() && FChar::IsWhitespace(Source[Position])) { ++Position; } };
        while (true)
        {
            SkipSpace();
            if (Position == Source.Len()) { return true; }
            if (Source[Position] != TEXT('.')) { OutErrors.Add(InSource + TEXT(": CSS selector must be a single .class")); return false; }
            ++Position;
            const auto NameStart = Position;
            while (Position < Source.Len() && (FChar::IsAlnum(Source[Position]) || Source[Position] == TEXT('_') || Source[Position] == TEXT('-'))) { ++Position; }
            const auto ClassName = Source.Mid(NameStart, Position - NameStart);
            if (!IsName(ClassName)) { OutErrors.Add(InSource + TEXT(": invalid CSS class selector")); return false; }
            SkipSpace();
            if (Position == Source.Len() || Source[Position++] != TEXT('{')) { OutErrors.Add(InSource + TEXT(": expected { after CSS selector")); return false; }
            auto Rule = FStyleRule{.ClassName = ClassName};
            while (true)
            {
                SkipSpace();
                if (Position == Source.Len()) { OutErrors.Add(InSource + TEXT(": unterminated CSS rule")); return false; }
                if (Source[Position] == TEXT('}')) { ++Position; break; }
                const auto PropertyStart = Position;
                while (Position < Source.Len() && (FChar::IsAlnum(Source[Position]) || Source[Position] == TEXT('-'))) { ++Position; }
                const auto Property = Source.Mid(PropertyStart, Position - PropertyStart);
                SkipSpace();
                if (!IsName(Property) || Position == Source.Len() || Source[Position++] != TEXT(':')) { OutErrors.Add(InSource + TEXT(": invalid CSS declaration")); return false; }
                const auto ValueStart = Position;
                while (Position < Source.Len() && Source[Position] != TEXT(';') && Source[Position] != TEXT('}')) { ++Position; }
                auto Value = Source.Mid(ValueStart, Position - ValueStart).TrimStartAndEnd();
                if (Value.IsEmpty()) { OutErrors.Add(InSource + TEXT(": empty CSS value for ") + Property); return false; }
                Rule.Declarations.Emplace(Property, Value);
                if (Position < Source.Len() && Source[Position] == TEXT(';')) { ++Position; }
                else if (Position < Source.Len() && Source[Position] == TEXT('}')) { ++Position; break; }
                else { OutErrors.Add(InSource + TEXT(": unterminated CSS declaration")); return false; }
            }
            OutRules.Add(MoveTemp(Rule));
        }
    }

    auto ValidateDeclaration(const FString& InProperty, const FString& InRawValue, FParseState& InOutState) -> bool
    {
        const ECkUiCustomStyleKind* CustomKind = InOutState.CustomRegistry.IsValid()
            ? InOutState.CustomRegistry->FindStyleProperty(InProperty) : nullptr;
        if (!IsSupportedProperty(InProperty) && CustomKind == nullptr)
        { InOutState.Error(TEXT("unsupported CSS property '") + InProperty + TEXT("'")); return false; }
        FString Value;
        const bool bResolved = InProperty == TEXT("padding")
            ? ResolvePaddingValue(InRawValue, InOutState.Tokens, Value)
            : ResolveValue(InRawValue, InOutState.Tokens, Value);
        if (!bResolved) { InOutState.Error(TEXT("unresolved or cyclic token in CSS property '") + InProperty + TEXT("'")); return false; }
        if (CustomKind != nullptr)
        {
            FCkUiCustomStyleValue CustomValue;
            if (!ParseCustomStyleValue(Value, *CustomKind, CustomValue))
            { InOutState.Error(TEXT("invalid CSS value for '") + InProperty + TEXT("'")); return false; }
            return true;
        }
        if (InProperty == TEXT("padding")) { FMargin Padding; if (!ParsePadding(Value, Padding)) { InOutState.Error(TEXT("invalid CSS padding")); return false; } return true; }
        if (InProperty == TEXT("color") || InProperty == TEXT("background-color")) { FLinearColor Color; if (!ParseColor(Value, Color)) { InOutState.Error(TEXT("invalid CSS color")); return false; } return true; }
        if (InProperty == TEXT("text-wrap"))
        {
            if (Value != TEXT("wrap") && Value != TEXT("nowrap")) { InOutState.Error(TEXT("invalid CSS text-wrap")); return false; }
            return true;
        }
        if (InProperty == TEXT("flex-wrap"))
        {
            if (Value != TEXT("nowrap") && Value != TEXT("wrap") && Value != TEXT("wrap-reverse")) { InOutState.Error(TEXT("invalid CSS flex-wrap")); return false; }
            return true;
        }
        if (InProperty == TEXT("overflow-wrap"))
        {
            if (Value != TEXT("normal") && Value != TEXT("anywhere")) { InOutState.Error(TEXT("invalid CSS overflow-wrap")); return false; }
            return true;
        }
        if (InProperty == TEXT("text-overflow"))
        {
            if (Value != TEXT("clip") && Value != TEXT("ellipsis")) { InOutState.Error(TEXT("invalid CSS text-overflow")); return false; }
            return true;
        }
        if (InProperty == TEXT("horizontal-align") || InProperty == TEXT("vertical-align") || InProperty == TEXT("font-weight"))
        {
            const auto Valid = InProperty == TEXT("horizontal-align") ? (Value == TEXT("fill") || Value == TEXT("left") || Value == TEXT("center") || Value == TEXT("right"))
                : InProperty == TEXT("vertical-align") ? (Value == TEXT("fill") || Value == TEXT("top") || Value == TEXT("center") || Value == TEXT("bottom"))
                : (Value == TEXT("normal") || Value == TEXT("bold"));
            if (!Valid) { InOutState.Error(TEXT("invalid CSS value for '") + InProperty + TEXT("'")); }
            return Valid;
        }
        const auto IsDimension = InProperty != TEXT("flex-grow") && InProperty != TEXT("flex-shrink");
        float Number = 0.0f;
        const auto IsFontSize = InProperty == TEXT("font-size");
        if (!ParseFloat(Value, IsDimension, Number) || Number < 0.0f || (IsFontSize && (Number < 1.0f || Number > 512.0f)))
        { InOutState.Error(TEXT("invalid CSS numeric value for '") + InProperty + TEXT("'")); return false; }
        return true;
    }

    auto HasExactlyOneXmlRoot(const FString& InMarkup) -> bool
    {
        auto Depth = 0;
        auto bSawRoot = false;
        auto bRootClosed = false;
        for (int32 Position = 0; Position < InMarkup.Len(); ++Position)
        {
            if (InMarkup[Position] != TEXT('<'))
            {
                if (Depth == 0 && !FChar::IsWhitespace(InMarkup[Position])) { return false; }
                continue;
            }
            if (bRootClosed) { return false; }
            auto TagEnd = Position + 1;
            TCHAR Quote = TEXT('\0');
            for (; TagEnd < InMarkup.Len(); ++TagEnd)
            {
                const auto Character = InMarkup[TagEnd];
                if (Quote != TEXT('\0')) { if (Character == Quote) { Quote = TEXT('\0'); } continue; }
                if (Character == TEXT('\"') || Character == TEXT('\'')) { Quote = Character; continue; }
                if (Character == TEXT('>')) { break; }
            }
            if (TagEnd == InMarkup.Len() || Quote != TEXT('\0')) { return false; }
            const auto bEndTag = Position + 1 < InMarkup.Len() && InMarkup[Position + 1] == TEXT('/');
            auto LastContent = TagEnd - 1;
            while (LastContent > Position && FChar::IsWhitespace(InMarkup[LastContent])) { --LastContent; }
            const auto bEmptyTag = !bEndTag && InMarkup[LastContent] == TEXT('/');
            if (bEndTag)
            {
                if (Depth <= 0) { return false; }
                if (--Depth == 0) { bRootClosed = true; }
            }
            else if (bEmptyTag)
            {
                if (Depth == 0)
                {
                    if (bSawRoot) { return false; }
                    bSawRoot = true;
                    bRootClosed = true;
                }
            }
            else
            {
                if (Depth == 0)
                {
                    if (bSawRoot) { return false; }
                    bSawRoot = true;
                }
                if (++Depth > MaxDepth + 2) { return false; }
            }
            Position = TagEnd;
        }
        return bSawRoot && bRootClosed && Depth == 0;
    }

    auto ApplyProperty(FCkUiNode& InOutNode, const FString& InProperty, const FString& InRawValue, FParseState& InOutState) -> bool
    {
        FString Value;
        const bool bResolved = InProperty == TEXT("padding")
            ? ResolvePaddingValue(InRawValue, InOutState.Tokens, Value)
            : ResolveValue(InRawValue, InOutState.Tokens, Value);
        if (!bResolved) { InOutState.Error(FString::Printf(TEXT("node '%s': unresolved or cyclic token in %s"), *InOutNode.Id, *InProperty)); return false; }
        const ECkUiCustomStyleKind* CustomKind = InOutState.CustomRegistry.IsValid()
            ? InOutState.CustomRegistry->FindStyleProperty(InProperty) : nullptr;
        if (CustomKind != nullptr)
        {
            const FCkUiCustomWidgetRegistration* Custom = InOutNode.Kind == ECkUiNodeKind::Custom
                ? InOutState.CustomRegistry->Find(InOutNode.CustomTag) : nullptr;
            const ECkUiCustomStyleKind* DeclaredKind = Custom != nullptr ? Custom->Schema.StyleProperties.Find(InProperty) : nullptr;
            if (DeclaredKind == nullptr || *DeclaredKind != *CustomKind)
            { InOutState.Error(FString::Printf(TEXT("node '%s': property '%s' is not applicable"), *InOutNode.Id, *InProperty)); return false; }
            FCkUiCustomStyleValue CustomValue;
            if (!ParseCustomStyleValue(Value, *CustomKind, CustomValue))
            { InOutState.Error(FString::Printf(TEXT("node '%s': invalid CSS value for '%s'"), *InOutNode.Id, *InProperty)); return false; }
            InOutNode.Style.CustomProperties.Add(InProperty, MoveTemp(CustomValue));
            return true;
        }
        const FCkUiBuiltinWidgetSchema* Schema = ck_ui_widget_registry::FindByKind(InOutNode.Kind);
        const bool bCustom = InOutNode.Kind == ECkUiNodeKind::Custom;
        if (Schema == nullptr && !bCustom) { InOutState.Error(FString::Printf(TEXT("node '%s': unsupported node kind"), *InOutNode.Id)); return false; }
        const auto InvalidForNode = [&InOutState, &InOutNode, &InProperty]() -> bool
        { InOutState.Error(FString::Printf(TEXT("node '%s': property '%s' is not applicable"), *InOutNode.Id, *InProperty)); return false; };
        if (InProperty == TEXT("gap") && (bCustom || !Schema->bAllowsGap)) { return InvalidForNode(); }
        if (InProperty == TEXT("flex-wrap") && InOutNode.Kind != ECkUiNodeKind::Row && InOutNode.Kind != ECkUiNodeKind::Column && InOutNode.Kind != ECkUiNodeKind::Repeat) { return InvalidForNode(); }
        if (InProperty == TEXT("padding") && (bCustom || !Schema->bAllowsPadding)) { return InvalidForNode(); }
        if (InProperty == TEXT("background-color") && (bCustom || !Schema->bAllowsBackground)) { return InvalidForNode(); }
        if ((InProperty == TEXT("font-size") || InProperty == TEXT("font-weight") || InProperty == TEXT("color")) && (bCustom || !Schema->bAllowsTextStyle)) { return InvalidForNode(); }
        if ((InProperty == TEXT("text-wrap") || InProperty == TEXT("overflow-wrap") || InProperty == TEXT("text-overflow")) && InOutNode.Kind != ECkUiNodeKind::Text && InOutNode.Kind != ECkUiNodeKind::Button) { return InvalidForNode(); }
        if (InProperty == TEXT("padding")) { if (!ParsePadding(Value, InOutNode.Style.Padding)) { InOutState.Error(TEXT("invalid padding on '") + InOutNode.Id + TEXT("'")); return false; } return true; }
        if (InProperty == TEXT("text-wrap")) { InOutNode.Style.AllowWrapping = Value == TEXT("wrap"); return true; }
        if (InProperty == TEXT("flex-wrap"))
        {
            if (Value == TEXT("nowrap")) { InOutNode.Style.FlexWrap = ECkFlexWrap::NoWrap; }
            else if (Value == TEXT("wrap")) { InOutNode.Style.FlexWrap = ECkFlexWrap::Wrap; }
            else if (Value == TEXT("wrap-reverse")) { InOutNode.Style.FlexWrap = ECkFlexWrap::WrapReverse; }
            else { InOutState.Error(TEXT("invalid flex-wrap on '") + InOutNode.Id + TEXT("'")); return false; }
            return true;
        }
        if (InProperty == TEXT("overflow-wrap")) { InOutNode.Style.WrappingPolicy = Value == TEXT("anywhere") ? ETextWrappingPolicy::AllowPerCharacterWrapping : ETextWrappingPolicy::DefaultWrapping; return true; }
        if (InProperty == TEXT("text-overflow")) { InOutNode.Style.OverflowPolicy = Value == TEXT("ellipsis") ? ETextOverflowPolicy::Ellipsis : ETextOverflowPolicy::Clip; return true; }
        if (InProperty == TEXT("color") || InProperty == TEXT("background-color"))
        { FLinearColor Color; if (!ParseColor(Value, Color)) { InOutState.Error(TEXT("invalid color on '") + InOutNode.Id + TEXT("'")); return false; } if (InProperty == TEXT("color")) { InOutNode.Style.Color = Color; } else { InOutNode.Style.Background = Color; } return true; }
        if (InProperty == TEXT("horizontal-align") || InProperty == TEXT("vertical-align"))
        {
            if (InProperty == TEXT("horizontal-align"))
            {
                if (Value == TEXT("fill")) { InOutNode.Style.HAlign = HAlign_Fill; } else if (Value == TEXT("left")) { InOutNode.Style.HAlign = HAlign_Left; } else if (Value == TEXT("center")) { InOutNode.Style.HAlign = HAlign_Center; } else if (Value == TEXT("right")) { InOutNode.Style.HAlign = HAlign_Right; } else { InOutState.Error(TEXT("invalid horizontal-align on '") + InOutNode.Id + TEXT("'")); return false; }
            }
            else
            {
                if (Value == TEXT("fill")) { InOutNode.Style.VAlign = VAlign_Fill; } else if (Value == TEXT("top")) { InOutNode.Style.VAlign = VAlign_Top; } else if (Value == TEXT("center")) { InOutNode.Style.VAlign = VAlign_Center; } else if (Value == TEXT("bottom")) { InOutNode.Style.VAlign = VAlign_Bottom; } else { InOutState.Error(TEXT("invalid vertical-align on '") + InOutNode.Id + TEXT("'")); return false; }
            }
            return true;
        }
        if (InProperty == TEXT("font-weight"))
        {
            if (Value == TEXT("normal")) { InOutNode.Style.Bold = false; return true; }
            if (Value == TEXT("bold")) { InOutNode.Style.Bold = true; return true; }
            InOutState.Error(TEXT("invalid font-weight on '") + InOutNode.Id + TEXT("'"));
            return false;
        }
        const auto IsGrowShrink = InProperty == TEXT("flex-grow") || InProperty == TEXT("flex-shrink");
        const auto IsDimension = InProperty == TEXT("gap") || InProperty == TEXT("min-width") || InProperty == TEXT("min-height") || InProperty == TEXT("max-width") || InProperty == TEXT("max-height") || InProperty == TEXT("width") || InProperty == TEXT("height") || InProperty == TEXT("font-size");
        if (!IsGrowShrink && !IsDimension) { InOutState.Error(FString::Printf(TEXT("node '%s': unsupported property '%s'"), *InOutNode.Id, *InProperty)); return false; }
        float Number = 0.0f;
        if (!ParseFloat(Value, IsDimension, Number) || Number < 0.0f || (InProperty == TEXT("font-size") && (Number < 1.0f || Number > 512.0f))) { InOutState.Error(FString::Printf(TEXT("node '%s': invalid numeric value for '%s'"), *InOutNode.Id, *InProperty)); return false; }
        if (InProperty == TEXT("gap")) { InOutNode.Style.Gap = Number; }
        else if (InProperty == TEXT("flex-grow")) { InOutNode.Style.Grow = Number; }
        else if (InProperty == TEXT("flex-shrink")) { InOutNode.Style.Shrink = Number; }
        else if (InProperty == TEXT("min-width")) { InOutNode.Style.MinWidth = Number; }
        else if (InProperty == TEXT("min-height")) { InOutNode.Style.MinHeight = Number; }
        else if (InProperty == TEXT("max-width")) { InOutNode.Style.MaxWidth = Number; }
        else if (InProperty == TEXT("max-height")) { InOutNode.Style.MaxHeight = Number; }
        else if (InProperty == TEXT("width")) { InOutNode.Style.MinWidth = Number; InOutNode.Style.MaxWidth = Number; }
        else if (InProperty == TEXT("height")) { InOutNode.Style.MinHeight = Number; InOutNode.Style.MaxHeight = Number; }
        else { InOutNode.Style.FontSize = Number; }
        return true;
    }

    auto ApplyClasses(FCkUiNode& InOutNode, const FString& InClasses, const TArray<FStyleRule>& InRules, FParseState& InOutState) -> bool
    {
        auto Classes = TArray<FString>{}; InClasses.ParseIntoArrayWS(Classes);
        for (const auto& ClassName : Classes)
        {
            if (!IsName(ClassName)) { InOutState.Error(TEXT("node '") + InOutNode.Id + TEXT("': invalid class '") + ClassName + TEXT("'")); return false; }
            auto bFound = false;
            for (const auto& Rule : InRules) { if (Rule.ClassName == ClassName) { bFound = true; break; } }
            if (!bFound) { InOutState.Error(TEXT("node '") + InOutNode.Id + TEXT("': unknown class '") + ClassName + TEXT("'")); return false; }
        }
        for (const auto& Rule : InRules)
        {
            if (!Classes.Contains(Rule.ClassName)) { continue; }
            for (const auto& Declaration : Rule.Declarations)
            { if (!ApplyProperty(InOutNode, Declaration.Key, Declaration.Value, InOutState)) { return false; } }
        }
        const auto& Style = InOutNode.Style;
        if ((Style.MaxWidth.IsSet() && Style.MaxWidth.GetValue() < Style.MinWidth) || (Style.MaxHeight.IsSet() && Style.MaxHeight.GetValue() < Style.MinHeight))
        { InOutState.Error(TEXT("node '") + InOutNode.Id + TEXT("': maximum is less than minimum")); return false; }
        if ((InOutNode.Kind == ECkUiNodeKind::Text || InOutNode.Kind == ECkUiNodeKind::Button) && Style.AllowWrapping && Style.OverflowPolicy == ETextOverflowPolicy::Ellipsis)
        { InOutState.Error(TEXT("node '") + InOutNode.Id + TEXT("': ellipsis requires text-wrap: nowrap")); return false; }
        return true;
    }

    auto IsBindingProperty(const ECkUiCustomPropertyKind InKind) -> bool
    {
        return InKind == ECkUiCustomPropertyKind::TextBinding || InKind == ECkUiCustomPropertyKind::ImageBinding
            || InKind == ECkUiCustomPropertyKind::NumberBinding || InKind == ECkUiCustomPropertyKind::BoolBinding
            || InKind == ECkUiCustomPropertyKind::StringBinding || InKind == ECkUiCustomPropertyKind::ColorBinding
            || InKind == ECkUiCustomPropertyKind::CollectionBinding;
    }

    enum class EValueKind : uint8
    {
        Text, Number, Bool, Color, TextBinding, ImageBinding, NumberBinding,
        BoolBinding, StringBinding, Action, ColorBinding, TextChanged, TextCommitted, BoolChanged, NumberChanged, NumberCommitted, NumberInteraction, StringChanged, SearchBinding, NativeBinding, CollectionBinding,
    };

    struct FValue
    {
        EValueKind Kind = EValueKind::Text;
        FCkUiCustomPropertyValue Data;
        FString Parameter;
        bool IsReference = false;
    };

    struct FAuthoredNode
    {
        FCkUiNode Prototype;
        TMap<FString, FValue> Fields;
        TArray<FAuthoredNode> Children;
        FString Template;
    };

    struct FTemplate
    {
        TMap<FString, EValueKind> Parameters;
        const FXmlNode* Source = nullptr;
        FAuthoredNode Body;
    };

    struct FSummary
    {
        int32 Count = 0;
        int32 Depth = 0;
        ECkUiNodeKind RootKind = ECkUiNodeKind::Column;
    };

    auto IsBindingKind(const EValueKind InKind) -> bool
    {
        return InKind == EValueKind::TextBinding || InKind == EValueKind::ImageBinding
            || InKind == EValueKind::NumberBinding || InKind == EValueKind::BoolBinding
            || InKind == EValueKind::StringBinding || InKind == EValueKind::ColorBinding
            || InKind == EValueKind::SearchBinding || InKind == EValueKind::NativeBinding || InKind == EValueKind::CollectionBinding;
    }

    auto CustomKind(const ECkUiCustomPropertyKind InKind) -> EValueKind
    {
        switch (InKind)
        {
        case ECkUiCustomPropertyKind::Text: return EValueKind::Text;
        case ECkUiCustomPropertyKind::Number: return EValueKind::Number;
        case ECkUiCustomPropertyKind::Bool: return EValueKind::Bool;
        case ECkUiCustomPropertyKind::Color: return EValueKind::Color;
        case ECkUiCustomPropertyKind::TextBinding: return EValueKind::TextBinding;
        case ECkUiCustomPropertyKind::ImageBinding: return EValueKind::ImageBinding;
        case ECkUiCustomPropertyKind::NumberBinding: return EValueKind::NumberBinding;
        case ECkUiCustomPropertyKind::BoolBinding: return EValueKind::BoolBinding;
        case ECkUiCustomPropertyKind::StringBinding: return EValueKind::StringBinding;
        case ECkUiCustomPropertyKind::Action: return EValueKind::Action;
        case ECkUiCustomPropertyKind::ColorBinding: return EValueKind::ColorBinding;
        case ECkUiCustomPropertyKind::TextChanged: return EValueKind::TextChanged;
        case ECkUiCustomPropertyKind::TextCommitted: return EValueKind::TextCommitted;
        case ECkUiCustomPropertyKind::BoolChanged: return EValueKind::BoolChanged;
        case ECkUiCustomPropertyKind::NumberChanged: return EValueKind::NumberChanged;
        case ECkUiCustomPropertyKind::NumberCommitted: return EValueKind::NumberCommitted;
        case ECkUiCustomPropertyKind::NumberInteraction: return EValueKind::NumberInteraction;
        case ECkUiCustomPropertyKind::StringChanged: return EValueKind::StringChanged;
        case ECkUiCustomPropertyKind::CollectionBinding: return EValueKind::CollectionBinding;
        default: return EValueKind::Text;
        }
    }

    auto BindingKind(const ECkUiBuiltinBindingKind InKind) -> EValueKind
    {
        switch (InKind)
        {
        case ECkUiBuiltinBindingKind::Native: return EValueKind::NativeBinding;
        case ECkUiBuiltinBindingKind::SearchText: return EValueKind::SearchBinding;
        case ECkUiBuiltinBindingKind::Image: return EValueKind::ImageBinding;
        case ECkUiBuiltinBindingKind::Collection: return EValueKind::CollectionBinding;
        case ECkUiBuiltinBindingKind::TreeCollection: return EValueKind::CollectionBinding;
        default: return EValueKind::TextBinding;
        }
    }

    auto ParseValueKind(const FString& InName, EValueKind& OutKind) -> bool
    {
        static const TMap<FString, EValueKind> Kinds = {
            {TEXT("text"), EValueKind::Text}, {TEXT("number"), EValueKind::Number},
            {TEXT("bool"), EValueKind::Bool}, {TEXT("color"), EValueKind::Color},
            {TEXT("text-binding"), EValueKind::TextBinding}, {TEXT("image-binding"), EValueKind::ImageBinding},
            {TEXT("number-binding"), EValueKind::NumberBinding}, {TEXT("bool-binding"), EValueKind::BoolBinding},
            {TEXT("string-binding"), EValueKind::StringBinding}, {TEXT("action"), EValueKind::Action}, {TEXT("text-changed"), EValueKind::TextChanged}, {TEXT("text-committed"), EValueKind::TextCommitted}, {TEXT("bool-changed"), EValueKind::BoolChanged}, {TEXT("number-changed"), EValueKind::NumberChanged}, {TEXT("number-committed"), EValueKind::NumberCommitted}, {TEXT("number-interaction"), EValueKind::NumberInteraction}, {TEXT("string-changed"), EValueKind::StringChanged}, {TEXT("search-binding"), EValueKind::SearchBinding},
            {TEXT("color-binding"), EValueKind::ColorBinding}, {TEXT("native-binding"), EValueKind::NativeBinding}, {TEXT("collection-binding"), EValueKind::CollectionBinding}};
        const EValueKind* Found = Kinds.Find(InName);
        if (Found == nullptr) { return false; }
        OutKind = *Found;
        return true;
    }

    auto CompileValue(const FString& InRaw, const EValueKind InKind, const bool InReference,
        const TMap<FString, EValueKind>* InParameters, FValue& OutValue, FParseState& State) -> bool
    {
        OutValue.Kind = InKind;
        OutValue.IsReference = InReference;
        if (InReference)
        {
            OutValue.Parameter = InRaw.TrimStartAndEnd();
            const EValueKind* ParameterKind = InParameters != nullptr ? InParameters->Find(OutValue.Parameter) : nullptr;
            if (ParameterKind == nullptr || *ParameterKind != InKind)
            { State.Error(TEXT("unknown or type-mismatched template parameter '") + OutValue.Parameter + TEXT("'")); return false; }
            return true;
        }
        switch (InKind)
        {
        case EValueKind::Text: OutValue.Data.Kind = ECkUiCustomPropertyKind::Text; break;
        case EValueKind::Number: OutValue.Data.Kind = ECkUiCustomPropertyKind::Number; break;
        case EValueKind::Bool: OutValue.Data.Kind = ECkUiCustomPropertyKind::Bool; break;
        case EValueKind::Color: OutValue.Data.Kind = ECkUiCustomPropertyKind::Color; break;
        case EValueKind::TextBinding: OutValue.Data.Kind = ECkUiCustomPropertyKind::TextBinding; break;
        case EValueKind::ImageBinding: OutValue.Data.Kind = ECkUiCustomPropertyKind::ImageBinding; break;
        case EValueKind::NumberBinding: OutValue.Data.Kind = ECkUiCustomPropertyKind::NumberBinding; break;
        case EValueKind::BoolBinding: OutValue.Data.Kind = ECkUiCustomPropertyKind::BoolBinding; break;
        case EValueKind::StringBinding: OutValue.Data.Kind = ECkUiCustomPropertyKind::StringBinding; break;
        case EValueKind::Action: OutValue.Data.Kind = ECkUiCustomPropertyKind::Action; break;
        case EValueKind::ColorBinding: OutValue.Data.Kind = ECkUiCustomPropertyKind::ColorBinding; break;
        case EValueKind::TextChanged: OutValue.Data.Kind = ECkUiCustomPropertyKind::TextChanged; break;
        case EValueKind::TextCommitted: OutValue.Data.Kind = ECkUiCustomPropertyKind::TextCommitted; break;
        case EValueKind::BoolChanged: OutValue.Data.Kind = ECkUiCustomPropertyKind::BoolChanged; break;
        case EValueKind::NumberChanged: OutValue.Data.Kind = ECkUiCustomPropertyKind::NumberChanged; break;
        case EValueKind::NumberCommitted: OutValue.Data.Kind = ECkUiCustomPropertyKind::NumberCommitted; break;
        case EValueKind::NumberInteraction: OutValue.Data.Kind = ECkUiCustomPropertyKind::NumberInteraction; break;
        case EValueKind::StringChanged: OutValue.Data.Kind = ECkUiCustomPropertyKind::StringChanged; break;
        case EValueKind::CollectionBinding: OutValue.Data.Kind = ECkUiCustomPropertyKind::CollectionBinding; break;
        default: break;
        }
        switch (InKind)
        {
        case EValueKind::Text: OutValue.Data.Text = FText::FromString(InRaw); return true;
        case EValueKind::Number:
            if (ParseFloat(InRaw, false, OutValue.Data.Number)) { return true; }
            break;
        case EValueKind::Bool:
            if (InRaw == TEXT("true")) { OutValue.Data.Bool = true; return true; }
            if (InRaw == TEXT("false")) { OutValue.Data.Bool = false; return true; }
            break;
        case EValueKind::Color:
            if (ParseColor(InRaw, OutValue.Data.Color)) { return true; }
            break;
        default:
            OutValue.Data.Name = InRaw.TrimStartAndEnd();
            if (IsName(OutValue.Data.Name)) { return true; }
            break;
        }
        State.Error(TEXT("invalid typed literal '") + InRaw.Left(80) + TEXT("'"));
        return false;
    }

    auto HasOnlyAttributes(const FXmlNode& Xml, const TSet<FString>& Allowed, FParseState& State) -> bool
    {
        auto Seen = TSet<FString>{};
        for (const FXmlAttribute& Attribute : Xml.GetAttributes())
        {
            if (!Allowed.Contains(Attribute.GetTag()) || Seen.Contains(Attribute.GetTag()))
            { State.Error(TEXT("unsupported or duplicate attribute '") + Attribute.GetTag() + TEXT("' on '") + Xml.GetTag() + TEXT("'")); return false; }
            Seen.Add(Attribute.GetTag());
        }
        return true;
    }

    auto HasAttribute(const FXmlNode& Xml, const FString& Name) -> bool
    {
        for (const FXmlAttribute& Attribute : Xml.GetAttributes())
        { if (Attribute.GetTag() == Name) { return true; } }
        return false;
    }

    auto ParseMenus(const FXmlNode& Root, FCkUiDocument& InOutDocument, FParseState& State, int32& InOutDeclarationCount) -> bool
    {
        for (const FXmlNode* MenuXml : Root.GetChildrenNodes())
        {
            if (MenuXml->GetTag() != TEXT("menu")) { continue; }
            const FString Id = MenuXml->GetAttribute(TEXT("id")).TrimStartAndEnd();
            if (++InOutDeclarationCount > MaxNodes || !IsName(Id) || InOutDocument.Menus.Contains(Id)
                || !HasOnlyAttributes(*MenuXml, {TEXT("id")}, State) || !IsOnlyWhitespace(MenuXml->GetContent())
                || MenuXml->GetChildrenNodes().IsEmpty())
            { State.Error(TEXT("menu requires a unique identifier and at least one entry")); return false; }
            FCkUiMenu Menu{.Id = Id};
            auto Keys = TSet<FString>{};
            for (const FXmlNode* EntryXml : MenuXml->GetChildrenNodes())
            {
                if (++InOutDeclarationCount > MaxNodes || !EntryXml->GetChildrenNodes().IsEmpty() || !IsOnlyWhitespace(EntryXml->GetContent()))
                { State.Error(TEXT("menu entry count or content is invalid")); return false; }
                FCkUiMenuEntry Entry;
                const FString Tag = EntryXml->GetTag();
                if (Tag == TEXT("menu-item"))
                {
                    Entry.Kind = ECkUiMenuEntryKind::Item;
                    if (!HasOnlyAttributes(*EntryXml, {TEXT("key"), TEXT("label"), TEXT("label-bind"), TEXT("tooltip"), TEXT("tooltip-bind"), TEXT("enabled-bind"), TEXT("visible-bind"), TEXT("action")}, State)) { return false; }
                    Entry.Action = EntryXml->GetAttribute(TEXT("action")).TrimStartAndEnd();
                }
                else if (Tag == TEXT("separator"))
                {
                    Entry.Kind = ECkUiMenuEntryKind::Separator;
                    if (!HasOnlyAttributes(*EntryXml, {TEXT("key")}, State)) { return false; }
                }
                else if (Tag == TEXT("submenu"))
                {
                    Entry.Kind = ECkUiMenuEntryKind::Submenu;
                    if (!HasOnlyAttributes(*EntryXml, {TEXT("key"), TEXT("label"), TEXT("label-bind"), TEXT("tooltip"), TEXT("tooltip-bind"), TEXT("enabled-bind"), TEXT("visible-bind"), TEXT("menu")}, State)) { return false; }
                    Entry.MenuReference = EntryXml->GetAttribute(TEXT("menu")).TrimStartAndEnd();
                }
                else { State.Error(TEXT("unknown menu entry '") + Tag + TEXT("'")); return false; }

                Entry.Key = EntryXml->GetAttribute(TEXT("key")).TrimStartAndEnd();
                if (Entry.Key.IsEmpty() || Keys.Contains(Entry.Key)) { State.Error(TEXT("menu entries require unique non-empty keys")); return false; }
                Keys.Add(Entry.Key);
                if (Entry.Kind == ECkUiMenuEntryKind::Separator) { Menu.Entries.Add(MoveTemp(Entry)); continue; }
                const bool bHasLabel = HasAttribute(*EntryXml, TEXT("label"));
                const bool bHasLabelBinding = HasAttribute(*EntryXml, TEXT("label-bind"));
                const bool bHasTooltip = HasAttribute(*EntryXml, TEXT("tooltip"));
                const bool bHasTooltipBinding = HasAttribute(*EntryXml, TEXT("tooltip-bind"));
                if (bHasLabel == bHasLabelBinding || (bHasTooltip && bHasTooltipBinding))
                { State.Error(TEXT("menu item requires exactly one label form and at most one tooltip form")); return false; }
                Entry.Label = EntryXml->GetAttribute(TEXT("label")).TrimStartAndEnd();
                Entry.LabelBinding = EntryXml->GetAttribute(TEXT("label-bind")).TrimStartAndEnd();
                Entry.Tooltip = EntryXml->GetAttribute(TEXT("tooltip")).TrimStartAndEnd();
                Entry.TooltipBinding = EntryXml->GetAttribute(TEXT("tooltip-bind")).TrimStartAndEnd();
                Entry.EnabledBinding = EntryXml->GetAttribute(TEXT("enabled-bind")).TrimStartAndEnd();
                Entry.VisibilityBinding = EntryXml->GetAttribute(TEXT("visible-bind")).TrimStartAndEnd();
                if ((bHasLabel && Entry.Label.IsEmpty()) || (bHasTooltip && Entry.Tooltip.IsEmpty())
                    || (bHasLabelBinding && !IsName(Entry.LabelBinding)) || (bHasTooltipBinding && !IsName(Entry.TooltipBinding))
                    || (HasAttribute(*EntryXml, TEXT("enabled-bind")) && !IsName(Entry.EnabledBinding)) || (HasAttribute(*EntryXml, TEXT("visible-bind")) && !IsName(Entry.VisibilityBinding))
                    || (Entry.Kind == ECkUiMenuEntryKind::Item && !IsName(Entry.Action))
                    || (Entry.Kind == ECkUiMenuEntryKind::Submenu && !IsName(Entry.MenuReference)))
                { State.Error(TEXT("menu entry has an invalid binding, action, or menu reference")); return false; }
                Menu.Entries.Add(MoveTemp(Entry));
            }
            InOutDocument.Menus.Add(Id, MoveTemp(Menu));
        }
        return true;
    }

    auto ValidateMenuReferences(const FCkUiDocument& Document, FParseState& State) -> bool
    {
        struct FMenuSummary { int32 Count = 0; int32 Depth = 0; };
        TSet<FString> Visiting;
        auto Summaries = TMap<FString, FMenuSummary>{};
        const auto ValidateMenu = [&Document, &State, &Visiting, &Summaries](const auto& Self, const FString& Id, const int32 Depth, FMenuSummary& Out) -> bool
        {
            if (Depth > MaxDepth) { State.Error(TEXT("menu reference depth limit exceeded")); return false; }
            if (const FMenuSummary* Existing = Summaries.Find(Id)) { Out = *Existing; return true; }
            if (Visiting.Contains(Id)) { State.Error(TEXT("recursive menu reference '") + Id + TEXT("'")); return false; }
            const FCkUiMenu* Menu = Document.Menus.Find(Id);
            if (Menu == nullptr) { State.Error(TEXT("unknown menu reference '") + Id + TEXT("'")); return false; }
            Visiting.Add(Id);
            FMenuSummary Summary;
            for (const FCkUiMenuEntry& Entry : Menu->Entries)
            {
                ++Summary.Count;
                Summary.Depth = FMath::Max(Summary.Depth, 1);
                if (Entry.Kind == ECkUiMenuEntryKind::Submenu)
                {
                    FMenuSummary Child;
                    if (!Self(Self, Entry.MenuReference, Depth + 1, Child)) { return false; }
                    Summary.Count += Child.Count;
                    Summary.Depth = FMath::Max(Summary.Depth, Child.Depth + 1);
                }
                if (Summary.Count > MaxNodes || Summary.Depth > MaxDepth)
                { State.Error(TEXT("expanded menu entry/depth limit exceeded")); return false; }
            }
            Visiting.Remove(Id);
            Summaries.Add(Id, Summary);
            Out = Summary;
            return true;
        };
        for (const auto& Pair : Document.Menus)
        {
            FMenuSummary Summary;
            if (!ValidateMenu(ValidateMenu, Pair.Key, 1, Summary)) { return false; }
        }
        const auto ValidateNodes = [&Document, &State](const auto& Self, const FCkUiNode& Node) -> bool
        {
            const FString* MenuReference = Node.Kind == ECkUiNodeKind::MenuButton ? &Node.MenuReference
                : (Node.Kind == ECkUiNodeKind::Table || Node.Kind == ECkUiNodeKind::Tree) && !Node.ContextMenuReference.IsEmpty()
                    ? &Node.ContextMenuReference : nullptr;
            if (MenuReference != nullptr && !Document.Menus.Contains(*MenuReference))
            { State.Error(TEXT("unknown menu reference '") + *MenuReference + TEXT("'")); return false; }
            for (const FCkUiNode& Child : Node.Children) if (!Self(Self, Child)) { return false; }
            return true;
        };
        for (const auto& Pair : Document.Regions) if (!ValidateNodes(ValidateNodes, Pair.Value)) { return false; }
        return true;
    }

    auto CompileNode(const FXmlNode& Xml, const TMap<FString, FTemplate>& Templates,
        const TMap<FString, EValueKind>* Parameters, const TSharedPtr<const FCkUiWidgetRegistrySnapshot>& Registry,
        const TArray<FStyleRule>& Rules, FParseState& State, int32& DeclarationCount,
        TSet<FString>& LocalIds, const int32 Depth, FAuthoredNode& Out) -> bool
    {
        if (Depth > MaxDepth || ++DeclarationCount > MaxNodes)
        { State.Error(TEXT("declaration node/depth limit exceeded")); return false; }
        const FString Tag = Xml.GetTag();
        Out.Prototype.Id = Xml.GetAttribute(TEXT("id")).TrimStartAndEnd();
        if (Out.Prototype.Id.IsEmpty() || LocalIds.Contains(Out.Prototype.Id))
        { State.Error(TEXT("node '") + Tag + TEXT("': missing or duplicate id")); return false; }
        LocalIds.Add(Out.Prototype.Id);
        auto Seen = TSet<FString>{};
        for (const FXmlAttribute& Attribute : Xml.GetAttributes())
        {
            if (Seen.Contains(Attribute.GetTag())) { State.Error(TEXT("duplicate attribute '") + Attribute.GetTag() + TEXT("'")); return false; }
            Seen.Add(Attribute.GetTag());
        }
        auto Expected = TMap<FString, EValueKind>{};
        auto Required = TSet<FString>{};
        auto ArgumentNames = TMap<FString, FString>{};
        const FCkUiBuiltinWidgetSchema* Schema = nullptr;
        const FCkUiCustomWidgetRegistration* Custom = nullptr;
        if (Tag == TEXT("use"))
        {
            Out.Template = Xml.GetAttribute(TEXT("template")).TrimStartAndEnd();
            const FTemplate* Definition = Templates.Find(Out.Template);
            if (Definition == nullptr) { State.Error(TEXT("unknown template '") + Out.Template + TEXT("'")); return false; }
            for (const auto& Parameter : Definition->Parameters)
            {
                const FString Attribute = IsBindingKind(Parameter.Value) ? Parameter.Key + TEXT("-bind") : Parameter.Key;
                Expected.Add(Attribute, Parameter.Value);
                Required.Add(Attribute);
                ArgumentNames.Add(Attribute, Parameter.Key);
            }
            if (!Xml.GetChildrenNodes().IsEmpty() || !IsOnlyWhitespace(Xml.GetContent()))
            { State.Error(TEXT("template use cannot contain text or children")); return false; }
        }
        else
        {
            Schema = ck_ui_widget_registry::FindByTag(Tag);
            Custom = Schema == nullptr && Registry.IsValid() ? Registry->Find(Tag) : nullptr;
            if (Schema == nullptr && Custom == nullptr) { State.Error(TEXT("unknown node tag '") + Tag + TEXT("'")); return false; }
            Out.Prototype.Kind = Schema != nullptr ? Schema->Kind : ECkUiNodeKind::Custom;
            Out.Prototype.CustomTag = Custom != nullptr ? Tag : FString{};
            if (Schema == nullptr || Schema->bAllowsVisibility) { Expected.Add(TEXT("visible"), EValueKind::BoolBinding); }
            if (Schema == nullptr || Schema->bAllowsVisibility) { Expected.Add(TEXT("visible-field"), EValueKind::BoolBinding); }
            if (Schema != nullptr)
            {
                if (Schema->BindingKind != ECkUiBuiltinBindingKind::None)
                {
                    Expected.Add(TEXT("bind"), BindingKind(Schema->BindingKind));
                    if (Schema->BindingKind != ECkUiBuiltinBindingKind::OptionalText) { Required.Add(TEXT("bind")); }
                }
                if (Schema->bAllowsAction) { Expected.Add(TEXT("action"), EValueKind::Action); }
                if (Schema->bRequiresAction) { Required.Add(TEXT("action")); }
                if (Schema->bAllowsPlaceholder)
                {
                    Expected.Add(TEXT("placeholder"), EValueKind::Text);
                    Expected.Add(TEXT("placeholder-bind"), EValueKind::TextBinding);
                }
                if (Schema->Kind == ECkUiNodeKind::Button)
                {
                    Expected.Add(TEXT("enabled-bind"), EValueKind::BoolBinding);
                    Expected.Add(TEXT("item-action"), EValueKind::Action);
                    Expected.Add(TEXT("bind-field"), EValueKind::TextBinding);
                }
                if (Schema->Kind == ECkUiNodeKind::Scroll || Schema->Kind == ECkUiNodeKind::Splitter || Schema->Kind == ECkUiNodeKind::Repeat) { Expected.Add(TEXT("direction"), EValueKind::Text); }
                if (Schema->Kind == ECkUiNodeKind::Text) { Expected.Add(TEXT("bind-field"), EValueKind::TextBinding); }
                if (Schema->Kind == ECkUiNodeKind::Image) { Expected.Add(TEXT("bind-field"), EValueKind::ImageBinding); }
                if (Schema->Kind == ECkUiNodeKind::Text || Schema->Kind == ECkUiNodeKind::Button)
                {
                    Expected.Add(TEXT("color-bind"), EValueKind::ColorBinding);
                    Expected.Add(TEXT("color-field"), EValueKind::ColorBinding);
                    Expected.Add(TEXT("tooltip"), EValueKind::Text);
                    Expected.Add(TEXT("tooltip-bind"), EValueKind::TextBinding);
                    Expected.Add(TEXT("tooltip-field"), EValueKind::TextBinding);
                }
                if (Schema->Kind == ECkUiNodeKind::Table || Schema->Kind == ECkUiNodeKind::Tree)
                {
                    Expected.Add(TEXT("row-height"), EValueKind::Number);
                    Expected.Add(TEXT("filter-bind"), EValueKind::TextBinding);
                    Expected.Add(TEXT("selection-action"), EValueKind::Action);
                    Expected.Add(TEXT("context-menu"), EValueKind::Text);
                    if (Schema->Kind == ECkUiNodeKind::Table) { Expected.Add(TEXT("context-menu-action"), EValueKind::Action); Expected.Add(TEXT("selectable"), EValueKind::Bool); }
                }
                if (Schema->Kind == ECkUiNodeKind::TableColumn)
                {
                    Expected.Add(TEXT("label"), EValueKind::Text);
                    Expected.Add(TEXT("label-bind"), EValueKind::TextBinding);
                    Expected.Add(TEXT("sort-field"), EValueKind::Text);
                }
                if (Schema->Kind == ECkUiNodeKind::Tabs)
                {
                    Expected.Add(TEXT("value-bind"), EValueKind::StringBinding); Required.Add(TEXT("value-bind"));
                    Expected.Add(TEXT("changed"), EValueKind::StringChanged); Required.Add(TEXT("changed"));
                }
                if (Schema->Kind == ECkUiNodeKind::Tab)
                {
                    Expected.Add(TEXT("key"), EValueKind::Text); Required.Add(TEXT("key"));
                    Expected.Add(TEXT("label"), EValueKind::Text);
                    Expected.Add(TEXT("label-bind"), EValueKind::TextBinding);
                    Expected.Add(TEXT("enabled-bind"), EValueKind::BoolBinding);
                }
                if (Schema->Kind == ECkUiNodeKind::MenuButton)
                {
                    Expected.Add(TEXT("menu"), EValueKind::Text); Required.Add(TEXT("menu"));
                    Expected.Add(TEXT("label"), EValueKind::Text);
                    Expected.Add(TEXT("label-bind"), EValueKind::TextBinding);
                    Expected.Add(TEXT("enabled-bind"), EValueKind::BoolBinding);
                }
            }
            else
            {
                for (const FCkUiCustomPropertySchema& Property : Custom->Schema.Properties)
                {
                    const FString Attribute = IsBindingProperty(Property.Kind) ? Property.Name + TEXT("-bind") : Property.Name;
                    Expected.Add(Attribute, CustomKind(Property.Kind));
                    if (IsBindingProperty(Property.Kind) && Property.Kind != ECkUiCustomPropertyKind::Action)
                    { Expected.Add(Property.Name + TEXT("-field"), CustomKind(Property.Kind)); }
                    if (Property.bRequired) { Required.Add(Attribute); }
                }
            }
        }
        auto Supplied = TSet<FString>{};
        for (const FXmlAttribute& Attribute : Xml.GetAttributes())
        {
            const FString Name = Attribute.GetTag();
            if (Name == TEXT("id")) { continue; }
            if (Tag == TEXT("use") && Name == TEXT("template")) { continue; }
            if (Tag != TEXT("use") && Name == TEXT("class")) { continue; }
            const bool Reference = Name.EndsWith(TEXT("-param"), ESearchCase::CaseSensitive);
            const FString Target = Reference ? Name.LeftChop(6) : Name;
            const bool LiteralTextReference = Schema != nullptr && Schema->bAllowsLiteralText && Name == TEXT("text-param");
            const EValueKind* ExpectedKind = Expected.Find(Target);
            if ((!LiteralTextReference && ExpectedKind == nullptr) || (Reference && Parameters == nullptr))
            { State.Error(TEXT("unsupported attribute '") + Name + TEXT("' on '") + Tag + TEXT("'")); return false; }
            const bool bFieldReference = Target == TEXT("bind-field") || Target == TEXT("visible-field") || Target == TEXT("color-field") || Target == TEXT("tooltip-field") || (Custom != nullptr && Target.EndsWith(TEXT("-field")));
            const FString LogicalTarget = bFieldReference ? Target.LeftChop(6) : Target;
            const FString FieldTarget = bFieldReference ? Target : LogicalTarget + TEXT("-field");
            const FString CustomBase = Custom != nullptr ? (Target.EndsWith(TEXT("-field")) ? Target.LeftChop(6) : Target.EndsWith(TEXT("-bind")) ? Target.LeftChop(5) : Target) : FString{};
            const bool bCustomBindingConflict = Custom != nullptr && (Supplied.Contains(CustomBase + TEXT("-bind")) || Supplied.Contains(CustomBase + TEXT("-field")));
            const bool bTextMetadataConflict = Schema != nullptr && (Schema->Kind == ECkUiNodeKind::Text || Schema->Kind == ECkUiNodeKind::Button) && ((Target == TEXT("color-bind") || Target == TEXT("color-field")) && (Supplied.Contains(TEXT("color-bind")) || Supplied.Contains(TEXT("color-field")))) || (Schema != nullptr && (Schema->Kind == ECkUiNodeKind::Text || Schema->Kind == ECkUiNodeKind::Button) && (Target == TEXT("tooltip") || Target == TEXT("tooltip-bind") || Target == TEXT("tooltip-field")) && (Supplied.Contains(TEXT("tooltip")) || Supplied.Contains(TEXT("tooltip-bind")) || Supplied.Contains(TEXT("tooltip-field"))));
            const bool bTabLabelConflict = Schema != nullptr && Schema->Kind == ECkUiNodeKind::Tab
                && (Target == TEXT("label") || Target == TEXT("label-bind"))
                && (Supplied.Contains(TEXT("label")) || Supplied.Contains(TEXT("label-bind")));
            const bool bMenuButtonLabelConflict = Schema != nullptr && Schema->Kind == ECkUiNodeKind::MenuButton
                && (Target == TEXT("label") || Target == TEXT("label-bind"))
                && (Supplied.Contains(TEXT("label")) || Supplied.Contains(TEXT("label-bind")));
            const bool bTableColumnLabelConflict = Schema != nullptr && Schema->Kind == ECkUiNodeKind::TableColumn
                && (Target == TEXT("label") || Target == TEXT("label-bind"))
                && (Supplied.Contains(TEXT("label")) || Supplied.Contains(TEXT("label-bind")));
            const bool bPlaceholderConflict = Schema != nullptr && Schema->bAllowsPlaceholder
                && (Target == TEXT("placeholder") || Target == TEXT("placeholder-bind"))
                && (Supplied.Contains(TEXT("placeholder")) || Supplied.Contains(TEXT("placeholder-bind")));
            const bool bContextMenuConflict = Schema != nullptr && Schema->Kind == ECkUiNodeKind::Table
                && ((Target == TEXT("context-menu") && Supplied.Contains(TEXT("context-menu-action")))
                    || (Target == TEXT("context-menu-action") && Supplied.Contains(TEXT("context-menu"))));
            const bool bButtonActionConflict = Schema != nullptr && Schema->Kind == ECkUiNodeKind::Button
                && ((Target == TEXT("action") && Supplied.Contains(TEXT("item-action")))
                    || (Target == TEXT("item-action") && Supplied.Contains(TEXT("action"))));
            if (Supplied.Contains(Target) || Supplied.Contains(LogicalTarget) || Supplied.Contains(FieldTarget) || bTextMetadataConflict || bTabLabelConflict || bMenuButtonLabelConflict || bTableColumnLabelConflict || bPlaceholderConflict || bContextMenuConflict || bButtonActionConflict || (Custom != nullptr && bFieldReference && Supplied.Contains(LogicalTarget + TEXT("-bind"))) || bCustomBindingConflict) { State.Error(TEXT("literal, parameter, and field reference conflict for '") + (Custom != nullptr ? CustomBase : LogicalTarget) + TEXT("'")); return false; }
            Supplied.Add(Target);
            auto Value = FValue{};
            if (!CompileValue(Attribute.GetValue(), LiteralTextReference ? EValueKind::Text : *ExpectedKind, Reference, Parameters, Value, State)) { return false; }
            Out.Fields.Add(Tag == TEXT("use") ? ArgumentNames.FindChecked(Target) : Target, MoveTemp(Value));
        }
        for (const FString& Name : Required)
        {
            const bool bFieldBindingSatisfiesBind = Name == TEXT("bind") && (Schema != nullptr && (Schema->Kind == ECkUiNodeKind::Text || Schema->Kind == ECkUiNodeKind::Image || Schema->Kind == ECkUiNodeKind::Button)) && Supplied.Contains(TEXT("bind-field"));
            const bool bItemActionSatisfiesAction = Name == TEXT("action") && Schema != nullptr && Schema->Kind == ECkUiNodeKind::Button && Supplied.Contains(TEXT("item-action"));
            const bool bCustomFieldSatisfies = Custom != nullptr && Name.EndsWith(TEXT("-bind")) && Supplied.Contains(Name.LeftChop(5) + TEXT("-field"));
            if (!Supplied.Contains(Name) && !bFieldBindingSatisfiesBind && !bItemActionSatisfiesAction && !bCustomFieldSatisfies) { State.Error(TEXT("missing required attribute '") + Name + TEXT("'")); return false; }
        }
        if (Schema != nullptr && Schema->Kind == ECkUiNodeKind::Tab
            && !Supplied.Contains(TEXT("label")) && !Supplied.Contains(TEXT("label-bind")))
        { State.Error(TEXT("tab requires exactly one of 'label' or 'label-bind'")); return false; }
        if (Schema != nullptr && Schema->Kind == ECkUiNodeKind::MenuButton
            && !Supplied.Contains(TEXT("label")) && !Supplied.Contains(TEXT("label-bind")))
        { State.Error(TEXT("menu-button requires exactly one of 'label' or 'label-bind'")); return false; }
        if (Schema != nullptr && Schema->Kind == ECkUiNodeKind::TableColumn
            && !Supplied.Contains(TEXT("label")) && !Supplied.Contains(TEXT("label-bind")))
        { State.Error(TEXT("table-column requires exactly one of 'label' or 'label-bind'")); return false; }
        if (Tag == TEXT("use")) { return true; }
        const bool HasContent = !IsOnlyWhitespace(Xml.GetContent());
        if (Schema != nullptr && Schema->bAllowsLiteralText)
        {
            if (Out.Fields.Contains(TEXT("text")) && HasContent)
            { State.Error(TEXT("text parameter and literal content conflict")); return false; }
            if ((Out.Fields.Contains(TEXT("bind")) || Out.Fields.Contains(TEXT("bind-field"))) && (HasContent || Out.Fields.Contains(TEXT("text"))))
            { State.Error(TEXT("text cannot declare both bind and literal text")); return false; }
            if (!Out.Fields.Contains(TEXT("text")) && !Out.Fields.Contains(TEXT("bind")) && !Out.Fields.Contains(TEXT("bind-field")))
            {
                auto Content = FValue{};
                if (!CompileValue(Xml.GetContent(), EValueKind::Text, false, nullptr, Content, State)) { return false; }
                Out.Fields.Add(TEXT("text"), MoveTemp(Content));
            }
        }
        else if (HasContent) { State.Error(TEXT("node '") + Tag + TEXT("' cannot contain text")); return false; }
        const int32 ChildCount = Xml.GetChildrenNodes().Num();
        if ((Schema == nullptr && (Custom == nullptr || Custom->Schema.Slots.IsEmpty()) && ChildCount != 0) || (Schema != nullptr &&
            (ChildCount < Schema->MinChildren || (Schema->MaxChildren != INDEX_NONE && ChildCount > Schema->MaxChildren))))
        { State.Error(TEXT("node '") + Tag + TEXT("': invalid child count")); return false; }
        if (!ApplyClasses(Out.Prototype, Xml.GetAttribute(TEXT("class")), Rules, State)) { return false; }
        if (Schema != nullptr && Schema->bRequiresZeroGap && Out.Prototype.Style.Gap != 0.0f)
        {
            State.Error(Schema->Kind == ECkUiNodeKind::Scroll
                ? TEXT("scroll cannot use gap with one child")
                : TEXT("splitter cannot use gap"));
            return false;
        }
        TSet<FString> SuppliedSlots;
        for (const FXmlNode* Child : Xml.GetChildrenNodes())
        {
            const FXmlNode* Root = Child;
            FString SlotName;
            if (Custom != nullptr && !Custom->Schema.Slots.IsEmpty())
            {
                if (Child->GetTag() != TEXT("slot") || Child->GetAttributes().Num() != 1
                    || Child->GetAttributes()[0].GetTag() != TEXT("name") || !IsOnlyWhitespace(Child->GetContent())
                    || Child->GetChildrenNodes().Num() != 1)
                { State.Error(TEXT("custom slot requires only name and exactly one authored root")); return false; }
                SlotName = Child->GetAttribute(TEXT("name"));
                if (SuppliedSlots.Contains(SlotName) || !Custom->Schema.Slots.ContainsByPredicate([&SlotName](const FCkUiCustomSlotSchema& Slot) { return Slot.Name == SlotName; }))
                { State.Error(TEXT("unknown or duplicate custom slot '") + SlotName + TEXT("'")); return false; }
                if (++DeclarationCount > MaxNodes || Depth + 2 > MaxDepth)
                { State.Error(TEXT("slot declaration node/depth limit exceeded")); return false; }
                SuppliedSlots.Add(SlotName);
                Root = Child->GetChildrenNodes()[0];
            }
            FAuthoredNode Compiled;
            if (!CompileNode(*Root, Templates, Parameters, Registry, Rules, State, DeclarationCount, LocalIds, Depth + (SlotName.IsEmpty() ? 1 : 2), Compiled)) { return false; }
            Compiled.Prototype.CustomSlotName = SlotName;
            Out.Children.Add(MoveTemp(Compiled));
        }
        if (Custom != nullptr)
        {
            for (const FCkUiCustomSlotSchema& Slot : Custom->Schema.Slots)
            {
                if (Slot.bRequired && !SuppliedSlots.Contains(Slot.Name))
                { State.Error(TEXT("missing required custom slot '") + Slot.Name + TEXT("'")); return false; }
            }
        }
        if (Schema != nullptr && Schema->Kind == ECkUiNodeKind::Scroll)
        {
            const FValue* Direction = Out.Fields.Find(TEXT("direction"));
            if (Direction != nullptr && !Direction->IsReference && Direction->Data.Text.ToString() != TEXT("vertical") && Direction->Data.Text.ToString() != TEXT("horizontal"))
            { State.Error(TEXT("scroll direction must be vertical or horizontal")); return false; }
        }
        if (Schema != nullptr && Schema->Kind == ECkUiNodeKind::Splitter)
        {
            const FValue* Direction = Out.Fields.Find(TEXT("direction"));
            if (Direction != nullptr && !Direction->IsReference && Direction->Data.Text.ToString() != TEXT("vertical") && Direction->Data.Text.ToString() != TEXT("horizontal"))
            { State.Error(TEXT("splitter direction must be vertical or horizontal")); return false; }
        }
        return true;
    }

    auto AnalyzeNode(const FAuthoredNode& Node, const TMap<FString, FTemplate>& Templates,
        TMap<FString, FSummary>& Cache, TSet<FString>& Calls, FParseState& State, const int32 CallDepth, FSummary& Out) -> bool
    {
        if (CallDepth > MaxDepth) { State.Error(TEXT("template depth limit exceeded")); return false; }
        Out.Count = 1;
        Out.Depth = 1;
        Out.RootKind = Node.Prototype.Kind;
        if (!Node.Template.IsEmpty())
        {
            if (Calls.Contains(Node.Template)) { State.Error(TEXT("recursive template '") + Node.Template + TEXT("'")); return false; }
            const FSummary* Summary = Cache.Find(Node.Template);
            if (Summary == nullptr)
            {
                const FTemplate* Definition = Templates.Find(Node.Template);
                if (Definition == nullptr) { State.Error(TEXT("unknown template")); return false; }
                Calls.Add(Node.Template);
                FSummary Computed;
                if (!AnalyzeNode(Definition->Body, Templates, Cache, Calls, State, CallDepth + 1, Computed)) { return false; }
                Calls.Remove(Node.Template);
                Cache.Add(Node.Template, Computed);
                Summary = Cache.Find(Node.Template);
            }
            Out.Count += Summary->Count;
            Out.Depth += Summary->Depth;
            Out.RootKind = Summary->RootKind;
        }
        for (const FAuthoredNode& Child : Node.Children)
        {
            FSummary ChildSummary;
            if (!AnalyzeNode(Child, Templates, Cache, Calls, State, CallDepth + 1, ChildSummary)) { return false; }
            Out.Count += ChildSummary.Count;
            Out.Depth = FMath::Max(Out.Depth, ChildSummary.Depth + 1);
            if (Out.Count > MaxNodes) { State.Error(TEXT("expanded node limit exceeded")); return false; }
        }
        if (Out.Count > MaxNodes || Out.Depth > MaxDepth) { State.Error(TEXT("expanded node/depth limit exceeded")); return false; }
        return true;
    }

    auto ResolveValue(const FValue& Value, const TMap<FString, FValue>& Arguments, FValue& Out, FParseState& State) -> bool
    {
        const FValue* Resolved = Value.IsReference ? Arguments.Find(Value.Parameter) : &Value;
        if (Resolved == nullptr || Resolved->IsReference || Resolved->Kind != Value.Kind)
        { State.Error(TEXT("template value resolution failed")); return false; }
        Out = *Resolved;
        return true;
    }

    auto ExpandNode(const FAuthoredNode& Node, const TMap<FString, FTemplate>& Templates,
        const TMap<FString, FValue>& Arguments, const FString& Prefix,
        const TSharedPtr<const FCkUiWidgetRegistrySnapshot>& Registry, FParseState& State,
        const int32 Depth, FCkUiNode& Out) -> bool
    {
        if (Depth > MaxDepth || ++State.NodeCount > MaxNodes) { State.Error(TEXT("expanded node/depth limit exceeded")); return false; }
        const FString Id = Prefix.IsEmpty() ? Node.Prototype.Id : Prefix + TEXT("/") + Node.Prototype.Id;
        if ((!Prefix.IsEmpty() || !Node.Template.IsEmpty()) && Id.Len() > 1024)
        { State.Error(TEXT("template generated id exceeds 1024 characters")); return false; }
        if (State.Ids.Contains(Id)) { State.Error(TEXT("missing or duplicate id '") + Id + TEXT("'")); return false; }
        State.Ids.Add(Id);
        auto Values = TMap<FString, FValue>{};
        for (const auto& Field : Node.Fields)
        {
            FValue Resolved;
            if (!ResolveValue(Field.Value, Arguments, Resolved, State)) { return false; }
            Values.Add(Field.Key, MoveTemp(Resolved));
        }
        if (!Node.Template.IsEmpty())
        {
            const FTemplate* Definition = Templates.Find(Node.Template);
            if (Definition == nullptr) { State.Error(TEXT("unknown template")); return false; }
            if (!ExpandNode(Definition->Body, Templates, Values, Id, Registry, State, Depth + 1, Out)) { return false; }
            Out.CustomSlotName = Node.Prototype.CustomSlotName;
            return true;
        }
        Out = Node.Prototype;
        Out.Id = Id;
        if (const FValue* Visible = Values.Find(TEXT("visible"))) { Out.VisibilityBinding = Visible->Data.Name; }
        if (const FValue* VisibleField = Values.Find(TEXT("visible-field"))) { Out.FieldBindings.Add(TEXT("visible"), VisibleField->Data.Name); }
        if (const FValue* Color = Values.Find(TEXT("color-bind"))) { Out.ColorBinding = Color->Data.Name; }
        if (const FValue* ColorField = Values.Find(TEXT("color-field"))) { Out.FieldBindings.Add(TEXT("color"), ColorField->Data.Name); }
        if (const FValue* Tooltip = Values.Find(TEXT("tooltip"))) { Out.Tooltip = Tooltip->Data.Text.ToString(); }
        if (const FValue* TooltipBinding = Values.Find(TEXT("tooltip-bind"))) { Out.TooltipBinding = TooltipBinding->Data.Name; }
        if (const FValue* TooltipField = Values.Find(TEXT("tooltip-field"))) { Out.FieldBindings.Add(TEXT("tooltip"), TooltipField->Data.Name); }
        if (Out.Kind == ECkUiNodeKind::Repeat)
        {
            const FString Direction = Values.Contains(TEXT("direction")) ? Values.FindChecked(TEXT("direction")).Data.Text.ToString() : TEXT("vertical");
            if (Direction == TEXT("vertical")) { Out.RepeatDirection = Orient_Vertical; }
            else if (Direction == TEXT("horizontal")) { Out.RepeatDirection = Orient_Horizontal; }
            else { State.Error(TEXT("repeat direction must be vertical or horizontal")); return false; }
        }
        if (Out.Kind == ECkUiNodeKind::Scroll)
        {
            const FString Direction = Values.Contains(TEXT("direction")) ? Values.FindChecked(TEXT("direction")).Data.Text.ToString() : TEXT("vertical");
            if (Direction == TEXT("vertical")) { Out.ScrollDirection = Orient_Vertical; }
            else if (Direction == TEXT("horizontal")) { Out.ScrollDirection = Orient_Horizontal; }
            else { State.Error(TEXT("scroll direction must be vertical or horizontal")); return false; }
        }
        if (Out.Kind == ECkUiNodeKind::Splitter)
        {
            const FString Direction = Values.Contains(TEXT("direction")) ? Values.FindChecked(TEXT("direction")).Data.Text.ToString() : TEXT("horizontal");
            if (Direction == TEXT("horizontal")) { Out.SplitterDirection = Orient_Horizontal; }
            else if (Direction == TEXT("vertical")) { Out.SplitterDirection = Orient_Vertical; }
            else { State.Error(TEXT("splitter direction must be vertical or horizontal")); return false; }
        }
        if (Out.Kind == ECkUiNodeKind::Custom)
        {
            const FCkUiCustomWidgetRegistration* Custom = Registry.IsValid() ? Registry->Find(Out.CustomTag) : nullptr;
            if (Custom == nullptr) { State.Error(TEXT("missing custom registration")); return false; }
            for (const FCkUiCustomPropertySchema& Property : Custom->Schema.Properties)
            {
                const FString Attribute = IsBindingProperty(Property.Kind) ? Property.Name + TEXT("-bind") : Property.Name;
                if (const FValue* FieldValue = Values.Find(Property.Name + TEXT("-field"))) { Out.FieldBindings.Add(Property.Name, FieldValue->Data.Name); }
                else if (const FValue* Value = Values.Find(Attribute)) { Out.CustomProperties.Add(Property.Name, Value->Data); }
            }
        }
        else
        {
            if (const FValue* Binding = Values.Find(TEXT("bind"))) { Out.Binding = Binding->Data.Name; }
            if (const FValue* BindingField = Values.Find(TEXT("bind-field"))) { Out.FieldBindings.Add(TEXT("bind"), BindingField->Data.Name); }
            if (const FValue* Action = Values.Find(TEXT("action"))) { Out.Action = Action->Data.Name; }
            if (const FValue* ItemAction = Values.Find(TEXT("item-action"))) { Out.ItemAction = ItemAction->Data.Name; }
            if (const FValue* Placeholder = Values.Find(TEXT("placeholder"))) { Out.Placeholder = Placeholder->Data.Text.ToString(); }
            if (const FValue* PlaceholderBinding = Values.Find(TEXT("placeholder-bind"))) { Out.PlaceholderBinding = PlaceholderBinding->Data.Name; }
            if (const FValue* Text = Values.Find(TEXT("text"))) { Out.Text = Text->Data.Text.ToString(); }
            if (Out.Kind == ECkUiNodeKind::Table || Out.Kind == ECkUiNodeKind::Tree)
            {
                Out.Header = TEXT("");
                if (const FValue* Selectable = Values.Find(TEXT("selectable"))) { Out.TableSelectable = Selectable->Data.Bool; }
                if (const FValue* Height = Values.Find(TEXT("row-height"))) { if (!FMath::IsFinite(Height->Data.Number) || Height->Data.Number < 1.0f || Height->Data.Number > 1024.0f) { State.Error(TEXT("table row-height must be finite within 1..1024")); return false; } Out.RowHeight = Height->Data.Number; }
                if (const FValue* Filter = Values.Find(TEXT("filter-bind"))) { Out.FilterBinding = Filter->Data.Name; }
                if (const FValue* Selection = Values.Find(TEXT("selection-action"))) { Out.SelectionAction = Selection->Data.Name; }
                if (const FValue* Context = Values.Find(TEXT("context-menu-action"))) { Out.ContextMenuAction = Context->Data.Name; }
                if (const FValue* Context = Values.Find(TEXT("context-menu")))
                {
                    Out.ContextMenuReference = Context->Data.Text.ToString().TrimStartAndEnd();
                    if (!IsName(Out.ContextMenuReference)) { State.Error(TEXT("context-menu must be an identifier")); return false; }
                }
            }
            if (Out.Kind == ECkUiNodeKind::TableColumn)
            {
                if (const FValue* Label = Values.Find(TEXT("label"))) { Out.Header = Label->Data.Text.ToString(); }
                if (const FValue* LabelBinding = Values.Find(TEXT("label-bind"))) { Out.HeaderBinding = LabelBinding->Data.Name; }
                if (const FValue* Sort = Values.Find(TEXT("sort-field"))) { Out.SortField = Sort->Data.Text.ToString(); if (!IsName(Out.SortField)) { State.Error(TEXT("table-column sort-field must be an identifier")); return false; } }
            }
            if (Out.Kind == ECkUiNodeKind::Tabs)
            {
                Out.Binding = Values.FindChecked(TEXT("value-bind")).Data.Name;
                Out.Action = Values.FindChecked(TEXT("changed")).Data.Name;
            }
            if (Out.Kind == ECkUiNodeKind::Button)
            {
                if (const FValue* EnabledBinding = Values.Find(TEXT("enabled-bind"))) { Out.ButtonEnabledBinding = EnabledBinding->Data.Name; }
            }
            if (Out.Kind == ECkUiNodeKind::Tab)
            {
                Out.TabKey = Values.FindChecked(TEXT("key")).Data.Text.ToString().TrimStartAndEnd();
                if (const FValue* Label = Values.Find(TEXT("label"))) { Out.Text = Label->Data.Text.ToString(); }
                if (const FValue* LabelBinding = Values.Find(TEXT("label-bind"))) { Out.TabLabelBinding = LabelBinding->Data.Name; }
                if (const FValue* EnabledBinding = Values.Find(TEXT("enabled-bind"))) { Out.TabEnabledBinding = EnabledBinding->Data.Name; }
            }
            if (Out.Kind == ECkUiNodeKind::MenuButton)
            {
                Out.MenuReference = Values.FindChecked(TEXT("menu")).Data.Text.ToString().TrimStartAndEnd();
                if (!IsName(Out.MenuReference)) { State.Error(TEXT("menu-button menu must be an identifier")); return false; }
                if (const FValue* Label = Values.Find(TEXT("label"))) { Out.Text = Label->Data.Text.ToString(); }
                if (const FValue* LabelBinding = Values.Find(TEXT("label-bind"))) { Out.Binding = LabelBinding->Data.Name; }
                if (const FValue* EnabledBinding = Values.Find(TEXT("enabled-bind"))) { Out.TabEnabledBinding = EnabledBinding->Data.Name; }
            }
            if (Out.Kind == ECkUiNodeKind::Native)
            {
                if (State.Bindings.Contains(Out.Binding)) { State.Error(TEXT("missing or duplicate native binding")); return false; }
                State.Bindings.Add(Out.Binding);
            }
        }
        for (const FAuthoredNode& Child : Node.Children)
        {
            FCkUiNode Expanded;
            if (!ExpandNode(Child, Templates, Arguments, Prefix, Registry, State, Depth + 1, Expanded)) { return false; }
            Out.Children.Add(MoveTemp(Expanded));
        }
        if (Out.Kind == ECkUiNodeKind::Table)
        {
            for (const FCkUiNode& Column : Out.Children) if (Column.Kind != ECkUiNodeKind::TableColumn) { State.Error(TEXT("table children must be table-column nodes")); return false; }
        }
        if (Out.Kind == ECkUiNodeKind::TableColumn && Out.Children.Num() != 1) { State.Error(TEXT("table-column must contain exactly one cell root")); return false; }
        return true;
    }

    auto ValidateTabsScope(const FCkUiNode& Node, const bool bDirectTabsChild, FParseState& State) -> bool
    {
        if (Node.Kind == ECkUiNodeKind::Tab && !bDirectTabsChild)
        { State.Error(TEXT("tab is only valid directly under tabs")); return false; }
        if (Node.Kind == ECkUiNodeKind::Tabs)
        {
            auto Keys = TSet<FString>{};
            for (const FCkUiNode& Child : Node.Children)
            {
                if (Child.Kind != ECkUiNodeKind::Tab) { State.Error(TEXT("tabs children must be tab nodes")); return false; }
                if (Child.TabKey.IsEmpty() || Keys.Contains(Child.TabKey)) { State.Error(TEXT("tabs require unique non-empty tab keys")); return false; }
                Keys.Add(Child.TabKey);
            }
        }
        for (const FCkUiNode& Child : Node.Children)
        { if (!ValidateTabsScope(Child, Node.Kind == ECkUiNodeKind::Tabs, State)) { return false; } }
        return true;
    }

    auto ValidateFieldScope(const FCkUiNode& Node, const bool bInRepeatItem, const bool bInTableCell, const bool bDirectTableColumn, FParseState& State) -> bool
    {
        if (!bInRepeatItem && !bInTableCell && !Node.FieldBindings.IsEmpty()) { State.Error(TEXT("field references are only valid inside repeat items, table cells, or tree rows")); return false; }
        if (!Node.ItemAction.IsEmpty() && (Node.Kind != ECkUiNodeKind::Button || !bInRepeatItem))
        { State.Error(TEXT("item-action is only valid on buttons inside repeat items")); return false; }
        if (bInRepeatItem && (Node.Kind == ECkUiNodeKind::Repeat || Node.Kind == ECkUiNodeKind::Table || Node.Kind == ECkUiNodeKind::Tree || Node.Kind == ECkUiNodeKind::Native))
        { State.Error(TEXT("repeat items do not support nested repeat, table, tree, or native nodes")); return false; }
        if (bInTableCell && Node.Kind == ECkUiNodeKind::Splitter)
        { State.Error(TEXT("splitter is not valid inside virtualized table cells or tree rows")); return false; }
        if (bInTableCell && Node.Kind != ECkUiNodeKind::Row && Node.Kind != ECkUiNodeKind::Column && Node.Kind != ECkUiNodeKind::Text && Node.Kind != ECkUiNodeKind::Image && Node.Kind != ECkUiNodeKind::Scroll && Node.Kind != ECkUiNodeKind::Custom)
        { State.Error(TEXT("table cells only support row, column, text, image, and scroll nodes")); return false; }
        if (Node.Kind == ECkUiNodeKind::TableColumn && !bDirectTableColumn) { State.Error(TEXT("table-column is only valid directly under table")); return false; }
        if (Node.Kind == ECkUiNodeKind::TableColumn && Node.Children.Num() != 1) { State.Error(TEXT("table-column requires one cell root")); return false; }
        for (const FCkUiNode& Child : Node.Children)
        {
            const bool bChildInRepeatItem = Node.Kind == ECkUiNodeKind::Repeat ? true : bInRepeatItem;
            const bool bChildInCell = Node.Kind == ECkUiNodeKind::TableColumn || Node.Kind == ECkUiNodeKind::Tree ? true : bInTableCell;
            if (!ValidateFieldScope(Child, bChildInRepeatItem, bChildInCell, Node.Kind == ECkUiNodeKind::Table, State)) { return false; }
        }
        return true;
    }
}

auto FCkUiDocumentParser::TryParse(const FString& InMarkup, const FString& InStylesheet, const TMap<FString, FString>& InTokens, FCkUiDocument& OutDocument, const FString& InSource, TSharedPtr<const FCkUiWidgetRegistrySnapshot> InCustomRegistry) -> FCkUiLoadResult
{
    using namespace ck_ui_document;
    auto Result = FCkUiLoadResult{};
    if (InMarkup.Len() > MaxSourceChars || InStylesheet.Len() > MaxSourceChars)
    { Result.Errors.Add(InSource + TEXT(": source exceeds 1 MiB limit")); return Result; }
    if (InMarkup.Contains(TEXT("<?")) || InMarkup.Contains(TEXT("<!")))
    { Result.Errors.Add(InSource + TEXT(": XML declarations and directives are unsupported")); return Result; }
    if (!HasExactlyOneXmlRoot(InMarkup))
    { Result.Errors.Add(InSource + TEXT(": XML must contain exactly one fully consumed root")); return Result; }
    auto State = FParseState{.Tokens = InTokens, .Source = InSource, .Errors = Result.Errors, .CustomRegistry = InCustomRegistry};
    auto Rules = TArray<FStyleRule>{};
    if (!ParseStylesheet(InStylesheet, Rules, Result.Errors, InSource)) { return Result; }
    for (const auto& Rule : Rules) for (const auto& Declaration : Rule.Declarations)
    { if (!ValidateDeclaration(Declaration.Key, Declaration.Value, State)) { return Result; } }
    auto Xml = FXmlFile(InMarkup, EConstructMethod::ConstructFromBuffer);
    if (!Xml.IsValid() || Xml.GetRootNode() == nullptr) { State.Error(TEXT("invalid XML: ") + Xml.GetLastError()); return Result; }
    const FXmlNode* Root = Xml.GetRootNode();
    if (Root->GetTag() != TEXT("ui") || Root->GetAttribute(TEXT("version")) != TEXT("1")
        || Root->GetAttributes().Num() != 1 || !IsOnlyWhitespace(Root->GetContent()))
    { State.Error(TEXT("expected <ui version=\"1\">")); return Result; }
    auto Templates = TMap<FString, FTemplate>{};
    int32 DeclarationCount = 0;
    // Collect all signatures before compiling bodies so forward references are order independent.
    for (const FXmlNode* Child : Root->GetChildrenNodes())
    {
        if (Child->GetTag() != TEXT("template")) { continue; }
        const FString Name = Child->GetAttribute(TEXT("name")).TrimStartAndEnd();
        if (++DeclarationCount > MaxNodes || !IsName(Name) || Templates.Contains(Name)
            || Child->GetAttributes().Num() != 1 || !IsOnlyWhitespace(Child->GetContent()))
        { State.Error(TEXT("invalid, duplicate, or excessive template declaration")); return Result; }
        auto Definition = FTemplate{};
        auto Attributes = TSet<FString>{TEXT("id"), TEXT("template"), TEXT("class"), TEXT("visible"), TEXT("text")};
        for (const FXmlNode* Part : Child->GetChildrenNodes())
        {
            if (Part->GetTag() != TEXT("param"))
            {
                if (Definition.Source != nullptr) { State.Error(TEXT("template requires exactly one body root")); return Result; }
                Definition.Source = Part;
                continue;
            }
            const FString ParameterName = Part->GetAttribute(TEXT("name")).TrimStartAndEnd();
            EValueKind Kind = EValueKind::Text;
            if (++DeclarationCount > MaxNodes || Definition.Parameters.Num() >= 64 || Definition.Source != nullptr
                || Part->GetAttributes().Num() != 2 || !Part->GetChildrenNodes().IsEmpty() || !IsOnlyWhitespace(Part->GetContent())
                || !IsName(ParameterName) || ParameterName.EndsWith(TEXT("-param"), ESearchCase::CaseSensitive)
                || Definition.Parameters.Contains(ParameterName) || !ParseValueKind(Part->GetAttribute(TEXT("type")), Kind))
            { State.Error(TEXT("invalid, duplicate, or excessive template parameter")); return Result; }
            const FString Attribute = IsBindingKind(Kind) ? ParameterName + TEXT("-bind") : ParameterName;
            if (Attributes.Contains(Attribute) || Attributes.Contains(Attribute + TEXT("-param")))
            { State.Error(TEXT("template parameter attribute collision")); return Result; }
            Attributes.Add(Attribute);
            Attributes.Add(Attribute + TEXT("-param"));
            Definition.Parameters.Add(ParameterName, Kind);
        }
        if (Definition.Source == nullptr) { State.Error(TEXT("template requires one body root")); return Result; }
        Templates.Add(Name, MoveTemp(Definition));
    }
    for (auto& Pair : Templates)
    {
        auto LocalIds = TSet<FString>{};
        if (!CompileNode(*Pair.Value.Source, Templates, &Pair.Value.Parameters, InCustomRegistry, Rules, State,
            DeclarationCount, LocalIds, 1, Pair.Value.Body)) { return Result; }
    }
    auto Cache = TMap<FString, FSummary>{};
    for (const auto& Pair : Templates)
    {
        auto Calls = TSet<FString>{Pair.Key};
        FSummary Summary;
        if (!AnalyzeNode(Pair.Value.Body, Templates, Cache, Calls, State, 1, Summary)) { return Result; }
        Cache.Add(Pair.Key, Summary);
    }
    auto Parsed = FCkUiDocument{};
    if (!ParseMenus(*Root, Parsed, State, DeclarationCount)) { return Result; }
    for (const FXmlNode* Region : Root->GetChildrenNodes())
    {
        if (Region->GetTag() == TEXT("template") || Region->GetTag() == TEXT("menu")) { continue; }
        const FString Name = Region->GetAttribute(TEXT("name")).TrimStartAndEnd();
        if (Region->GetTag() != TEXT("region") || Region->GetAttributes().Num() != 1 || !IsOnlyWhitespace(Region->GetContent())
            || Name.IsEmpty() || Parsed.Regions.Contains(Name) || Region->GetChildrenNodes().Num() != 1)
        { State.Error(TEXT("region: unique name and exactly one root node required")); return Result; }
        auto LocalIds = TSet<FString>{};
        FAuthoredNode Compiled;
        if (!CompileNode(*Region->GetChildrenNodes()[0], Templates, nullptr, InCustomRegistry, Rules, State,
            DeclarationCount, LocalIds, 1, Compiled)) { return Result; }
        auto Calls = TSet<FString>{};
        FSummary Summary;
        if (!AnalyzeNode(Compiled, Templates, Cache, Calls, State, 1, Summary)) { return Result; }
        FCkUiNode Expanded;
        if (!ExpandNode(Compiled, Templates, {}, {}, InCustomRegistry, State, 1, Expanded)) { return Result; }
        if (!ValidateFieldScope(Expanded, false, false, false, State)) { return Result; }
        if (!ValidateTabsScope(Expanded, false, State)) { return Result; }
        Parsed.Regions.Add(Name, MoveTemp(Expanded));
    }
    if (Parsed.Regions.IsEmpty()) { State.Error(TEXT("document has no regions")); return Result; }
    if (!ValidateMenuReferences(Parsed, State)) { return Result; }
    OutDocument = MoveTemp(Parsed);
    Result.Succeeded = true;
    return Result;
}
