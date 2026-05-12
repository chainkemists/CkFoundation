#include "CkDataAssetExporter.h"

#include "CkAssetExporter_Log.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Format/CkFormat.h"
#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Engine/DataAsset.h>
#include <Dom/JsonObject.h>
#include <Dom/JsonValue.h>
#include <Serialization/JsonSerializer.h>
#include <Serialization/JsonWriter.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/DateTime.h>
#include <Misc/ScopeExit.h>
#include <UObject/UnrealType.h>
#include <UObject/TextProperty.h>
#include <UObject/EnumProperty.h>
#include <UObject/Class.h>
#include <GameplayTagContainer.h>
#include <StructUtils/InstancedStruct.h>

// --------------------------------------------------------------------------------------------------------------------
// Public API
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DataAssetExporter::
    ExportDataAsset(
        UDataAsset* InDataAsset)
    -> FCk_DataAssetExportResult
{
    auto Result = FCk_DataAssetExportResult{};

    if (ck::Is_NOT_Valid(InDataAsset))
    {
        Result.ErrorMessage = TEXT("Invalid DataAsset");
        return Result;
    }

    Result.AssetName = InDataAsset->GetName();

    // Serialize to JSON
    const auto JsonObject = DoSerializeToJson(InDataAsset);
    if (NOT JsonObject.IsValid())
    {
        Result.ErrorMessage = TEXT("Failed to serialize DataAsset to JSON");
        return Result;
    }

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    // Serialize to plain text
    const auto TextString = DoSerializeToText(InDataAsset);

    // Resolve output paths
    const auto JsonPath = DoResolveOutputPath(InDataAsset, TEXT(".json"));
    const auto TextPath = DoResolveOutputPath(InDataAsset, TEXT(".txt"));

    if (JsonPath.IsEmpty() || TextPath.IsEmpty())
    {
        Result.ErrorMessage = TEXT("Failed to resolve output file paths");
        return Result;
    }

    // Write files
    const auto JsonWritten = FFileHelper::SaveStringToFile(
        JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const auto TextWritten = FFileHelper::SaveStringToFile(
        TextString, *TextPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    if (NOT JsonWritten || NOT TextWritten)
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write files (JSON: {}, Text: {})"),
            JsonWritten ? TEXT("OK") : TEXT("FAILED"),
            TextWritten ? TEXT("OK") : TEXT("FAILED"));
        return Result;
    }

    Result.bSuccess = true;
    Result.JsonFilePath = JsonPath;
    Result.TextFilePath = TextPath;
    return Result;
}

auto
    FCk_DataAssetExporter::
    ExportDataAssets(
        const TArray<UDataAsset*>& InDataAssets)
    -> TArray<FCk_DataAssetExportResult>
{
    auto Results = TArray<FCk_DataAssetExportResult>{};
    Results.Reserve(InDataAssets.Num());

    ck::algo::ForEach(InDataAssets, [&](UDataAsset* DA)
    {
        Results.Add(ExportDataAsset(DA));
    });

    return Results;
}

// --------------------------------------------------------------------------------------------------------------------
// JSON Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DataAssetExporter::
    DoSerializeToJson(
        const UDataAsset* InDataAsset)
    -> TSharedPtr<FJsonObject>
{
    auto RootObject = MakeShared<FJsonObject>();

    RootObject->SetStringField(TEXT("assetName"), InDataAsset->GetName());
    RootObject->SetStringField(TEXT("assetPath"), InDataAsset->GetPathName());
    RootObject->SetStringField(TEXT("assetClass"), InDataAsset->GetClass()->GetName());
    RootObject->SetStringField(TEXT("exportTimestamp"), FDateTime::UtcNow().ToIso8601());

    // Parent class chain
    auto ParentChain = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto* Class = InDataAsset->GetClass()->GetSuperClass();
         Class != nullptr && Class != UObject::StaticClass();
         Class = Class->GetSuperClass())
    {
        ParentChain.Add(MakeShared<FJsonValueString>(Class->GetName()));
    }
    RootObject->SetArrayField(TEXT("parentClasses"), ParentChain);

    // Properties — stop at UDataAsset base (don't serialize UObject internals)
    RootObject->SetArrayField(TEXT("properties"),
        DoSerializeProperties_Json(InDataAsset, UDataAsset::StaticClass()));

    return RootObject;
}

