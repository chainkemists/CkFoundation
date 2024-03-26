#include "CkDataViewerAsset.h"

#include "CkCore/Object/CkObject_Utils.h"

#include "CkDataViewer/CkDataViewer_Log.h"

#include "CkLog/CkLog_Utils.h"

#include <EdGraphSchema_K2.h>
#include <Kismet2/BlueprintEditorUtils.h>
#include <Kismet2/KismetEditorUtilities.h>

#include <UObject/TextProperty.h>

namespace data_viewer_cpp
{
    constexpr auto VariablesPrefix = TEXT("DataViewerVar_");
}

auto
    UCk_DataViewerAsset_PDA::
    DoGet_ObjectsToView_Implementation() const
    -> TArray<FCk_DataViewer_ObjectsInfo>
{
    return {};
}

auto
    UCk_DataViewerAsset_PDA::
    DoGet_PropertiesToIgnore_Implementation() const
    -> TArray<FString>
{
    return {};
}

auto
    UCk_DataViewerAsset_PDA::
    PostLoad()
    -> void
{
    Super::PostLoad();

    if (IsTemplate())
    { return; }

    const auto& TimerManager = GEditor->GetTimerManager();
    TimerManager->SetTimerForNextTick([this]()
    {
        Reload();
    });
}

auto
    UCk_DataViewerAsset_PDA::
    PostEditChangeProperty(
        FPropertyChangedEvent& PropertyChangedEvent)
    -> void
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    if (IsTemplate())
    { return; }

    const auto& PropertyName = PropertyChangedEvent.Property->GetFName();

    const auto& PropertyNameString = PropertyName.ToString();

    if (PropertyNameString.Find(data_viewer_cpp::VariablesPrefix) == INDEX_NONE)
    { return; }

    int32 IndexOfGuidStart = INDEX_NONE;
    PropertyNameString.FindLastChar('_', IndexOfGuidStart);

    CK_LOG_ERROR_NOTIFY_IF_NOT(ck::data_viewer, IndexOfGuidStart != INDEX_NONE,
        TEXT("Property [{}] has the expected Prefix but could not extract the GUID from it.{}"),
        PropertyNameString, ck::Context(this))
    { return; }

    const auto& GuidString = PropertyNameString.RightChop(IndexOfGuidStart + 1);
    const auto& Guid = FGuid{GuidString};

    CK_LOG_ERROR_NOTIFY_IF_NOT(ck::data_viewer, IndexOfGuidStart != INDEX_NONE, TEXT("Could NOT get a valid BUID from Property [{}].{}"),
        PropertyNameString, ck::Context(this))
    { return; }

    const auto& ThisPropertyValue = DoGet_ValueFromProperty(PropertyChangedEvent.Property);
    auto NameOnly = PropertyNameString;
    NameOnly.RemoveFromStart(FString{data_viewer_cpp::VariablesPrefix});
    NameOnly.RemoveFromEnd(FString{TEXT("_")} + GuidString);
    DoUpdatePropertyOnBlueprint(Guid, FName{NameOnly}, ThisPropertyValue);

    const auto Package = GetOutermost();
    Package->SetDirtyFlag(false);
}

auto
    UCk_DataViewerAsset_PDA::
    DoUpdatePropertyOnBlueprint(
        const FGuid& InGuid,
        FName InPropertyName,
        const FString& InValue)
    -> void
{
    auto Blueprint = DoGet_Blueprint(InGuid);

    CK_LOG_ERROR_NOTIFY_IF_NOT(ck::data_viewer, ck::IsValid(Blueprint, ck::IsValid_Policy_NullptrOnly{}),
        TEXT("Could not find a Blueprint with Guid [{}]."), InGuid, ck::Context(this))
    { return; }

    const auto  BlueprintGenClass = Blueprint->GeneratedClass;

    auto Property = BlueprintGenClass->FindPropertyByName(InPropertyName);
    if (ck::IsValid(Property, ck::IsValid_Policy_NullptrOnly{}))
    {
        FBlueprintEditorUtils::PropertyValueFromString(Property, InValue, reinterpret_cast<uint8*>(BlueprintGenClass->ClassDefaultObject));
    }

    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    std::ignore = Blueprint->MarkPackageDirty();
}

