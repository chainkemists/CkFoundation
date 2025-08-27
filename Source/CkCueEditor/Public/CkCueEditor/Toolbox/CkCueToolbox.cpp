#include "CkCueToolbox.h"

#include "CkCore/Algorithms/CkAlgorithms.h"

#include <Widgets/Layout/SScrollBox.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/Layout/SSeparator.h>
#include <EditorStyleSet.h>
#include <Framework/Application/SlateApplication.h>
#include <AssetRegistry/AssetRegistryModule.h>
#include <ContentBrowserModule.h>
#include <IContentBrowserSingleton.h>
#include <Subsystems/AssetEditorSubsystem.h>

// --------------------------------------------------------------------------------------------------------------------

namespace CkCueToolbox_Constants
{
    constexpr float PanelPadding = 8.0f;
    constexpr float SectionSpacing = 8.0f;
    constexpr float ButtonWidth = 70.0f;
    constexpr float ComboBoxWidth = 200.0f;
    constexpr float ListViewHeight = 400.0f;
}

// --------------------------------------------------------------------------------------------------------------------

auto FCkCueDiscovery::Request_DiscoverSubsystems() -> void
{
    _DiscoveredSubsystems.Empty();

    for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
    {
        const auto Class = *ClassIterator;

        if (NOT ck::IsValid(Class))
        { continue; }

        if (Class->HasAnyClassFlags(CLASS_Abstract))
        { continue; }

        if (NOT Class->IsChildOf<UCk_CueSubsystem_Base_UE>())
        { continue; }

        if (Class == UCk_CueSubsystem_Base_UE::StaticClass())
        { continue; }

        const auto CueSubsystem = GEngine->GetEngineSubsystemBase(Class);
        const auto CastedCueSubsystem = Cast<UCk_CueSubsystem_Base_UE>(CueSubsystem);

        if (ck::Is_NOT_Valid(CastedCueSubsystem))
        { continue; }

        const auto& SubsystemName = Class->GetName();
        _DiscoveredSubsystems.Add(SubsystemName, CastedCueSubsystem);
    }
}

auto FCkCueDiscovery::Request_RefreshCuesForSubsystem(UCk_CueSubsystem_Base_UE* InSubsystem) -> void
{
    _AllCueInfos.Empty();

    if (ck::Is_NOT_Valid(InSubsystem))
    { return; }

    InSubsystem->Request_PopulateAllCues();

    const auto& DiscoveredCues = InSubsystem->Get_DiscoveredCues();
    const auto& SubsystemName = InSubsystem->GetClass()->GetName();

    for (const auto& [CueName, CueClass] : DiscoveredCues)
    {
        const auto CueInfo = MakeShared<FCk_CueToolbox_CueInfo>(CueName, CueClass, SubsystemName);
        _AllCueInfos.Add(CueInfo);
    }

    Request_ValidateCues();
    Request_ApplySearchFilter(FString{});
}

auto FCkCueDiscovery::Request_ValidateCues() -> void
{
    _TotalCuesCount = _AllCueInfos.Num();
    _DuplicateCuesCount = 0;
    _InvalidCuesCount = 0;

    TMap<FGameplayTag, int32> CueNameCounts;

    for (const auto& CueInfo : _AllCueInfos)
    {
        if (NOT CueInfo.IsValid())
        { continue; }

        CueInfo->IsValid = true;
        CueInfo->ValidationMessage.Empty();

        if (ck::Is_NOT_Valid(CueInfo->CueClass))
        {
            CueInfo->IsValid = false;
            CueInfo->ValidationMessage = TEXT("Invalid cue class");
            _InvalidCuesCount++;
        }

        CueNameCounts.FindOrAdd(CueInfo->CueName, 0)++;
    }

    for (const auto& CueInfo : _AllCueInfos)
    {
        if (NOT CueInfo.IsValid())
        { continue; }

        const auto* NameCount = CueNameCounts.Find(CueInfo->CueName);

        if (NameCount && *NameCount > 1)
        {
            CueInfo->IsValid = false;
            CueInfo->ValidationMessage += (CueInfo->ValidationMessage.IsEmpty() ? TEXT("") : TEXT("; ")) +
                FString::Printf(TEXT("Duplicate name (found %d times)"), *NameCount);

            if (_DuplicateCuesCount == 0)
            {
                _DuplicateCuesCount = CueNameCounts.Num() -
                    ck::algo::CountIf(CueNameCounts, [](const auto& Pair) { return Pair.Value == 1; });
            }
        }
    }
}

