#pragma once

#include "CoreMinimal.h"
#include "CkSlateLayout/CkFlexWrap.h"
#include "Framework/Text/TextLayout.h"
#include "Layout/Margin.h"
#include "Styling/SlateTypes.h"
#include "Types/SlateEnums.h"

enum class ECkUiNodeKind : uint8 { Row, Column, Text, Button, Native, Search, Image, Scroll, Splitter, Custom, Table, TableColumn, Overlay, Tree, Tabs, Tab, MenuButton, Repeat };

enum class ECkUiMenuEntryKind : uint8 { Item, Separator, Submenu };

struct FCkUiMenuEntry
{
    ECkUiMenuEntryKind Kind = ECkUiMenuEntryKind::Item;
    FString Key;
    FString Label;
    FString LabelBinding;
    FString Tooltip;
    FString TooltipBinding;
    FString EnabledBinding;
    FString VisibilityBinding;
    FString Action;
    FString MenuReference;
};

struct FCkUiMenu
{
    FString Id;
    TArray<FCkUiMenuEntry> Entries;
};

enum class ECkUiCustomPropertyKind : uint8
{
    Text,
    Number,
    Bool,
    Color,
    TextBinding,
    ImageBinding,
    NumberBinding,
    BoolBinding,
    StringBinding,
    Action,
    ColorBinding,
    TextChanged,
    TextCommitted,
    BoolChanged,
    NumberChanged,
    NumberCommitted,
    NumberInteraction,
    StringChanged,
    CollectionBinding,
};

struct FCkUiCustomPropertyValue
{
    ECkUiCustomPropertyKind Kind = ECkUiCustomPropertyKind::Text;
    FText Text;
    float Number = 0.0f;
    bool Bool = false;
    FLinearColor Color = FLinearColor::White;
    FString Name;
};

enum class ECkUiCustomStyleKind : uint8 { Number, Length, Color };

struct FCkUiCustomStyleValue
{
    ECkUiCustomStyleKind Kind = ECkUiCustomStyleKind::Number;
    float Number = 0.0f;
    FLinearColor Color = FLinearColor::White;
};

struct FCkUiStyle
{
    /** Registry-declared visual properties, validated before factories or reload preparation run. */
    TMap<FString, FCkUiCustomStyleValue> CustomProperties;
    float Gap = 0.0f;
    ECkFlexWrap FlexWrap = ECkFlexWrap::NoWrap;
    FMargin Padding = FMargin(0.0f);
    float Grow = 0.0f;
    float Shrink = 0.0f;
    float MinWidth = 0.0f;
    float MinHeight = 0.0f;
    TOptional<float> MaxWidth;
    TOptional<float> MaxHeight;
    TOptional<float> FontSize;
    bool Bold = false;
    bool AllowWrapping = true;
    ETextWrappingPolicy WrappingPolicy = ETextWrappingPolicy::DefaultWrapping;
    ETextOverflowPolicy OverflowPolicy = ETextOverflowPolicy::Clip;
    TOptional<FLinearColor> Color;
    TOptional<FLinearColor> Background;
    EHorizontalAlignment HAlign = HAlign_Fill;
    EVerticalAlignment VAlign = VAlign_Fill;
};

struct FCkUiNode
{
    FString Id;
    ECkUiNodeKind Kind = ECkUiNodeKind::Column;
    FString Binding;
    FString VisibilityBinding;
    FString ColorBinding;
    FString TooltipBinding;
    FString Tooltip;
    EOrientation ScrollDirection = Orient_Vertical;
    EOrientation SplitterDirection = Orient_Horizontal;
    EOrientation RepeatDirection = Orient_Vertical;
    FString Placeholder;
    FString PlaceholderBinding;
    FString Action;
    /** Per-item action emitted by a button rendered under a repeat item subtree. */
    FString ItemAction;
    // Ordinary button enabled state; resolved by the owning view.
    FString ButtonEnabledBinding;
    FString Text;
    FString CustomTag;
    FString CustomSlotName;
    // Table configuration and typed row-field references; resolved by the owning view.
    FString Header;
    FString HeaderBinding;
    FString SortField;
    FString FilterBinding;
    FString SelectionAction;
    FString ContextMenuAction;
    FString ContextMenuReference;
    // Tabs configuration. The owning view resolves these bindings and retains panels by TabKey.
    FString TabKey;
    FString TabLabelBinding;
    FString TabEnabledBinding;
    FString MenuReference;
    bool TableSelectable = true;
    float RowHeight = 24.0f;
    TMap<FString, FString> FieldBindings;
    TMap<FString, FCkUiCustomPropertyValue> CustomProperties;
    FCkUiStyle Style;
    TArray<FCkUiNode> Children;
};

struct FCkUiDocument
{
    // Named roots mount into retained native regions, such as splitter panes.
    TMap<FString, FCkUiNode> Regions;
    TMap<FString, FCkUiMenu> Menus;
};

struct FCkUiLoadResult
{
    bool Succeeded = false;
    TArray<FString> Errors;
};

class CKSLATELAYOUT_API FCkUiDocumentParser
{
public:
    // Whole-or-nothing: OutDocument is unchanged on any error. Tokens use var(--name).
    static auto TryParse(const FString& InMarkup, const FString& InStylesheet,
        const TMap<FString, FString>& InTokens, FCkUiDocument& OutDocument,
        const FString& InSource = TEXT("<memory>"),
        TSharedPtr<const class FCkUiWidgetRegistrySnapshot> InCustomRegistry = {}) -> FCkUiLoadResult;
};