auto
    FCk_DataAssetExporter::
    DoSerializeProperties_Json(
        const UObject* InObject,
        const UClass* InStopAtClass)
    -> TArray<TSharedPtr<FJsonValue>>
{
    auto Properties = TArray<TSharedPtr<FJsonValue>>{};

    if (InObject == nullptr)
    { return Properties; }

    for (TFieldIterator<FProperty> It(InObject->GetClass()); It; ++It)
    {
        const auto* Property = *It;

        if (NOT DoShouldIncludeProperty(Property))
        { continue; }

        // Skip properties owned by the stop class itself (but keep ones owned by
        // child classes of the stop class — that's how we exclude UObject/UDataAsset
        // internals while including PDA-hierarchy declared members).
        if (Property->GetOwnerClass() == InStopAtClass)
        { continue; }

        auto PropObject = MakeShared<FJsonObject>();
        PropObject->SetStringField(TEXT("name"), Property->GetName());
        PropObject->SetStringField(TEXT("type"), Property->GetCPPType());

        // Category
        const auto Category = Property->GetMetaData(TEXT("Category"));
        if (NOT Category.IsEmpty())
        {
            PropObject->SetStringField(TEXT("category"), Category);
        }

        // Value — recursive typed JSON value
        const auto* ValuePtr = Property->ContainerPtrToValuePtr<void>(InObject);
        const auto JsonValue = DoSerializePropertyValue_Json(Property, ValuePtr);
        if (JsonValue.IsValid())
        {
            PropObject->SetField(TEXT("value"), JsonValue);
        }
        else
        {
            PropObject->SetField(TEXT("value"), MakeShared<FJsonValueNull>());
        }

        // Tooltip/description
        const auto Tooltip = Property->GetMetaData(TEXT("ToolTip"));
        if (NOT Tooltip.IsEmpty())
        {
            PropObject->SetStringField(TEXT("tooltip"), Tooltip);
        }

        Properties.Add(MakeShared<FJsonValueObject>(PropObject));
    }

    return Properties;
}