auto FCkCueDiscovery::Request_ApplySearchFilter(const FString& InSearchText) -> void
{
    _FilteredCueInfos.Empty();

    if (InSearchText.IsEmpty())
    {
        _FilteredCueInfos = _AllCueInfos;
    }
    else
    {
        const auto SearchTextLower = InSearchText.ToLower();

        _FilteredCueInfos = ck::algo::Filter(_AllCueInfos,
        [&](const TSharedPtr<FCk_CueToolbox_CueInfo>& CueInfo) -> bool
        {
            if (NOT CueInfo.IsValid())
            { return false; }

            const auto CueNameString = CueInfo->CueName.ToString().ToLower();
            return CueNameString.Contains(SearchTextLower);
        });
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto SCkCueListRow::Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView) -> void
{
    _CueInfo = InArgs._CueInfo;

    SMultiColumnTableRow<TSharedPtr<FCk_CueToolbox_CueInfo>>::Construct(
        FSuperRowType::FArguments()
            .Padding(1.0f),
        InOwnerTableView
    );
}

auto SCkCueListRow::GenerateWidgetForColumn(const FName& ColumnName) -> TSharedRef<SWidget>
{
    if (NOT _CueInfo.IsValid())
    {
        return SNew(STextBlock).Text(FText::FromString(TEXT("Invalid")));
    }

    if (ColumnName == TEXT("Status"))
    {
        const auto StatusText = _CueInfo->IsValid ? TEXT("✓") : TEXT("❌");
        const auto StatusColor = _CueInfo->IsValid ? FLinearColor::Green : FLinearColor::Red;

        return SNew(STextBlock)
            .Text(FText::FromString(StatusText))
            .ColorAndOpacity(StatusColor)
            .ToolTipText(FText::FromString(_CueInfo->ValidationMessage));
    }

    if (ColumnName == TEXT("Name"))
    {
        return SNew(STextBlock)
            .Text(FText::FromString(_CueInfo->CueName.ToString()))
            .Font(FCkCueToolboxStyle::Get().GetFontStyle("CkCueToolbox.Font.Bold"));
    }

    if (ColumnName == TEXT("Class"))
    {
        const auto ClassName = _CueInfo->CueClass ? _CueInfo->CueClass->GetName() : TEXT("None");
        return SNew(STextBlock)
            .Text(FText::FromString(ClassName))
            .Font(FCkCueToolboxStyle::Get().GetFontStyle("CkCueToolbox.Font.Regular"));
    }

    if (ColumnName == TEXT("Actions"))
    {
        return SNew(SHorizontalBox)

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(2.0f)
        [
            SNew(SButton)
            .ButtonStyle(&FCkCueToolboxStyle::Get().GetWidgetStyle<FButtonStyle>("CkCueToolbox.Button.Small"))
            .Text(FText::FromString(TEXT("Edit")))
            .IsEnabled(_CueInfo->IsValid)
            .OnClicked_Lambda([this]()
            {
                DoOnEditClicked();
                return FReply::Handled();
            })
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(2.0f)
        [
            SNew(SButton)
            .ButtonStyle(&FCkCueToolboxStyle::Get().GetWidgetStyle<FButtonStyle>("CkCueToolbox.Button.Small"))
            .Text(FText::FromString(TEXT("Locate")))
            .IsEnabled(_CueInfo->IsValid)
            .OnClicked_Lambda([this]()
            {
                DoOnLocateClicked();
                return FReply::Handled();
            })
        ];
    }

    return SNew(STextBlock).Text(FText::FromString(TEXT("Unknown Column")));
}

auto SCkCueListRow::DoOnEditClicked() -> void
{
    if (NOT _CueInfo.IsValid())
    { return; }

    const auto CueClass = _CueInfo->CueClass;

    if (ck::IsValid(CueClass))
    {
        TArray<UObject*> Assets = {CueClass->GetDefaultObject()};
        GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAssets(Assets);
    }
}

auto SCkCueListRow::DoOnLocateClicked() -> void
{
    if (NOT _CueInfo.IsValid())
    { return; }

    const auto CueClass = _CueInfo->CueClass;

    if (ck::IsValid(CueClass))
    {
        const auto& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
        TArray<FAssetData> AssetsToSync = {};

        for (TObjectIterator<UBlueprint> BlueprintIterator; BlueprintIterator; ++BlueprintIterator)
        {
            const auto Blueprint = *BlueprintIterator;

            if (ck::IsValid(Blueprint) && ck::IsValid(Blueprint->GeneratedClass) && Blueprint->GeneratedClass == CueClass)
            {
                AssetsToSync.Add(FAssetData(Blueprint));
                break;
            }
        }

        if (AssetsToSync.Num() > 0)
        {
            ContentBrowserModule.Get().SyncBrowserToAssets(AssetsToSync);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto SCkCueToolbox::Construct(const FArguments& InArgs) -> void
{
    _CueDiscovery = MakeShared<FCkCueDiscovery>();

    ChildSlot
    [
        SNew(SBorder)
        .BorderImage(&FCkCueToolboxStyle::Get().GetWidgetStyle<FTableRowStyle>("CkCueToolbox.TableRow.Normal").EvenRowBackgroundBrush)
        .Padding(CkCueToolbox_Constants::PanelPadding)
        [
            SNew(SScrollBox)
            .Orientation(Orient_Vertical)

            + SScrollBox::Slot()
            [
                SNew(SVerticalBox)

                + SVerticalBox::Slot()
                .AutoHeight()
                .Padding(0, 0, 0, CkCueToolbox_Constants::SectionSpacing)
                [
                    DoCreateSubsystemPanel()
                ]

                + SVerticalBox::Slot()
                .FillHeight(1.0f)
                [
                    DoCreateCueListPanel()
                ]
            ]
        ]
    ];

    _CueDiscovery->Request_DiscoverSubsystems();
    DoRefreshCueList();
}

auto SCkCueToolbox::DoCreateHeaderPanel() -> TSharedRef<SWidget>
{
    return SNew(SBorder)
        .BorderImage(&FCkCueToolboxStyle::Get().GetWidgetStyle<FTableRowStyle>("CkCueToolbox.TableRow.Normal").EvenRowBackgroundBrush)
        .Padding(CkCueToolbox_Constants::PanelPadding)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Cue Toolbox")))
                .Font(FCkCueToolboxStyle::Get().GetFontStyle("CkCueToolbox.Font.Header"))
                .ColorAndOpacity(FCkCueToolboxStyle::Get().GetColor("CkCueToolbox.Color.Accent"))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 4, 0, 0)
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Discover, validate, and manage gameplay cues")))
                .Font(FCkCueToolboxStyle::Get().GetFontStyle("CkCueToolbox.Font.Regular"))
                .ColorAndOpacity(FCkCueToolboxStyle::Get().GetColor("CkCueToolbox.Color.Secondary"))
            ]
        ];
}

auto SCkCueToolbox::DoCreateSubsystemPanel() -> TSharedRef<SWidget>
{
    _SubsystemOptions.Empty();
    for (const auto& [SubsystemName, Subsystem] : _CueDiscovery->Get_DiscoveredSubsystems())
    {
        _SubsystemOptions.Add(MakeShared<FString>(SubsystemName));
    }

    return SNew(SHorizontalBox)

        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        .Padding(0, 0, 8, 0)
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("Subsystem Selection")))
            .Font(FCkCueToolboxStyle::Get().GetFontStyle("CkCueToolbox.Font.Bold"))
            .ColorAndOpacity(FCkCueToolboxStyle::Get().GetColor("CkCueToolbox.Color.Primary"))
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .WidthOverride(CkCueToolbox_Constants::ComboBoxWidth)
            [
                SAssignNew(_SubsystemComboBox, SComboBox<TSharedPtr<FString>>)
                .OptionsSource(&_SubsystemOptions)
                .OnGenerateWidget_Lambda([](TSharedPtr<FString> InOption)
                {
                    return SNew(STextBlock).Text(FText::FromString(*InOption));
                })
                .OnSelectionChanged(this, &SCkCueToolbox::DoOnSubsystemChanged)
                [
                    SNew(STextBlock)
                    .Text_Lambda([this]()
                    {
                        const auto Selected = _SubsystemComboBox->GetSelectedItem();
                        return Selected.IsValid() ? FText::FromString(*Selected) : FText::FromString(TEXT("Select Subsystem"));
                    })
                ]
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(8, 0, 0, 0)
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .WidthOverride(CkCueToolbox_Constants::ButtonWidth)
            [
                SNew(SButton)
                .ButtonStyle(&FCkCueToolboxStyle::Get().GetWidgetStyle<FButtonStyle>("CkCueToolbox.Button.Primary"))
                .Text(FText::FromString(TEXT("Refresh")))
                .OnClicked(this, &SCkCueToolbox::DoOnRefreshClicked)
            ]
        ]

        + SHorizontalBox::Slot()
        .AutoWidth()
        .Padding(4, 0, 0, 0)
        .VAlign(VAlign_Center)
        [
            SNew(SBox)
            .WidthOverride(CkCueToolbox_Constants::ButtonWidth + 20)
            [
                SNew(SButton)
                .ButtonStyle(&FCkCueToolboxStyle::Get().GetWidgetStyle<FButtonStyle>("CkCueToolbox.Button.Secondary"))
                .Text(FText::FromString(TEXT("Refresh All")))
                .OnClicked(this, &SCkCueToolbox::DoOnRefreshAllClicked)
            ]
        ];
}

