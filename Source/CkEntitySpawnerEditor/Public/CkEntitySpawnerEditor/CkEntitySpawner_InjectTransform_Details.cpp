#include "CkEntitySpawner_InjectTransform_Details.h"

#include "CkCore/Validation/CkIsValid.h"

#include "CkEntitySpawner/CkEntitySpawner_Actor.h"

#include <DetailWidgetRow.h>
#include <Framework/MultiBox/MultiBoxBuilder.h>
#include <IDetailChildrenBuilder.h>
#include <PropertyHandle.h>
#include <Widgets/Input/SComboButton.h>
#include <Widgets/Layout/SBox.h>
#include <Widgets/SBoxPanel.h>
#include <Widgets/Text/STextBlock.h>

// --------------------------------------------------------------------------------------------------------------------

#define LOCTEXT_NAMESPACE "CkEntitySpawnerInjectTransformCustomization"

namespace ck::layout
{
    auto
        FCk_EntitySpawner_InjectTransform_Details::
        MakeInstance()
        -> TSharedRef<IPropertyTypeCustomization>
    {
        return MakeShareable(new FCk_EntitySpawner_InjectTransform_Details);
    }

    auto
        FCk_EntitySpawner_InjectTransform_Details::
        CustomizeHeader(
            TSharedRef<IPropertyHandle> InStructPropertyHandle,
            FDetailWidgetRow& InHeaderRow,
            IPropertyTypeCustomizationUtils& InStructCustomizationUtils)
        -> void
    {
        _OuterBindingHandle = InStructPropertyHandle;
        // Literal FName: _PropertyName is private, GET_MEMBER_NAME_CHECKED can't see it from here.
        _PropertyNameHandle = InStructPropertyHandle->GetChildHandle(FName{TEXT("_PropertyName")});

        InHeaderRow.
            NameContent()
            [
                InStructPropertyHandle->CreatePropertyNameWidget()
            ]
            .ValueContent()
            .MinDesiredWidth(250.0f)
            .MaxDesiredWidth(4096.0f)
            [
                SNew(SComboButton)
                .ContentPadding(FMargin(2.0f, 2.0f))
                .OnGetMenuContent(this, &FCk_EntitySpawner_InjectTransform_Details::MakeMenu)
                .ButtonContent()
                [
                    SNew(STextBlock)
                    .Text(this, &FCk_EntitySpawner_InjectTransform_Details::GetButtonLabel)
                ]
            ];
    }

    auto
        FCk_EntitySpawner_InjectTransform_Details::
        CustomizeChildren(
            TSharedRef<IPropertyHandle> InStructPropertyHandle,
            IDetailChildrenBuilder& InStructBuilder,
            IPropertyTypeCustomizationUtils& InStructCustomizationUtils)
        -> void
    {
    }

    auto
        FCk_EntitySpawner_InjectTransform_Details::
        GetButtonLabel() const
        -> FText
    {
        auto CurrentName = FName{};
        if (_PropertyNameHandle.IsValid())
        {
            _PropertyNameHandle->GetValue(CurrentName);
        }

        if (CurrentName.IsNone())
        {
            if (auto* Default = ck::entityspawner::TryResolveDefaultTransformProperty(ResolveOwningEntityScriptClass());
                ck::IsValid(Default))
            {
                return FText::Format(LOCTEXT("DefaultBinding", "{0} (default)"),
                    FText::FromName(Default->GetFName()));
            }

            return LOCTEXT("NoBinding", "(none — use authored value)");
        }

        return FText::FromName(CurrentName);
    }

    auto
        FCk_EntitySpawner_InjectTransform_Details::
        CollectCandidateProperties() const
        -> TArray<TTuple<FName, FText>>
    {
        auto Candidates = TArray<TTuple<FName, FText>>{};

        const auto* EntityScriptClass = ResolveOwningEntityScriptClass();
        if (ck::Is_NOT_Valid(EntityScriptClass))
        { return Candidates; }

        for (auto PropIt = TFieldIterator<FProperty>(EntityScriptClass); PropIt; ++PropIt)
        {
            const auto* Property = *PropIt;
            if (ck::Is_NOT_Valid(Property))
            { continue; }

            if (NOT Property->HasAnyPropertyFlags(CPF_Edit))
            { continue; }

            const auto* StructProp = CastField<FStructProperty>(Property);
            if (ck::Is_NOT_Valid(StructProp))
            { continue; }

            if (StructProp->Struct == TBaseStructure<FTransform>::Get())
            {
                Candidates.Emplace(Property->GetFName(),
                    FText::Format(LOCTEXT("TransformEntry", "{0}  (Transform)"),
                        Property->GetDisplayNameText()));
            }
        }

        return Candidates;
    }

    auto
        FCk_EntitySpawner_InjectTransform_Details::
        ResolveOwningEntityScriptClass() const
        -> const UClass*
    {
        if (NOT _OuterBindingHandle.IsValid())
        { return nullptr; }

        auto OuterObjects = TArray<UObject*>{};
        _OuterBindingHandle->GetOuterObjects(OuterObjects);

        for (const auto& Outer : OuterObjects)
        {
            auto* Spawner = Cast<ACk_EntitySpawner_UE>(Outer);
            if (ck::Is_NOT_Valid(Spawner))
            { continue; }

            const auto& ScriptPtr = Spawner->Get_EntityScript();
            if (ck::IsValid(ScriptPtr))
            {
                return ScriptPtr->GetClass();
            }
        }

        return nullptr;
    }

    auto
        FCk_EntitySpawner_InjectTransform_Details::
        MakeMenu() const
        -> TSharedRef<SWidget>
    {
        auto MenuBuilder = FMenuBuilder{true, nullptr};

        auto NameHandle = _PropertyNameHandle;
        const auto SetValue = [NameHandle](FName InName)
        {
            if (NameHandle.IsValid())
            { NameHandle->SetValue(InName); }
        };

        MenuBuilder.AddMenuEntry(
            LOCTEXT("ClearBinding", "(none — use authored value)"),
            LOCTEXT("ClearBindingTooltip",
                "Do not inject the spawner's actor transform. The script property uses its authored default."),
            FSlateIcon{},
            FUIAction{FExecuteAction::CreateLambda([SetValue]() { SetValue(NAME_None); })});

        const auto Candidates = CollectCandidateProperties();

        if (Candidates.IsEmpty())
        {
            MenuBuilder.AddWidget(
                SNew(SBox)
                .Padding(8.0f)
                [
                    SNew(STextBlock)
                    .Text(LOCTEXT("NoCandidates",
                        "No FTransform UPROPERTY found on the assigned EntityScript."))
                ],
                FText::GetEmpty());

            return MenuBuilder.MakeWidget();
        }

        MenuBuilder.BeginSection(NAME_None, LOCTEXT("BindToHeader", "Bind to Script Property"));
        for (const auto& Entry : Candidates)
        {
            const auto PropertyName = Entry.Get<0>();
            const auto Label = Entry.Get<1>();

            MenuBuilder.AddMenuEntry(
                Label,
                FText::FromName(PropertyName),
                FSlateIcon{},
                FUIAction{FExecuteAction::CreateLambda(
                    [SetValue, PropertyName]() { SetValue(PropertyName); })});
        }
        MenuBuilder.EndSection();

        return MenuBuilder.MakeWidget();
    }
}

#undef LOCTEXT_NAMESPACE

// --------------------------------------------------------------------------------------------------------------------
