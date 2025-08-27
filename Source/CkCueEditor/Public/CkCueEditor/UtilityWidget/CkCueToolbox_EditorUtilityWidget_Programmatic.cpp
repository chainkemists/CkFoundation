#include "CkCueToolbox_EditorUtilityWidget_Programmatic.h"

#include "CkUI/Styles/CkCommonTextStyle_Header.h"
#include "CkUI/Styles/CkCommonTextStyle_Body.h"
#include "CkCore/Algorithms/CkAlgorithms.h"

#include "Blueprint/WidgetTree.h"
#include "Engine/Engine.h"

void UCkCueToolboxWidget::NativePreConstruct()
{
    Super::NativePreConstruct();
    Request_CreateLayout();
}

void UCkCueToolboxWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (ck::IsValid(SubsystemComboBox))
    {
        SubsystemComboBox->OnSelectionChanged.AddDynamic(this, &UCkCueToolboxWidget::OnSubsystemChanged);
    }

    if (ck::IsValid(SearchTextBox))
    {
        SearchTextBox->OnTextChanged.AddDynamic(this, &UCkCueToolboxWidget::OnSearchTextChanged);
    }

    DoDiscoverSubsystems();
}

void UCkCueToolboxWidget::Request_CreateLayout()
{
    if (ck::Is_NOT_Valid(ContentContainer))
    { return; }

    auto MainScrollBox = WidgetTree->ConstructWidget<UScrollBox>();
    auto MainContainer = WidgetTree->ConstructWidget<UVerticalBox>();

    MainScrollBox->AddChild(MainContainer);
    ContentContainer->AddChild(MainScrollBox);

    // Header
    auto HeaderText = WidgetTree->ConstructWidget<UCommonTextBlock>();
    HeaderText->SetText(FText::FromString(TEXT("Cue Toolbox")));
    HeaderText->SetStyle(UCkCommonTextStyle_Header::StaticClass());
    MainContainer->AddChildToVerticalBox(HeaderText);

    auto Spacer1 = WidgetTree->ConstructWidget<USpacer>();
    Spacer1->SetSize(FVector2D(0, 16));
    MainContainer->AddChildToVerticalBox(Spacer1);

    // Subsystem Selection Section
    auto SubsystemSection = Request_CreateSection(
        FText::FromString(TEXT("Subsystem Selection")),
        Request_CreateSubsystemControls()
    );
    MainContainer->AddChildToVerticalBox(SubsystemSection);

    auto Spacer2 = WidgetTree->ConstructWidget<USpacer>();
    Spacer2->SetSize(FVector2D(0, 16));
    MainContainer->AddChildToVerticalBox(Spacer2);

    // Discovered Cues Section
    auto CuesSection = Request_CreateSection(
        FText::FromString(TEXT("Discovered Cues")),
        Request_CreateCuesControls()
    );
    MainContainer->AddChildToVerticalBox(CuesSection);

    auto Spacer3 = WidgetTree->ConstructWidget<USpacer>();
    Spacer3->SetSize(FVector2D(0, 16));
    MainContainer->AddChildToVerticalBox(Spacer3);

    // Quick Actions Section
    auto ActionsSection = Request_CreateSection(
        FText::FromString(TEXT("Quick Actions")),
        Request_CreateQuickActions()
    );
    MainContainer->AddChildToVerticalBox(ActionsSection);

    auto Spacer4 = WidgetTree->ConstructWidget<USpacer>();
    Spacer4->SetSize(FVector2D(0, 16));
    MainContainer->AddChildToVerticalBox(Spacer4);

    // Validation Section
    auto ValidationSection = Request_CreateSection(
        FText::FromString(TEXT("Validation")),
        Request_CreateValidationDisplay(),
        FLinearColor(0.88f, 0.37f, 0.37f)
    );
    MainContainer->AddChildToVerticalBox(ValidationSection);

    auto Spacer5 = WidgetTree->ConstructWidget<USpacer>();
    Spacer5->SetSize(FVector2D(0, 16));
    MainContainer->AddChildToVerticalBox(Spacer5);

    // Debug Panel Section
    auto DebugSection = Request_CreateSection(
        FText::FromString(TEXT("Debug Panel")),
        Request_CreateDebugPanel(),
        FLinearColor(0.25f, 0.82f, 0.79f)
    );
    MainContainer->AddChildToVerticalBox(DebugSection);
}

