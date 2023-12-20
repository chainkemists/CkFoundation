#include "CkEditorStyle_ClassPicker.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "EditorStyleSet.h"
#include "SlateOptMacros.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Widgets/Layout/SUniformGridPanel.h"

#define LOCTEXT_NAMESPACE "SEditorStyle_AssetClassPicker"

// --------------------------------------------------------------------------------------------------------------------

namespace ck
{
    namespace filter
    {
        class FEditorStyle_AssetClassFilter : public IClassViewerFilter
        {
            CK_GENERATED_BODY(FEditorStyle_AssetClassFilter);

        public:
            FEditorStyle_AssetClassFilter()
                : _DisallowedClassFlags(CLASS_None), _DisallowBlueprintBase(false) {}

        public:
            auto
                IsClassAllowed(
                    const FClassViewerInitializationOptions& InInitOptions,
                    const UClass*                            InClass,
                    TSharedRef<FClassViewerFilterFuncs>      InFilterFuncs)
                -> bool override
            {
                const auto IsAllowed= NOT InClass->HasAnyClassFlags(_DisallowedClassFlags)
                    && InClass->CanCreateAssetOfClass()
                    && InFilterFuncs->IfInChildOfClassesSet(_AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;

                if (IsAllowed && _DisallowBlueprintBase && FKismetEditorUtilities::CanCreateBlueprintOfClass(InClass))
                { return false; }

                return IsAllowed;
            }

            auto
                IsUnloadedClassAllowed(
                    const FClassViewerInitializationOptions&       InInitOptions,
                    const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData,
                    TSharedRef<FClassViewerFilterFuncs>            InFilterFuncs)
                -> bool override
            {
                if (_DisallowBlueprintBase)
                { return false; }

                return NOT InUnloadedClassData->HasAnyClassFlags(_DisallowedClassFlags)
                    && InFilterFuncs->IfInChildOfClassesSet(_AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
            }

        private:
            TSet<const UClass*> _AllowedChildrenOfClasses;
            EClassFlags _DisallowedClassFlags;
            bool _DisallowBlueprintBase;

        public:
            CK_PROPERTY(_AllowedChildrenOfClasses);
            CK_PROPERTY(_DisallowedClassFlags);
            CK_PROPERTY(_DisallowBlueprintBase);
        };
    }

    // --------------------------------------------------------------------------------------------------------------------

    namespace widgets
    {
        auto
            SEditorStyle_AssetClassPicker::
            Construct(
                const FArguments& InArgs)
            -> void
        {}

        auto
            SEditorStyle_AssetClassPicker::
            ConfigureProperties()
            -> bool
        {
            _IsOkClicked = false;

            ChildSlot
            [
                SNew(SBorder)
                .Visibility(EVisibility::Visible)
                .BorderImage(DoGet_Brush("Menu.Background"))
                [
                    SNew(SBox)
                    .Visibility(EVisibility::Visible)
                    .WidthOverride(1500.0f)
                    [
                        SNew(SVerticalBox)
                        + SVerticalBox::Slot()
                        .FillHeight(1)
                        [
                            SNew(SBorder)
                            .BorderImage(DoGet_Brush("ToolPanel.GroupBorder"))
                            .Content()
                            [
                                SAssignNew(_ParentClassContainer, SVerticalBox)
                            ]
                        ]
                        // Ok/Cancel buttons
                        + SVerticalBox::Slot()
                          .AutoHeight()
                          .HAlign(HAlign_Center)
                          .VAlign(VAlign_Bottom)
                          .Padding(8)
                        [
                            SNew(SUniformGridPanel)
                                .SlotPadding(DoGet_Margin("StandardDialog.SlotPadding"))
                                .MinDesiredSlotWidth(DoGet_Float("StandardDialog.MinDesiredSlotWidth"))
                                .MinDesiredSlotHeight(DoGet_Float("StandardDialog.MinDesiredSlotHeight"))
                            + SUniformGridPanel::Slot(0, 0)
                            [
                                SNew(SButton)
                                .HAlign(HAlign_Center)
                                .ContentPadding(DoGet_Margin("StandardDialog.ContentPadding"))
                                .OnClicked(this, &SEditorStyle_AssetClassPicker::OnOkClicked)
                                .Text(LOCTEXT("EditorStyle_ClassPicker_Ok", "OK"))
                            ]
                            + SUniformGridPanel::Slot(1, 0)
                            [
                                SNew(SButton)
                                .HAlign(HAlign_Center)
                                .ContentPadding(DoGet_Margin("StandardDialog.ContentPadding"))
                                .OnClicked(this, &SEditorStyle_AssetClassPicker::OnCancelClicked)
                                .Text(LOCTEXT("EditorStyle_ClassPicker_Cancel", "Cancel"))
                            ]
                        ]
                    ]
                ]
            ];

            MakeParentClassPicker();

            const TSharedRef<SWindow> Window = SNew(SWindow)
            .Title(LOCTEXT("SelectAssetClassOptions", "Select Asset Class"))
            .ClientSize(FVector2D(400, 700))
            .SupportsMinimize(false).SupportsMaximize(false)
            [
                AsShared()
            ];

            _PickerWindow = Window;

            GEditor->EditorAddModalWindow(Window);
            return _IsOkClicked;
        }