auto SCkCueToolbox::DoCreateCueListPanel() -> TSharedRef<SWidget>
{
    return SNew(SBorder)
        .BorderImage(&FCkCueToolboxStyle::Get().GetWidgetStyle<FTableRowStyle>("CkCueToolbox.TableRow.Normal").EvenRowBackgroundBrush)
        .Padding(CkCueToolbox_Constants::PanelPadding)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text_Lambda([this]()
                {
                    const auto [TotalCount, DuplicateCount, InvalidCount] = _CueDiscovery->Get_ValidationStats();

                    FString Title = FString::Printf(TEXT("Discovered %d cues"), TotalCount);

                    if (DuplicateCount > 0 || InvalidCount > 0)
                    {
                        Title += TEXT(" (");
                        if (DuplicateCount > 0) Title += FString::Printf(TEXT("%d duplicates"), DuplicateCount);
                        if (DuplicateCount > 0 && InvalidCount > 0) Title += TEXT(", ");
                        if (InvalidCount > 0) Title += FString::Printf(TEXT("%d invalid"), InvalidCount);
                        Title += TEXT(")");
                    }

                    return FText::FromString(Title);
                })
                .Font(FCkCueToolboxStyle::Get().GetFontStyle("CkCueToolbox.Font.Bold"))
                .ColorAndOpacity(FCkCueToolboxStyle::Get().GetColor("CkCueToolbox.Color.Primary"))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 4, 0, 0)
            [
                SNew(SHorizontalBox)

                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SAssignNew(_SearchBox, SSearchBox)
                    .HintText(FText::FromString(TEXT("Search cues...")))
                    .OnTextChanged(this, &SCkCueToolbox::DoOnSearchTextChanged)
                ]
            ]

            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(0, 4, 0, 0)
            [
                SNew(SBox)
                .HeightOverride(CkCueToolbox_Constants::ListViewHeight)
                [
                    SAssignNew(_CueListView, SListView<TSharedPtr<FCk_CueToolbox_CueInfo>>)
                    .ListItemsSource(&_CueDiscovery->Get_FilteredCueInfos())
                    .OnGenerateRow_Lambda([](TSharedPtr<FCk_CueToolbox_CueInfo> InItem, const TSharedRef<STableViewBase>& OwnerTable) -> TSharedRef<ITableRow>
                    {
                        return SNew(SCkCueListRow, OwnerTable)
                            .CueInfo(InItem);
                    })
                    .HeaderRow
                    (
                        SNew(SHeaderRow)

                        + SHeaderRow::Column(TEXT("Status"))
                        .DefaultLabel(FText::FromString(TEXT("Status")))
                        .FixedWidth(50.0f)

                        + SHeaderRow::Column(TEXT("Name"))
                        .DefaultLabel(FText::FromString(TEXT("Cue Name")))
                        .FillWidth(1.0f)

                        + SHeaderRow::Column(TEXT("Class"))
                        .DefaultLabel(FText::FromString(TEXT("Class")))
                        .FillWidth(1.0f)

                        + SHeaderRow::Column(TEXT("Actions"))
                        .DefaultLabel(FText::FromString(TEXT("Actions")))
                        .FixedWidth(120.0f)
                    )
                ]
            ]
        ];
}