auto
    FCk_DataAssetExporter::
    DoSerializePropertyValue_Json(
        const FProperty* InProperty,
        const void* InValuePtr)
    -> TSharedPtr<FJsonValue>
{
    if (InProperty == nullptr || InValuePtr == nullptr)
    { return MakeShared<FJsonValueNull>(); }

    // Bool
    if (const auto* BoolProp = CastField<FBoolProperty>(InProperty))
    {
        return MakeShared<FJsonValueBoolean>(BoolProp->GetPropertyValue(InValuePtr));
    }

    // Enum (FEnumProperty — strongly-typed enum class)
    if (const auto* EnumProp = CastField<FEnumProperty>(InProperty))
    {
        const auto* UnderlyingProp = EnumProp->GetUnderlyingProperty();
        const auto Value = UnderlyingProp->GetSignedIntPropertyValue(InValuePtr);
        if (const auto* Enum = EnumProp->GetEnum())
        {
            return MakeShared<FJsonValueString>(Enum->GetNameStringByValue(Value));
        }
        return MakeShared<FJsonValueNumber>(static_cast<double>(Value));
    }

    // Byte / TEnumAsByte
    if (const auto* ByteProp = CastField<FByteProperty>(InProperty))
    {
        const auto Value = ByteProp->GetSignedIntPropertyValue(InValuePtr);
        if (ByteProp->Enum != nullptr)
        {
            return MakeShared<FJsonValueString>(ByteProp->Enum->GetNameStringByValue(Value));
        }
        return MakeShared<FJsonValueNumber>(static_cast<double>(Value));
    }

    // Numeric (int/float)
    if (const auto* NumericProp = CastField<FNumericProperty>(InProperty))
    {
        if (NumericProp->IsFloatingPoint())
        {
            return MakeShared<FJsonValueNumber>(NumericProp->GetFloatingPointPropertyValue(InValuePtr));
        }
        return MakeShared<FJsonValueNumber>(static_cast<double>(NumericProp->GetSignedIntPropertyValue(InValuePtr)));
    }

    // String / Name / Text
    if (const auto* StrProp = CastField<FStrProperty>(InProperty))
    {
        return MakeShared<FJsonValueString>(StrProp->GetPropertyValue(InValuePtr));
    }
    if (const auto* NameProp = CastField<FNameProperty>(InProperty))
    {
        return MakeShared<FJsonValueString>(NameProp->GetPropertyValue(InValuePtr).ToString());
    }
    if (const auto* TextProp = CastField<FTextProperty>(InProperty))
    {
        return MakeShared<FJsonValueString>(TextProp->GetPropertyValue(InValuePtr).ToString());
    }

    // Struct — recurse into inner properties (special-case FGameplayTag for readability)
    if (const auto* StructProp = CastField<FStructProperty>(InProperty))
    {
        if (StructProp->Struct == TBaseStructure<FGameplayTag>::Get())
        {
            const auto& Tag = *static_cast<const FGameplayTag*>(InValuePtr);
            return MakeShared<FJsonValueString>(Tag.ToString());
        }

        if (StructProp->Struct == TBaseStructure<FGameplayTagContainer>::Get())
        {
            const auto& Container = *static_cast<const FGameplayTagContainer*>(InValuePtr);
            return MakeShared<FJsonValueString>(Container.ToString());
        }

        // FInstancedStruct wraps a separately-allocated USTRUCT payload. The
        // outer FInstancedStruct has no reflected fields of its own — iterating
        // TFieldIterator over its struct yields nothing — so without this branch
        // every entry of a TArray<FInstancedStruct> exports as an empty {}.
        if (StructProp->Struct == FInstancedStruct::StaticStruct())
        {
            const auto& Instanced = *static_cast<const FInstancedStruct*>(InValuePtr);
            const auto* InnerStruct = Instanced.GetScriptStruct();
            const auto* InnerMemory = Instanced.GetMemory();

            auto WrapperObject = MakeShared<FJsonObject>();
            if (InnerStruct == nullptr || InnerMemory == nullptr)
            {
                WrapperObject->SetField(TEXT("structType"), MakeShared<FJsonValueNull>());
                return MakeShared<FJsonValueObject>(WrapperObject);
            }

            WrapperObject->SetStringField(TEXT("structType"), InnerStruct->GetName());
            WrapperObject->SetStringField(TEXT("structPath"), InnerStruct->GetPathName());

            auto InnerObject = MakeShared<FJsonObject>();
            for (TFieldIterator<FProperty> InnerIt(InnerStruct); InnerIt; ++InnerIt)
            {
                const auto* InnerProp = *InnerIt;
                if (InnerProp == nullptr)
                { continue; }

                if (InnerProp->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_DuplicateTransient))
                { continue; }

                const auto* InnerValuePtr = InnerProp->ContainerPtrToValuePtr<void>(InnerMemory);
                if (auto InnerVal = DoSerializePropertyValue_Json(InnerProp, InnerValuePtr); InnerVal.IsValid())
                {
                    InnerObject->SetField(InnerProp->GetName(), InnerVal);
                }
            }
            WrapperObject->SetField(TEXT("properties"), MakeShared<FJsonValueObject>(InnerObject));
            return MakeShared<FJsonValueObject>(WrapperObject);
        }

        auto StructObject = MakeShared<FJsonObject>();
        for (TFieldIterator<FProperty> It(StructProp->Struct); It; ++It)
        {
            const auto* InnerProp = *It;
            if (InnerProp == nullptr)
            { continue; }

            // Struct members are owned by a UScriptStruct, not a UClass, so
            // DoShouldIncludeProperty's UClass-based filter rejects them all.
            // Apply a lighter filter appropriate for struct fields.
            if (InnerProp->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_DuplicateTransient))
            { continue; }

            const auto* InnerValuePtr = InnerProp->ContainerPtrToValuePtr<void>(InValuePtr);
            if (auto InnerVal = DoSerializePropertyValue_Json(InnerProp, InnerValuePtr); InnerVal.IsValid())
            {
                StructObject->SetField(InnerProp->GetName(), InnerVal);
            }
        }
        return MakeShared<FJsonValueObject>(StructObject);
    }

    // Array
    if (const auto* ArrayProp = CastField<FArrayProperty>(InProperty))
    {
        FScriptArrayHelper Helper(ArrayProp, InValuePtr);
        auto Elements = TArray<TSharedPtr<FJsonValue>>{};
        Elements.Reserve(Helper.Num());

        for (auto i = int32{0}; i < Helper.Num(); ++i)
        {
            const auto* ElemPtr = Helper.GetRawPtr(i);
            const auto ElemVal = DoSerializePropertyValue_Json(ArrayProp->Inner, ElemPtr);
            Elements.Add(ElemVal.IsValid() ? ElemVal : MakeShared<FJsonValueNull>());
        }
        return MakeShared<FJsonValueArray>(Elements);
    }

    // Set
    if (const auto* SetProp = CastField<FSetProperty>(InProperty))
    {
        FScriptSetHelper Helper(SetProp, InValuePtr);
        auto Elements = TArray<TSharedPtr<FJsonValue>>{};

        for (auto i = int32{0}; i < Helper.GetMaxIndex(); ++i)
        {
            if (!Helper.IsValidIndex(i))
            { continue; }
            const auto* ElemPtr = Helper.GetElementPtr(i);
            const auto ElemVal = DoSerializePropertyValue_Json(SetProp->ElementProp, ElemPtr);
            Elements.Add(ElemVal.IsValid() ? ElemVal : MakeShared<FJsonValueNull>());
        }
        return MakeShared<FJsonValueArray>(Elements);
    }

    // Map
    if (const auto* MapProp = CastField<FMapProperty>(InProperty))
    {
        FScriptMapHelper Helper(MapProp, InValuePtr);
        auto Entries = TArray<TSharedPtr<FJsonValue>>{};

        for (auto i = int32{0}; i < Helper.GetMaxIndex(); ++i)
        {
            if (!Helper.IsValidIndex(i))
            { continue; }

            auto EntryObj = MakeShared<FJsonObject>();
            const auto KeyVal = DoSerializePropertyValue_Json(MapProp->KeyProp, Helper.GetKeyPtr(i));
            const auto ValueVal = DoSerializePropertyValue_Json(MapProp->ValueProp, Helper.GetValuePtr(i));
            EntryObj->SetField(TEXT("key"), KeyVal.IsValid() ? KeyVal : MakeShared<FJsonValueNull>());
            EntryObj->SetField(TEXT("value"), ValueVal.IsValid() ? ValueVal : MakeShared<FJsonValueNull>());
            Entries.Add(MakeShared<FJsonValueObject>(EntryObj));
        }
        return MakeShared<FJsonValueArray>(Entries);
    }

    // Class reference (UClass*)
    if (const auto* ClassProp = CastField<FClassProperty>(InProperty))
    {
        const auto* ClassPtr = ClassProp->GetPropertyValue(InValuePtr).Get();
        return MakeShared<FJsonValueString>(ClassPtr != nullptr ? ClassPtr->GetPathName() : TEXT("None"));
    }

    // Object reference — recurse for instanced, otherwise emit path
    if (const auto* ObjectProp = CastField<FObjectProperty>(InProperty))
    {
        auto* Obj = ObjectProp->GetObjectPropertyValue(InValuePtr);

        const auto IsInstanced =
            ObjectProp->HasAnyPropertyFlags(CPF_InstancedReference | CPF_PersistentInstance) ||
            ObjectProp->HasMetaData(TEXT("EditInline")) ||
            (ObjectProp->PropertyClass != nullptr && ObjectProp->PropertyClass->HasAnyClassFlags(CLASS_EditInlineNew));

        if (IsInstanced && ck::IsValid(Obj))
        {
            return MakeShared<FJsonValueObject>(
                DoSerializeObjectProperties_Json(Obj, UObject::StaticClass()));
        }

        return MakeShared<FJsonValueString>(Obj != nullptr ? Obj->GetPathName() : TEXT("None"));
    }

    // Soft references / interface — path string
    if (const auto* SoftObjectProp = CastField<FSoftObjectProperty>(InProperty))
    {
        const auto& SoftObj = SoftObjectProp->GetPropertyValue(InValuePtr);
        return MakeShared<FJsonValueString>(SoftObj.ToString());
    }
    if (const auto* SoftClassProp = CastField<FSoftClassProperty>(InProperty))
    {
        const auto& SoftClass = SoftClassProp->GetPropertyValue(InValuePtr);
        return MakeShared<FJsonValueString>(SoftClass.ToString());
    }

    // Fallback — ExportTextItem_Direct
    auto FallbackStr = FString{};
    InProperty->ExportTextItem_Direct(FallbackStr, InValuePtr, nullptr, nullptr, PPF_None);
    return MakeShared<FJsonValueString>(FallbackStr);
}

