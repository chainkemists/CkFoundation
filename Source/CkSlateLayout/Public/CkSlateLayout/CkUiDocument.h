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
    ColorCommitted,
    NumberInteraction,
    StringChanged,
    CollectionBinding,
    FloatSeriesBinding,
    IntegerBinding,
    IntegerCommitted,
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
    /** CSS em tracking converted to Slate's one-thousandth-em letter spacing. */
    TOptional<int32> LetterSpacing;
    /** Optional face selection; omitted inherits the consumer's base font. */
    TOptional<bool> Monospace;
    bool Bold = false;
    bool AllowWrapping = true;
    ETextWrappingPolicy WrappingPolicy = ETextWrappingPolicy::DefaultWrapping;
    ETextOverflowPolicy OverflowPolicy = ETextOverflowPolicy::Clip;
    TOptional<FLinearColor> Color;
    TOptional<FLinearColor> Background;
    TOptional<FLinearColor> BorderColor;
    TOptional<float> BorderWidth;
    TOptional<float> BorderRadius;
    EHorizontalAlignment HAlign = HAlign_Fill;
    EVerticalAlignment VAlign = VAlign_Fill;
};

/** Optional authored presentation for a built-in button. Any -ck-button-* declaration enables it. */
struct FCkUiButtonVisualStyle
{
    bool Enabled = false;
    TOptional<FLinearColor> Background;
    TOptional<FLinearColor> BorderColor;
    TOptional<FLinearColor> HoverBackground;
    TOptional<FLinearColor> HoverBorderColor;
    TOptional<FLinearColor> PressedBackground;
    TOptional<FLinearColor> PressedBorderColor;
    TOptional<FLinearColor> DisabledBackground;
    TOptional<FLinearColor> DisabledBorderColor;
    TOptional<FLinearColor> Color;
    TOptional<FLinearColor> DisabledColor;
    TOptional<float> Radius;
    TOptional<float> OutlineWidth;
    FMargin ContentPadding = FMargin(9.0f, 6.0f);
};

/** Optional authored presentation for a built-in menu button. Any -ck-menu-button-* declaration enables it. */
struct FCkUiMenuButtonVisualStyle
{
    bool Enabled = false;
    TOptional<FLinearColor> Background;
    TOptional<FLinearColor> BorderColor;
    TOptional<FLinearColor> HoverBackground;
    TOptional<FLinearColor> HoverBorderColor;
    TOptional<FLinearColor> PressedBackground;
    TOptional<FLinearColor> PressedBorderColor;
    TOptional<FLinearColor> DisabledBackground;
    TOptional<FLinearColor> DisabledBorderColor;
    TOptional<float> Radius;
    TOptional<float> OutlineWidth;
    FMargin ContentPadding = FMargin(9.0f, 6.0f);
    bool HasContentPadding = false;
    TOptional<bool> HasDownArrow;
};

/** Optional authored presentation for a built-in tabs control. Any -ck-tabs-* declaration enables it. */
struct FCkUiTabsVisualStyle
{
    bool Enabled = false;
    TOptional<FLinearColor> InactiveColor;
    TOptional<FLinearColor> ActiveColor;
    TOptional<FLinearColor> UnderlineColor;
    TOptional<float> UnderlineHeight;
    TOptional<float> FontSize;
    TOptional<bool> Bold;
    FMargin HeaderPadding = FMargin(8.0f, 6.0f);
};

/** Optional authored presentation for a built-in table. Any -ck-table-* declaration enables it. */
struct FCkUiTableVisualStyle
{
    bool Enabled = false;
    TOptional<FLinearColor> HeaderBackground;
    TOptional<FLinearColor> SortIndicatorColor;
    FMargin HeaderPadding = FMargin(0.0f);
    bool HasHeaderPadding = false;
    TOptional<FLinearColor> RowBackground;
    TOptional<FLinearColor> RowHoverBackground;
    TOptional<FLinearColor> RowSelectedBackground;
    TOptional<FLinearColor> RowSeparatorColor;
    TOptional<float> RowSeparatorWidth;
};

/** Optional authored presentation for a built-in tree. Any -ck-tree-* declaration enables it. */
struct FCkUiTreeVisualStyle
{
    bool Enabled = false;
    TOptional<FLinearColor> RowBackground;
    TOptional<FLinearColor> RowHoverBackground;
    TOptional<FLinearColor> RowSelectedBackground;
    TOptional<FLinearColor> SelectedAccentColor;
    TOptional<float> SelectedAccentWidth;
};

struct FCkUiNode
{
    FString Id;
    ECkUiNodeKind Kind = ECkUiNodeKind::Column;
    FString Binding;
    /** Nested-repeat child collection name; resolved against the enclosing repeat record in a future surface slice. */
    FString ChildBinding;
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
    /** Custom event property to repeat-item handler name; rewritten by the innermost repeat. */
    TMap<FString, FString> ItemEventBindings;
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
    /** Tree-only Bool schema field used to project nodes without mutating the collection. */
    FString ProjectionField;
    FString SelectionAction;
    FString ContextMenuAction;
    FString ContextMenuReference;
    // Tabs configuration. The owning view resolves these bindings and retains panels by TabKey.
    FString TabKey;
    FString TabLabelBinding;
    FString TabEnabledBinding;
    FString MenuReference;
    bool TableSelectable = true;
    /** Tree-only: unmodified left clicks on parent rows toggle native expansion. */
    bool TreeExpandOnRowClick = false;
    float RowHeight = 24.0f;
    TMap<FString, FString> FieldBindings;
    TMap<FString, FCkUiCustomPropertyValue> CustomProperties;
    FCkUiStyle Style;
    FCkUiButtonVisualStyle ButtonVisualStyle;
    FCkUiMenuButtonVisualStyle MenuButtonVisualStyle;
    FCkUiTabsVisualStyle TabsVisualStyle;
    FCkUiTableVisualStyle TableVisualStyle;
    FCkUiTreeVisualStyle TreeVisualStyle;
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