auto SCkCueToolbox::DoCreateValidationPanel() -> TSharedRef<SWidget>
{
    return SNew(SBorder)
        .BorderImage(&FCkCueToolboxStyle::Get().GetWidgetStyle<FTableRowStyle>("CkCueToolbox.TableRow.Normal").EvenRowBackgroundBrush)
        .Padding(CkCueToolbox_Constants::PanelPadding)
        [
            SNew(SVerticalBox)

            + SVerticalBox::Slot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(FText::FromString(TEXT("Validation")))
                .Font(FCkCueToolboxStyle::Get().GetFontStyle("CkCueToolbox.Font.Bold"))
                .ColorAndOpacity(FCkCueToolboxStyle::Get().GetColor("CkCueToolbox.Color.Warning"))
            ]

            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(0, 4, 0, 0)
            [
                SAssignNew(_ValidationText, STextBlock)
                .Text(FText::FromString(TEXT("No validation performed yet")))
                .Font(FCkCueToolboxStyle::Get().GetFontStyle("CkCueToolbox.Font.Regular"))
                .ColorAndOpacity(FCkCueToolboxStyle::Get().GetColor("CkCueToolbox.Color.Secondary"))
                .AutoWrapText(true)
            ]
        ];
}

auto SCkCueToolbox::DoOnSubsystemChanged(TSharedPtr<FString> SelectedItem, ESelectInfo::Type SelectionType) -> void
{
    if (NOT SelectedItem.IsValid())
    { return; }

    const auto& SubsystemName = *SelectedItem;
    const auto& DiscoveredSubsystems = _CueDiscovery->Get_DiscoveredSubsystems();
    const auto* FoundSubsystem = DiscoveredSubsystems.Find(SubsystemName);

    if (NOT FoundSubsystem || NOT FoundSubsystem->IsValid())
    { return; }

    _CueDiscovery->Request_RefreshCuesForSubsystem(FoundSubsystem->Get());
    DoRefreshCueList();
    DoUpdateValidationDisplay();
}