namespace ck_data_asset_exporter_internal
{
    // Cap recursion depth and detect cycles to keep cyclic instanced-object graphs
    // (e.g. component subobjects that reference back to their owner) from blowing
    // the stack. Thread-local so it's safe under any caller threading model.
    static constexpr int32 GMaxObjectRecursionDepth = 8;
    thread_local int32 GObjectRecursionDepth = 0;
    thread_local TSet<const UObject*> GObjectsInProgress;
}

auto
    FCk_DataAssetExporter::
    DoSerializeObjectProperties_Json(
        const UObject* InObject,
        const UClass* InStopAtClass)
    -> TSharedPtr<FJsonObject>
{
    using namespace ck_data_asset_exporter_internal;

    auto Result = MakeShared<FJsonObject>();

    if (InObject == nullptr)
    {
        Result->SetField(TEXT("objectClass"), MakeShared<FJsonValueNull>());
        return Result;
    }

    Result->SetStringField(TEXT("objectClass"), InObject->GetClass()->GetName());
    Result->SetStringField(TEXT("objectClassPath"), InObject->GetClass()->GetPathName());
    Result->SetStringField(TEXT("objectName"), InObject->GetName());

    if (GObjectRecursionDepth >= GMaxObjectRecursionDepth || GObjectsInProgress.Contains(InObject))
    {
        Result->SetStringField(TEXT("objectPath"), InObject->GetPathName());
        Result->SetBoolField(TEXT("truncated"), true);
        return Result;
    }

    ++GObjectRecursionDepth;
    GObjectsInProgress.Add(InObject);
    ON_SCOPE_EXIT
    {
        GObjectsInProgress.Remove(InObject);
        --GObjectRecursionDepth;
    };

    Result->SetArrayField(TEXT("properties"),
        DoSerializeProperties_Json(InObject, InStopAtClass));

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
// Plain-Text Serialization
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DataAssetExporter::
    DoSerializeToText(
        const UDataAsset* InDataAsset)
    -> FString
{
    auto Text = FString{};

    Text += ck::Format_UE(TEXT("=== DataAsset: {} ===\n"), InDataAsset->GetName());
    Text += ck::Format_UE(TEXT("Path: {}\n"), InDataAsset->GetPathName());
    Text += ck::Format_UE(TEXT("Class: {}\n"), InDataAsset->GetClass()->GetName());

    // Parent class chain
    auto ParentNames = TArray<FString>{};
    for (const auto* Class = InDataAsset->GetClass()->GetSuperClass();
         Class != nullptr && Class != UObject::StaticClass();
         Class = Class->GetSuperClass())
    {
        ParentNames.Add(Class->GetName());
    }
    Text += ck::Format_UE(TEXT("Parent Classes: {}\n"), FString::Join(ParentNames, TEXT(" -> ")));

    Text += ck::Format_UE(TEXT("Exported: {}\n\n"), FDateTime::UtcNow().ToString());

    // Properties
    DoSerializeProperties_Text(InDataAsset, UDataAsset::StaticClass(), Text, 0);

    return Text;
}

auto
    FCk_DataAssetExporter::
    DoSerializeProperties_Text(
        const UObject* InObject,
        const UClass* InStopAtClass,
        FString& OutText,
        int32 InDepth)
    -> void
{
    if (InObject == nullptr)
    { return; }

    const auto Indent = DoGetIndent(InDepth);

    // Group properties by category
    auto CategoryProperties = TMap<FString, TArray<const FProperty*>>{};
    auto PropertyOrder = TArray<const FProperty*>{};

    for (TFieldIterator<FProperty> It(InObject->GetClass()); It; ++It)
    {
        const auto* Property = *It;

        if (NOT DoShouldIncludeProperty(Property))
        { continue; }

        if (Property->GetOwnerClass() == InStopAtClass)
        { continue; }

        const auto Category = Property->GetMetaData(TEXT("Category"));
        const auto CategoryKey = Category.IsEmpty() ? FString{TEXT("Uncategorized")} : Category;
        CategoryProperties.FindOrAdd(CategoryKey).Add(Property);
        PropertyOrder.Add(Property);
    }

    if (PropertyOrder.Num() == 0)
    {
        OutText += ck::Format_UE(TEXT("{}(No exported properties)\n"), Indent);
        return;
    }

    OutText += ck::Format_UE(TEXT("{}--- Properties ({}) ---\n"), Indent, PropertyOrder.Num());

    // Output grouped by category
    for (const auto& [Category, Props] : CategoryProperties)
    {
        OutText += ck::Format_UE(TEXT("{}  [{}]\n"), Indent, Category);

        ck::algo::ForEach(Props, [&](const FProperty* Property)
        {
            OutText += ck::Format_UE(TEXT("{}    ({}) {} = {}\n"),
                Indent,
                Property->GetCPPType(),
                Property->GetName(),
                DoGetPropertyValueAsString(Property, InObject));
        });
    }

    OutText += TEXT("\n");
}

// --------------------------------------------------------------------------------------------------------------------
// Helpers
// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DataAssetExporter::
    DoResolveOutputPath(
        const UDataAsset* InDataAsset,
        const FString& InExtension)
    -> FString
{
    const auto& PackageName = InDataAsset->GetOutermost()->GetName();

    auto DiskPath = FString{};
    if (NOT FPackageName::TryConvertLongPackageNameToFilename(PackageName, DiskPath))
    {
        return FString{};
    }

    DiskPath += InExtension;
    return DiskPath;
}

auto
    FCk_DataAssetExporter::
    DoGetPropertyValueAsString(
        const FProperty* InProperty,
        const void* InContainer)
    -> FString
{
    auto Value = FString{};
    const auto* ValuePtr = InProperty->ContainerPtrToValuePtr<void>(InContainer);
    InProperty->ExportTextItem_Direct(Value, ValuePtr, nullptr, nullptr, PPF_None);
    return Value;
}

auto
    FCk_DataAssetExporter::
    DoShouldIncludeProperty(
        const FProperty* InProperty)
    -> bool
{
    if (InProperty == nullptr)
    { return false; }

    // Skip transient and deprecated properties
    if (InProperty->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_DuplicateTransient))
    { return false; }

    // Skip properties owned by UObject itself
    const auto* OwnerClass = InProperty->GetOwnerClass();
    if (OwnerClass == nullptr)
    { return false; }

    if (OwnerClass == UObject::StaticClass())
    { return false; }

    // Include if the property is EditAnywhere, VisibleAnywhere, or BlueprintVisible
    if (InProperty->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
    { return true; }

    return false;
}

auto
    FCk_DataAssetExporter::
    DoGetIndent(
        int32 InDepth)
    -> FString
{
    auto Result = FString{};
    for (auto i = int32{0}; i < InDepth; ++i)
    {
        Result += TEXT("  ");
    }
    return Result;
}

// --------------------------------------------------------------------------------------------------------------------
