#include "CkSlateLayout/CkUiWidgetRegistry.h"

#include "CkUiWidgetRegistry.h"
#include "Misc/Char.h"

namespace ck_ui_widget_registry
{
    auto Schemas() -> const TArray<FCkUiBuiltinWidgetSchema>&
    {
        static const TArray<FCkUiBuiltinWidgetSchema> Value = {
            {.Kind = ECkUiNodeKind::Row, .Tag = TEXT("row"), .MaxChildren = INDEX_NONE, .bAllowsGap = true, .bAllowsPadding = true, .bAllowsBackground = true},
            {.Kind = ECkUiNodeKind::Column, .Tag = TEXT("column"), .MaxChildren = INDEX_NONE, .bAllowsGap = true, .bAllowsPadding = true, .bAllowsBackground = true},
            {.Kind = ECkUiNodeKind::Text, .Tag = TEXT("text"), .BindingKind = ECkUiBuiltinBindingKind::OptionalText, .bAllowsLiteralText = true, .bAllowsTextStyle = true},
            {.Kind = ECkUiNodeKind::Button, .Tag = TEXT("button"), .BindingKind = ECkUiBuiltinBindingKind::OptionalText, .bAllowsAction = true, .bRequiresAction = true, .bAllowsLiteralText = true, .bAllowsTextStyle = true},
            {.Kind = ECkUiNodeKind::MenuButton, .Tag = TEXT("menu-button"), .MaxChildren = 0, .bAllowsTextStyle = true},
            {.Kind = ECkUiNodeKind::Native, .Tag = TEXT("native"), .BindingKind = ECkUiBuiltinBindingKind::Native},
            {.Kind = ECkUiNodeKind::Search, .Tag = TEXT("search"), .BindingKind = ECkUiBuiltinBindingKind::SearchText, .bAllowsPlaceholder = true},
            {.Kind = ECkUiNodeKind::Image, .Tag = TEXT("image"), .BindingKind = ECkUiBuiltinBindingKind::Image},
            {.Kind = ECkUiNodeKind::Scroll, .Tag = TEXT("scroll"), .MinChildren = 1, .MaxChildren = 1, .bAllowsGap = true, .bRequiresZeroGap = true, .bAllowsPadding = true, .bAllowsBackground = true},
            {.Kind = ECkUiNodeKind::Splitter, .Tag = TEXT("splitter"), .MinChildren = 2, .MaxChildren = INDEX_NONE, .bAllowsGap = true, .bRequiresZeroGap = true, .bAllowsPadding = true, .bAllowsBackground = true},
            {.Kind = ECkUiNodeKind::Repeat, .Tag = TEXT("repeat"), .BindingKind = ECkUiBuiltinBindingKind::Collection, .MinChildren = 1, .MaxChildren = 1, .bAllowsGap = true, .bAllowsPadding = true, .bAllowsBackground = true},
            {.Kind = ECkUiNodeKind::Table, .Tag = TEXT("table"), .BindingKind = ECkUiBuiltinBindingKind::Collection, .MinChildren = 1, .MaxChildren = 64, .bAllowsPadding = true, .bAllowsBackground = true},
            {.Kind = ECkUiNodeKind::TableColumn, .Tag = TEXT("table-column"), .MinChildren = 1, .MaxChildren = 1, .bAllowsTextStyle = true},
            {.Kind = ECkUiNodeKind::Tree, .Tag = TEXT("tree"), .BindingKind = ECkUiBuiltinBindingKind::TreeCollection, .MinChildren = 1, .MaxChildren = 1, .bAllowsPadding = true, .bAllowsBackground = true},
            {.Kind = ECkUiNodeKind::Overlay, .Tag = TEXT("overlay"), .MinChildren = 1, .MaxChildren = INDEX_NONE, .bAllowsPadding = true, .bAllowsBackground = true},
            {.Kind = ECkUiNodeKind::Tabs, .Tag = TEXT("tabs"), .MinChildren = 1, .MaxChildren = INDEX_NONE, .bAllowsPadding = true, .bAllowsBackground = true},
            {.Kind = ECkUiNodeKind::Tab, .Tag = TEXT("tab"), .MaxChildren = INDEX_NONE, .bAllowsGap = true, .bAllowsPadding = true, .bAllowsBackground = true, .bAllowsTextStyle = true},
        };
        return Value;
    }

    auto FindByTag(const FString& InTag) -> const FCkUiBuiltinWidgetSchema*
    {
        for (const FCkUiBuiltinWidgetSchema& Schema : Schemas())
        {
            if (InTag == Schema.Tag) { return &Schema; }
        }
        return nullptr;
    }