auto UCkCueToolboxWidget::Request_CreateSection(
    const FText& Title,
    UWidget* Content,
    const FLinearColor& AccentColor) -> UWidget*
{
    auto SectionContainer = WidgetTree->ConstructWidget<UVerticalBox>();

    auto HeaderText = WidgetTree->ConstructWidget<UCommonTextBlock>();
    HeaderText->SetText(Title);
    HeaderText->SetStyle(UCkCommonTextStyle_Header::StaticClass());
    if (AccentColor != FLinearColor::White)
    {
        HeaderText->SetColorAndOpacity(AccentColor);
    }
    SectionContainer->AddChildToVerticalBox(HeaderText);

    auto ContentBorder = WidgetTree->ConstructWidget<UBorder>();
    ContentBorder->SetBrushColor(FLinearColor(0.14f, 0.15f, 0.16f));
    ContentBorder->SetPadding(FMargin(12.0f));
    ContentBorder->SetContent(Content);

    SectionContainer->AddChildToVerticalBox(ContentBorder);

    return SectionContainer;
}

auto UCkCueToolboxWidget::Request_CreateSubsystemControls() -> UWidget*
{
    auto Container = WidgetTree->ConstructWidget<UHorizontalBox>();

    SubsystemComboBox = WidgetTree->ConstructWidget<UComboBoxString>();
    auto ComboSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    ComboSizeBox->SetWidthOverride(300.0f);
    ComboSizeBox->SetContent(SubsystemComboBox);
    Container->AddChildToHorizontalBox(ComboSizeBox);

    RefreshButton = WidgetTree->ConstructWidget<UCkCommonButton_Primary>();
    RefreshButton->Request_SetText(FText::FromString(TEXT("Refresh")));
    auto RefreshSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    RefreshSizeBox->SetWidthOverride(80.0f);
    RefreshSizeBox->SetContent(RefreshButton);
    Container->AddChildToHorizontalBox(RefreshSizeBox);

    return Container;
}

auto UCkCueToolboxWidget::Request_CreateCuesControls() -> UWidget*
{
    auto Container = WidgetTree->ConstructWidget<UVerticalBox>();

    SearchTextBox = WidgetTree->ConstructWidget<UEditableTextBox>();
    SearchTextBox->SetHintText(FText::FromString(TEXT("Search...")));
    auto SearchSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    SearchSizeBox->SetHeightOverride(32.0f);
    SearchSizeBox->SetContent(SearchTextBox);
    Container->AddChildToVerticalBox(SearchSizeBox);

    auto SearchSpacer = WidgetTree->ConstructWidget<USpacer>();
    SearchSpacer->SetSize(FVector2D(0, 8));
    Container->AddChildToVerticalBox(SearchSpacer);

    auto FilterComboBox = WidgetTree->ConstructWidget<UComboBoxString>();
    FilterComboBox->AddOption(TEXT("Filter"));
    FilterComboBox->SetSelectedIndex(0);
    auto FilterSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    FilterSizeBox->SetHeightOverride(32.0f);
    FilterSizeBox->SetContent(FilterComboBox);
    Container->AddChildToVerticalBox(FilterSizeBox);

    auto FilterSpacer = WidgetTree->ConstructWidget<USpacer>();
    FilterSpacer->SetSize(FVector2D(0, 8));
    Container->AddChildToVerticalBox(FilterSpacer);

    CueListView = WidgetTree->ConstructWidget<UListView>();
    auto ListSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    ListSizeBox->SetHeightOverride(200.0f);
    ListSizeBox->SetContent(CueListView);
    Container->AddChildToVerticalBox(ListSizeBox);

    return Container;
}