        auto
            SEditorStyle_AssetClassPicker::
            OnOkClicked()
            -> FReply
        {
            CloseDialog(true);
            return FReply::Handled();
        }

        auto
            SEditorStyle_AssetClassPicker::
            CloseDialog(
                bool InWasPicked)
            -> void
        {
            _IsOkClicked = InWasPicked;

            if (NOT _IsOkClicked)
            {
                _ClassPicked.Reset();
            }

            if (ck::IsValid(_PickerWindow))
            {
                _PickerWindow.Pin()->RequestDestroyWindow();
            }
        }

        auto
            SEditorStyle_AssetClassPicker::
            OnCancelClicked()
            -> FReply
        {
            CloseDialog();
            return FReply::Handled();
        }

        auto
            SEditorStyle_AssetClassPicker::
            OnKeyDown(
                const FGeometry& MyGeometry,
                const FKeyEvent& InKeyEvent)
            -> FReply
        {
            if (InKeyEvent.GetKey() == EKeys::Escape)
            {
                CloseDialog();
                return FReply::Handled();
            }

            return SWidget::OnKeyDown(MyGeometry, InKeyEvent);
        }

        auto
            SEditorStyle_AssetClassPicker::
            DoGet_Brush(
                FName PropertyName) const
            -> const FSlateBrush*
        {
            return FAppStyle::GetBrush(PropertyName);
        }

        auto
            SEditorStyle_AssetClassPicker::
            DoGet_Margin(
                FName PropertyName) const
            -> const FMargin&
        {
            return FAppStyle::GetMargin(PropertyName);
        }

        auto
            SEditorStyle_AssetClassPicker::
            DoGet_Float(
                FName PropertyName) const
            -> float
        {
            return FAppStyle::GetFloat(PropertyName);
        }

        auto
            SEditorStyle_AssetClassPicker::
            MakeParentClassPicker()
            -> void
        {
            FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

            const TSharedPtr<filter::FEditorStyle_AssetClassFilter> Filter = MakeShareable(new filter::FEditorStyle_AssetClassFilter);
            Filter->Set_AllowedChildrenOfClasses({ UObject::StaticClass() });

            FClassViewerInitializationOptions Options;
            Options.Mode = EClassViewerMode::ClassPicker;
            Options.bIsBlueprintBaseOnly = true;
            Options.ClassFilters.Add(Filter.ToSharedRef());
            Options.NameTypeToDisplay = EClassViewerNameTypeToDisplay::DisplayName;

            _ParentClassContainer->ClearChildren();
            _ParentClassContainer->AddSlot()
            .AutoHeight()
            [
                SNew(STextBlock)
                .Text(LOCTEXT("ParentClass", "Search for Class:"))
                .ShadowOffset(FVector2D(1.0f, 1.0f))
            ];

            _ParentClassContainer->AddSlot()
            [
                ClassViewerModule.CreateClassViewer(Options, FOnClassPicked::CreateSP(this, &SEditorStyle_AssetClassPicker::OnClassPicked))
            ];
        }

        auto
            SEditorStyle_AssetClassPicker::
            OnClassPicked(
                UClass* ChosenClass)
            -> void
        {
            _ClassPicked = ChosenClass;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------

#undef LOCATEXT_NAMESPACE