auto
    UCk_DataViewerAsset_PDA::
    DoGet_ValueFromProperty(
        FProperty* InProperty) const
        -> FString
{
    const uint8* ThisContainer = reinterpret_cast<const uint8*>(this);
    return DoGet_ValueFromProperty(InProperty, ThisContainer);
}

auto
    UCk_DataViewerAsset_PDA::
    DoGet_ValueFromProperty(
        FProperty* InProperty,
        const uint8* InContainer)
    -> FString
{
    FString Value;
    FBlueprintEditorUtils::PropertyValueToString(InProperty, InContainer, Value);
    return Value;
}

auto
    UCk_DataViewerAsset_PDA::
    DoResetView(
        ECompileOrNot InCompileOrNot)
    -> void
{
    auto Blueprint = Cast<UBlueprint>(UCk_Utils_Object_UE::Get_ClassGeneratedByBlueprint(GetClass()));

    if (ck::Is_NOT_Valid(Blueprint))
    { return; }

    for (auto Itr = TFieldIterator<FProperty>{GetClass()}; Itr; ++Itr)
    {
        const auto Field = *Itr;
        const auto Property = static_cast<FStructProperty*>(Field);

        if (NOT (Property->PropertyFlags & CPF_BlueprintVisible))
        { continue; }

        FBlueprintEditorUtils::RemoveMemberVariable(Blueprint, *Itr->GetNameCPP());
    }

    if (InCompileOrNot == ECompileOrNot::Compile)
    { FKismetEditorUtilities::CompileBlueprint(Blueprint); }

    DoForcePackageMarkNotDirty(Blueprint);
}

auto
    UCk_DataViewerAsset_PDA::
    DoForcePackageMarkNotDirty(
        UBlueprint* InBlueprint)
    -> void
{
    if (IsTemplate())
    { return; }

    UPackage* Package = InBlueprint->GetOutermost();

    if (ck::Is_NOT_Valid(Package, ck::IsValid_Policy_NullptrOnly{}))
    { return; }

    Package->SetDirtyFlag(false);
}