auto UCkCueToolboxWidget::Request_CreateQuickActions() -> UWidget*
{
    auto Container = WidgetTree->ConstructWidget<UHorizontalBox>();

    CreateCueButton = WidgetTree->ConstructWidget<UCkCommonButton_Secondary>();
    CreateCueButton->Request_SetText(FText::FromString(TEXT("Create New Cue")));
    Container->AddChildToHorizontalBox(CreateCueButton);

    RefreshAllButton = WidgetTree->ConstructWidget<UCkCommonButton_Secondary>();
    RefreshAllButton->Request_SetText(FText::FromString(TEXT("Refresh All")));
    Container->AddChildToHorizontalBox(RefreshAllButton);

    OpenSettingsButton = WidgetTree->ConstructWidget<UCkCommonButton_Secondary>();
    OpenSettingsButton->Request_SetText(FText::FromString(TEXT("Open Settings")));
    Container->AddChildToHorizontalBox(OpenSettingsButton);

    return Container;
}

auto UCkCueToolboxWidget::Request_CreateValidationDisplay() -> UWidget*
{
    ValidationStatusText = WidgetTree->ConstructWidget<UCommonTextBlock>();
    ValidationStatusText->SetText(FText::FromString(TEXT("No validation performed yet")));
    ValidationStatusText->SetStyle(UCkCommonTextStyle_Body::StaticClass());

    return ValidationStatusText;
}

auto UCkCueToolboxWidget::Request_CreateDebugPanel() -> UWidget*
{
    auto Container = WidgetTree->ConstructWidget<UVerticalBox>();

    // Context Entity row
    auto ContextRow = WidgetTree->ConstructWidget<UHorizontalBox>();

    auto ContextLabel = WidgetTree->ConstructWidget<UCommonTextBlock>();
    ContextLabel->SetText(FText::FromString(TEXT("Context Entity:")));
    ContextLabel->SetStyle(UCkCommonTextStyle_Body::StaticClass());
    auto ContextLabelSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    ContextLabelSizeBox->SetWidthOverride(120.0f);
    ContextLabelSizeBox->SetContent(ContextLabel);
    ContextRow->AddChildToHorizontalBox(ContextLabelSizeBox);

    ContextEntityTextBox = WidgetTree->ConstructWidget<UEditableTextBox>();
    ContextEntityTextBox->SetHintText(FText::FromString(TEXT("Select Entity")));
    ContextRow->AddChildToHorizontalBox(ContextEntityTextBox);

    Container->AddChildToVerticalBox(ContextRow);

    auto ContextSpacer = WidgetTree->ConstructWidget<USpacer>();
    ContextSpacer->SetSize(FVector2D(0, 8));
    Container->AddChildToVerticalBox(ContextSpacer);

    // Spawn Params row
    auto ParamsRow = WidgetTree->ConstructWidget<UHorizontalBox>();

    auto ParamsLabel = WidgetTree->ConstructWidget<UCommonTextBlock>();
    ParamsLabel->SetText(FText::FromString(TEXT("Spawn Params:")));
    ParamsLabel->SetStyle(UCkCommonTextStyle_Body::StaticClass());
    auto ParamsLabelSizeBox = WidgetTree->ConstructWidget<USizeBox>();
    ParamsLabelSizeBox->SetWidthOverride(120.0f);
    ParamsLabelSizeBox->SetContent(ParamsLabel);
    ParamsRow->AddChildToHorizontalBox(ParamsLabelSizeBox);

    SpawnParamsTextBox = WidgetTree->ConstructWidget<UEditableTextBox>();
    SpawnParamsTextBox->SetHintText(FText::FromString(TEXT("Optional Struct Instance")));
    ParamsRow->AddChildToHorizontalBox(SpawnParamsTextBox);

    Container->AddChildToVerticalBox(ParamsRow);

    auto ParamsSpacer = WidgetTree->ConstructWidget<USpacer>();
    ParamsSpacer->SetSize(FVector2D(0, 8));
    Container->AddChildToVerticalBox(ParamsSpacer);

    // Execute buttons row
    auto ButtonsRow = WidgetTree->ConstructWidget<UHorizontalBox>();

    ExecuteContextButton = WidgetTree->ConstructWidget<UCkCommonButton_Primary>();
    ExecuteContextButton->Request_SetText(FText::FromString(TEXT("Execute on Context")));
    ButtonsRow->AddChildToHorizontalBox(ExecuteContextButton);

    ExecuteGlobalButton = WidgetTree->ConstructWidget<UCkCommonButton_Primary>();
    ExecuteGlobalButton->Request_SetText(FText::FromString(TEXT("Execute Global")));
    ButtonsRow->AddChildToHorizontalBox(ExecuteGlobalButton);

    ExecuteLocalButton = WidgetTree->ConstructWidget<UCkCommonButton_Primary>();
    ExecuteLocalButton->Request_SetText(FText::FromString(TEXT("Execute Local")));
    ButtonsRow->AddChildToHorizontalBox(ExecuteLocalButton);

    Container->AddChildToVerticalBox(ButtonsRow);

    return Container;
}