auto SCkCueToolbox::DoOnSearchTextChanged(const FText& InText) -> void
{
    const auto SearchText = InText.ToString();
    _CueDiscovery->Request_ApplySearchFilter(SearchText);
    DoRefreshCueList();
}

auto SCkCueToolbox::DoOnRefreshClicked() -> FReply
{
    const auto SelectedSubsystem = _SubsystemComboBox->GetSelectedItem();

    if (NOT SelectedSubsystem.IsValid())
    { return FReply::Handled(); }

    const auto& SubsystemName = *SelectedSubsystem;
    const auto& DiscoveredSubsystems = _CueDiscovery->Get_DiscoveredSubsystems();
    const auto* FoundSubsystem = DiscoveredSubsystems.Find(SubsystemName);

    if (FoundSubsystem && FoundSubsystem->IsValid())
    {
        _CueDiscovery->Request_RefreshCuesForSubsystem(FoundSubsystem->Get());
        DoRefreshCueList();
        DoUpdateValidationDisplay();
    }

    return FReply::Handled();
}

auto SCkCueToolbox::DoOnRefreshAllClicked() -> FReply
{
    _CueDiscovery->Request_DiscoverSubsystems();

    _SubsystemOptions.Empty();
    for (const auto& [SubsystemName, Subsystem] : _CueDiscovery->Get_DiscoveredSubsystems())
    {
        _SubsystemOptions.Add(MakeShared<FString>(SubsystemName));

        if (Subsystem.IsValid())
        {
            Subsystem->Request_PopulateAllCues();
        }
    }

    _SubsystemComboBox->RefreshOptions();

    if (_SubsystemOptions.Num() > 0)
    {
        _SubsystemComboBox->SetSelectedItem(_SubsystemOptions[0]);
        DoOnSubsystemChanged(_SubsystemOptions[0], ESelectInfo::Direct);
    }

    return FReply::Handled();
}

auto SCkCueToolbox::DoUpdateValidationDisplay() -> void
{
    const auto [TotalCount, DuplicateCount, InvalidCount] = _CueDiscovery->Get_ValidationStats();

    FString ValidationMessage = FString::Printf(TEXT("✓ %d cues discovered"), TotalCount);

    if (DuplicateCount > 0)
    {
        ValidationMessage += FString::Printf(TEXT("\n⚠ %d cues with duplicate names"), DuplicateCount);
    }

    if (InvalidCount > 0)
    {
        ValidationMessage += FString::Printf(TEXT("\n❌ %d cues with invalid class"), InvalidCount);
    }

    if (_ValidationText.IsValid())
    {
        _ValidationText->SetText(FText::FromString(ValidationMessage));
    }
}

auto SCkCueToolbox::DoRefreshCueList() -> void
{
    if (_CueListView.IsValid())
    {
        _CueListView->RequestListRefresh();
    }
}

// --------------------------------------------------------------------------------------------------------------------