auto
    UCk_DataViewerAsset_PDA::
    TryGet_PinType(
        const FStructProperty* InProperty)
    -> TOptional<FEdGraphPinType>
{
    auto PinType = FEdGraphPinType{};

    if (InProperty-> IsA(FBoolProperty::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Boolean;
    }
    else if (InProperty->IsA(FInt8Property::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Byte;
    }
    else if (InProperty->IsA(FInt16Property::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
    }
    else if (InProperty->IsA(FIntProperty::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Int;
    }
    else if (InProperty->IsA(FInt64Property::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Int64;
    }
    else if (InProperty->IsA(FFloatProperty::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Float;
    }
    else if (InProperty->IsA(FDoubleProperty::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Double;
    }
    else if (InProperty->IsA(FStrProperty::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_String;
    }
    else if (InProperty->IsA(FNameProperty::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Name;
    }
    else if (InProperty->IsA(FTextProperty::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Text;
    }
    else if (InProperty->IsA(FByteProperty::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Enum;
    }
    else if (InProperty->IsA(FObjectPropertyBase::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Object;
        PinType.PinSubCategoryObject = InProperty->Struct;
    }
    else if (InProperty->IsA(FArrayProperty::StaticClass()))
    {
        return {};
    }
    else if (InProperty->IsA(FMapProperty::StaticClass()))
    {
        return {};
    }
    else if (InProperty->IsA(FSetProperty::StaticClass()))
    {
        return {};
    }
    else if (InProperty->IsA(FStructProperty::StaticClass()))
    {
        PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
        PinType.PinSubCategoryObject = InProperty->Struct;
    }
    // Add more checks for other property types as needed...
    else
    {
        return {};
    }

    return PinType;
}

auto
    UCk_DataViewerAsset_PDA::
    ResetView()
    -> void
{
    DoResetView(ECompileOrNot::Compile);
}

auto
    UCk_DataViewerAsset_PDA::
    Reload()
    -> void
{
    if (IsTemplate())
    { return; }

    DoResetView(ECompileOrNot::DoNotCompile);

    auto Blueprint = Cast<UBlueprint>(UCk_Utils_Object_UE::Get_ClassGeneratedByBlueprint(GetClass()));

    if (ck::Is_NOT_Valid(Blueprint))
    { return; }

    const auto& ClassesToView = DoGet_ObjectsToView();
    const auto& PropertiesToIgnore = DoGet_PropertiesToIgnore();

    for (auto Info : ClassesToView)
    {
        auto Class = Info.Get_Class();
        auto PropertiesToIgnoreCombined = Info.Get_PropertiesToIgnore();
        PropertiesToIgnoreCombined.Append(PropertiesToIgnore);

        if (ck::Is_NOT_Valid(Class))
        { continue; }

        const auto GenClassObject = UCk_Utils_Object_UE::Get_ClassGeneratedByBlueprint(Class);
        auto ClassBlueprint = Cast<UBlueprint>(GenClassObject);

        for (auto Itr = TFieldIterator<FProperty>{Class}; Itr; ++Itr)
        {
            const auto Field = *Itr;
            const auto Property = static_cast<FStructProperty*>(Field);

            if (PropertiesToIgnoreCombined.Find(Itr->NamePrivate.ToString().TrimStartAndEnd()) != INDEX_NONE ||
                PropertiesToIgnoreCombined.Find(Itr->GetDisplayNameText().ToString().TrimStartAndEnd()) != INDEX_NONE)
            { continue; }

            if (NOT (Property->PropertyFlags & CPF_ExposeOnSpawn))
            { continue; }

            auto PinType = TryGet_PinType(Property);
            if (ck::Is_NOT_Valid(PinType))
            { continue; }

            if (ck::Is_NOT_Valid(ClassBlueprint, ck::IsValid_Policy_NullptrOnly{}))
            { continue; }

            const auto ClassBlueprintGenClass = ClassBlueprint->GeneratedClass;
            const auto& Value = DoGet_ValueFromProperty(Property, reinterpret_cast<uint8*>(ClassBlueprintGenClass->ClassDefaultObject));

            const auto Name = [&]()
            {
                auto NameStr = FString{data_viewer_cpp::VariablesPrefix};
                NameStr += Itr->NamePrivate.ToString();
                NameStr += TEXT("_");
                NameStr += ClassBlueprint->GetBlueprintGuid().ToString();

                return FName{NameStr};
            }();

            FBlueprintEditorUtils::AddMemberVariable(Blueprint, Name, *PinType, Value);

            if (const auto Found = Blueprint->NewVariables.FindByPredicate([&](const FBPVariableDescription& InVarDesc)
            {
                if (InVarDesc.VarName == Name)
                { return true; }

                return false;
            }))
            {
                Found->RemoveMetaData("BlueprintPrivate");
                Found->Category = FText::FromString(ck::Format_UE(TEXT("{} ({})"), Info.Get_OptionalFriendlyName(), ClassBlueprint->GetFName()));
                Found->SetMetaData("DisplayName", Property->GetDisplayNameText().ToString());
            }
        }
    }

    FKismetEditorUtilities::CompileBlueprint(Blueprint);
    DoForcePackageMarkNotDirty(Blueprint);
}

auto
    UCk_DataViewerAsset_PDA::
    DoGet_Blueprint(
        const FGuid& InGuid) const
    -> UBlueprint*
{
    const auto& ClassesToView = DoGet_ObjectsToView();

    for (auto ObjectInfo : ClassesToView)
    {
        auto Class = ObjectInfo.Get_Class();

        const auto GenClassObject = UCk_Utils_Object_UE::Get_ClassGeneratedByBlueprint(Class);
        const auto ClassBlueprint = Cast<UBlueprint>(GenClassObject);

        if (ClassBlueprint->GetBlueprintGuid() == InGuid)
        {
            return ClassBlueprint;
        }
    }

    return {};
}