void UCkCueToolboxWidget::OnSubsystemChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
    DoRefreshCuesForCurrentSubsystem();
}

void UCkCueToolboxWidget::OnSearchTextChanged(const FText& Text)
{
    CurrentSearchText = Text.ToString();
    DoApplySearchFilter();
}

void UCkCueToolboxWidget::OnRefreshClicked()
{
    DoRefreshCuesForCurrentSubsystem();
}

void UCkCueToolboxWidget::OnCreateCueClicked() {}

void UCkCueToolboxWidget::OnRefreshAllClicked()
{
    DoDiscoverSubsystems();
    DoRefreshCuesForCurrentSubsystem();
}

void UCkCueToolboxWidget::OnOpenSettingsClicked() {}

void UCkCueToolboxWidget::OnExecuteContextClicked() {}

void UCkCueToolboxWidget::OnExecuteGlobalClicked() {}

void UCkCueToolboxWidget::OnExecuteLocalClicked() {}

auto UCkCueToolboxWidget::DoDiscoverSubsystems() -> void
{
    DiscoveredSubsystems.Empty();

    if (NOT ck::IsValid(SubsystemComboBox))
    { return; }

    SubsystemComboBox->ClearOptions();

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

        const auto CueSubsystemClass = Cast<UClass>(Class);
        if (ck::Is_NOT_Valid(CueSubsystemClass))
        { continue; }

        const auto CueSubsystem = GEngine->GetEngineSubsystemBase(CueSubsystemClass);
        if (ck::Is_NOT_Valid(CueSubsystem))
        { continue; }

        const auto CastedCueSubsystem = Cast<UCk_CueSubsystem_Base_UE>(CueSubsystem);

        if (ck::Is_NOT_Valid(CastedCueSubsystem))
        { continue; }

        const auto& SubsystemName = Class->GetName();
        DiscoveredSubsystems.Add(SubsystemName, CastedCueSubsystem);
        SubsystemComboBox->AddOption(SubsystemName);
    }

    if (SubsystemComboBox->GetOptionCount() > 0)
    {
        SubsystemComboBox->SetSelectedIndex(0);
        DoRefreshCuesForCurrentSubsystem();
    }
}

auto UCkCueToolboxWidget::DoRefreshCuesForCurrentSubsystem() -> void
{
    AllCueEntries.Empty();

    const auto CurrentSubsystem = DoGetCurrentSubsystem();

    if (ck::Is_NOT_Valid(CurrentSubsystem))
    { return; }

    CurrentSubsystem->Request_PopulateAllCues();

    const auto& DiscoveredCues = CurrentSubsystem->Get_DiscoveredCues();
    const auto& SubsystemName = CurrentSubsystem->GetClass()->GetName();

    for (const auto& [CueName, CueClass] : DiscoveredCues)
    {
        const auto CueInfo = FCk_CueToolbox_CueInfo{CueName, CueClass, SubsystemName};
        const auto CueEntry = NewObject<UCk_CueToolbox_CueListEntry>(this, UCk_CueToolbox_CueListEntry::StaticClass());
        CueEntry->CueInfo = CueInfo;
        AllCueEntries.Add(CueEntry);
    }

    DoValidateCues();
    DoApplySearchFilter();
}