    auto FindByKind(const ECkUiNodeKind InKind) -> const FCkUiBuiltinWidgetSchema*
    {
        for (const FCkUiBuiltinWidgetSchema& Schema : Schemas())
        {
            if (InKind == Schema.Kind) { return &Schema; }
        }
        return nullptr;
    }
}

namespace ck_ui_widget_registry_public
{
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

    auto Error(const FString& InMessage) -> FCkUiLoadResult
    {
        auto Result = FCkUiLoadResult{};
        Result.Errors.Add(InMessage);
        return Result;
    }
}

auto FCkUiWidgetRegistrySnapshot::Find(const FString& InTag) const -> const FCkUiCustomWidgetRegistration*
{
    return _Registrations.Find(InTag);
}

auto FCkUiWidgetRegistrySnapshot::FindStyleProperty(const FString& InName) const -> const ECkUiCustomStyleKind*
{
    return _StyleProperties.Find(InName);
}

auto FCkUiWidgetRegistry::Register(FCkUiCustomWidgetRegistration InRegistration) -> FCkUiLoadResult
{
    const FCkUiCustomWidgetSchema& Schema = InRegistration.Schema;
    if (Schema.Properties.Num() > 64)
    { return ck_ui_widget_registry_public::Error(TEXT("Custom widget schema may declare at most 64 properties.")); }
    if (Schema.Slots.Num() > 16)
    { return ck_ui_widget_registry_public::Error(TEXT("Custom widget schema may declare at most 16 slots.")); }
    if (!Schema.Slots.IsEmpty() && !InRegistration.RetainedFactory)
    { return ck_ui_widget_registry_public::Error(TEXT("Custom widget slots require a retained factory.")); }
    auto SlotNames = TSet<FString>{};
    for (const FCkUiCustomSlotSchema& Slot : Schema.Slots)
    {
        if (!ck_ui_widget_registry_public::IsName(Slot.Name) || SlotNames.Contains(Slot.Name))
        { return ck_ui_widget_registry_public::Error(TEXT("Custom widget slots require unique identifier names.")); }
        SlotNames.Add(Slot.Name);
    }
    if (!ck_ui_widget_registry_public::IsName(Schema.Tag))
    { return ck_ui_widget_registry_public::Error(TEXT("Custom widget tag must be a non-empty identifier.")); }
    if (Schema.Tag != Schema.Tag.ToLower())
    { return ck_ui_widget_registry_public::Error(TEXT("Custom widget tag must be lowercase.")); }
    if (Schema.Tag == TEXT("ui") || Schema.Tag == TEXT("region") || Schema.Tag == TEXT("template")
        || Schema.Tag == TEXT("param") || Schema.Tag == TEXT("use") || Schema.Tag == TEXT("slot"))
    { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget tag '%s' is reserved."), *Schema.Tag)); }
    if (ck_ui_widget_registry::FindByTag(Schema.Tag) != nullptr)
    { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget tag '%s' conflicts with a built-in tag."), *Schema.Tag)); }
    if (static_cast<bool>(InRegistration.Factory) == static_cast<bool>(InRegistration.RetainedFactory))
    { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget '%s' requires exactly one factory."), *Schema.Tag)); }
    if (_Registrations.Contains(Schema.Tag))
    { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget tag '%s' is already registered."), *Schema.Tag)); }
    if (_Registrations.Num() >= 512)
    { return ck_ui_widget_registry_public::Error(TEXT("Custom widget registry may contain at most 512 schemas.")); }

    if (Schema.StyleProperties.Num() > 64)
    { return ck_ui_widget_registry_public::Error(TEXT("Custom widget schema may declare at most 64 style properties.")); }
    for (const auto& Property : Schema.StyleProperties)
    {
        if (!Property.Key.StartsWith(TEXT("-ck-"), ESearchCase::CaseSensitive) || Property.Key.Len() <= 4
            || !ck_ui_widget_registry_public::IsName(Property.Key) || !Property.Key.Equals(Property.Key.ToLower(), ESearchCase::CaseSensitive))
        { return ck_ui_widget_registry_public::Error(TEXT("Custom CSS property names must be lowercase identifiers with a -ck- prefix.")); }
        if (Property.Value != ECkUiCustomStyleKind::Number && Property.Value != ECkUiCustomStyleKind::Length
            && Property.Value != ECkUiCustomStyleKind::Color)
        { return ck_ui_widget_registry_public::Error(TEXT("Custom CSS property has an invalid kind.")); }
        for (const auto& Existing : _Registrations)
        {
            const ECkUiCustomStyleKind* Kind = Existing.Value.Schema.StyleProperties.Find(Property.Key);
            if (Kind != nullptr && *Kind != Property.Value)
            { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom CSS property '%s' has conflicting kinds."), *Property.Key)); }
        }
    }

    auto Names = TSet<FString>{};
    auto Attributes = TSet<FString>{TEXT("id"), TEXT("class"), TEXT("visible")};
    for (const FCkUiCustomPropertySchema& Property : Schema.Properties)
    {
        if (!ck_ui_widget_registry_public::IsName(Property.Name))
        { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget '%s' has an invalid property name."), *Schema.Tag)); }
        if (Names.Contains(Property.Name))
        { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget '%s' declares property '%s' more than once."), *Schema.Tag, *Property.Name)); }
        if (Property.Name == TEXT("id") || Property.Name == TEXT("class") || Property.Name == TEXT("visible"))
        { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget '%s' declares reserved property '%s'."), *Schema.Tag, *Property.Name)); }
        if (Property.Name.EndsWith(TEXT("-param"), ESearchCase::CaseSensitive)
            || Property.Name.EndsWith(TEXT("-field"), ESearchCase::CaseSensitive)
            || Property.Name.EndsWith(TEXT("-bind"), ESearchCase::CaseSensitive))
        { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget '%s' property '%s' conflicts with generated binding or table field syntax."), *Schema.Tag, *Property.Name)); }
        switch (Property.Kind)
        {
        case ECkUiCustomPropertyKind::Text:
        case ECkUiCustomPropertyKind::Number:
        case ECkUiCustomPropertyKind::Bool:
        case ECkUiCustomPropertyKind::Color:
        case ECkUiCustomPropertyKind::Action:
        case ECkUiCustomPropertyKind::TextChanged:
        case ECkUiCustomPropertyKind::TextCommitted:
        case ECkUiCustomPropertyKind::BoolChanged:
        case ECkUiCustomPropertyKind::NumberChanged:
        case ECkUiCustomPropertyKind::NumberCommitted:
        case ECkUiCustomPropertyKind::IntegerCommitted:
        case ECkUiCustomPropertyKind::ColorCommitted:
        case ECkUiCustomPropertyKind::NumberInteraction:
        case ECkUiCustomPropertyKind::StringChanged:
            if (Attributes.Contains(Property.Name))
            { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget '%s' has colliding attribute '%s'."), *Schema.Tag, *Property.Name)); }
            Attributes.Add(Property.Name);
            break;
        case ECkUiCustomPropertyKind::TextBinding:
        case ECkUiCustomPropertyKind::ImageBinding:
        case ECkUiCustomPropertyKind::NumberBinding:
        case ECkUiCustomPropertyKind::IntegerBinding:
        case ECkUiCustomPropertyKind::BoolBinding:
        case ECkUiCustomPropertyKind::StringBinding:
        case ECkUiCustomPropertyKind::ColorBinding:
        case ECkUiCustomPropertyKind::CollectionBinding:
        case ECkUiCustomPropertyKind::FloatSeriesBinding:
        {
            const FString Attribute = Property.Name + TEXT("-bind");
            if (Attributes.Contains(Attribute))
            { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget '%s' has colliding attribute '%s'."), *Schema.Tag, *Attribute)); }
            Attributes.Add(Attribute);
            break;
        }
        default:
            return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget '%s' has an invalid property kind."), *Schema.Tag));
        }
        Names.Add(Property.Name);
    }
    if (!Schema.StateKeyProperty.IsEmpty())
    {
        if (!InRegistration.RetainedFactory)
        { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget '%s' state key requires a retained factory."), *Schema.Tag)); }
        const FCkUiCustomPropertySchema* StateKey = Schema.Properties.FindByPredicate([&Schema](const FCkUiCustomPropertySchema& Property)
        { return Property.Name == Schema.StateKeyProperty; });
        if (StateKey == nullptr || StateKey->Kind != ECkUiCustomPropertyKind::Text || !StateKey->bRequired)
        { return ck_ui_widget_registry_public::Error(FString::Printf(TEXT("Custom widget '%s' state key must name a required literal Text property."), *Schema.Tag)); }
    }

    const FString Tag = Schema.Tag;
    _Registrations.Add(Tag, MoveTemp(InRegistration));
    auto Result = FCkUiLoadResult{};
    Result.Succeeded = true;
    return Result;
}

auto FCkUiWidgetRegistry::CreateSnapshot() const -> TSharedRef<const FCkUiWidgetRegistrySnapshot>
{
    const TSharedRef<FCkUiWidgetRegistrySnapshot> Snapshot = MakeShared<FCkUiWidgetRegistrySnapshot>();
    Snapshot->_Registrations = _Registrations;
    for (const auto& Registration : _Registrations)
    {
        for (const auto& Property : Registration.Value.Schema.StyleProperties)
        { Snapshot->_StyleProperties.Add(Property.Key, Property.Value); }
    }
    return Snapshot;
}