auto UCkCueToolboxWidget::DoValidateCues() -> void
{
    TotalCuesCount = AllCueEntries.Num();
    DuplicateCuesCount = 0;
    InvalidCuesCount = 0;

    TMap<FGameplayTag, int32> CueNameCounts;

    for (const auto& CueEntry : AllCueEntries)
    {
        if (ck::Is_NOT_Valid(CueEntry))
        { continue; }

        auto& CueInfo = CueEntry->CueInfo;
        CueInfo.IsValid = true;
        CueInfo.ValidationMessage.Empty();

        if (ck::Is_NOT_Valid(CueInfo.CueClass))
        {
            CueInfo.IsValid = false;
            CueInfo.ValidationMessage = TEXT("Invalid cue class");
            InvalidCuesCount++;
        }

        CueNameCounts.FindOrAdd(CueInfo.CueName, 0)++;
    }

    for (const auto& CueEntry : AllCueEntries)
    {
        if (ck::Is_NOT_Valid(CueEntry))
        { continue; }

        auto& CueInfo = CueEntry->CueInfo;
        const auto* NameCount = CueNameCounts.Find(CueInfo.CueName);

        if (NameCount && *NameCount > 1)
        {
            CueInfo.IsValid = false;
            CueInfo.ValidationMessage += (CueInfo.ValidationMessage.IsEmpty() ? TEXT("") : TEXT("; ")) +
                FString::Printf(TEXT("Duplicate name (found %d times)"), *NameCount);

            if (DuplicateCuesCount == 0)
            {
                DuplicateCuesCount = CueNameCounts.Num() -
                    ck::algo::CountIf(CueNameCounts, [](const auto& Pair) { return Pair.Value == 1; });
            }
        }
    }

    if (ck::IsValid(ValidationStatusText))
    {
        FString ValidationMessage = FString::Printf(TEXT("✓ %d cues discovered"), TotalCuesCount);

        if (DuplicateCuesCount > 0)
        {
            ValidationMessage += FString::Printf(TEXT("\n⚠ %d cues with duplicate names"), DuplicateCuesCount);
        }

        if (InvalidCuesCount > 0)
        {
            ValidationMessage += FString::Printf(TEXT("\n❌ %d cues with invalid class"), InvalidCuesCount);
        }

        ValidationStatusText->SetText(FText::FromString(ValidationMessage));
    }
}

auto UCkCueToolboxWidget::DoApplySearchFilter() -> void
{
    FilteredCueEntries.Empty();

    if (CurrentSearchText.IsEmpty())
    {
        FilteredCueEntries = AllCueEntries;
    }
    else
    {
        const auto SearchTextLower = CurrentSearchText.ToLower();

        FilteredCueEntries = ck::algo::Filter(AllCueEntries,
        [&](const TObjectPtr<UCk_CueToolbox_CueListEntry>& CueEntry) -> bool
        {
            if (ck::Is_NOT_Valid(CueEntry))
            { return false; }

            const auto& CueInfo = CueEntry->CueInfo;
            const auto CueNameString = CueInfo.CueName.ToString().ToLower();

            return CueNameString.Contains(SearchTextLower);
        });
    }

    if (ck::IsValid(CueListView))
    {
        CueListView->ClearListItems();

        for (const auto& CueEntry : FilteredCueEntries)
        {
            CueListView->AddItem(CueEntry);
        }

        CueListView->RegenerateAllEntries();
    }
}

auto UCkCueToolboxWidget::DoGetCurrentSubsystem() -> UCk_CueSubsystem_Base_UE*
{
    if (ck::Is_NOT_Valid(SubsystemComboBox))
    { return nullptr; }

    const auto SelectedOption = SubsystemComboBox->GetSelectedOption();

    if (SelectedOption.IsEmpty())
    { return nullptr; }

    const auto* FoundSubsystem = DiscoveredSubsystems.Find(SelectedOption);

    return FoundSubsystem && FoundSubsystem->IsValid() ? FoundSubsystem->Get() : nullptr;
}
