#include "CkDataAssetExporter.h"

#include "CkAssetExporter_Log.h"
#include "CkAssetExporter/ExportMeta/CkAssetExporter_ExportMeta.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"
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

namespace ck_data_asset_exporter_internal
{
    // Per-export visited sets so a DAG-shaped reference graph (shared referenced UObjects, fragments that
    // back-reference the owning DataAsset) emits "alreadyExported": true instead of re-descending. The depth caps are
    // a secondary guard. Thread-local, so any caller threading model is safe; reset per ExportDataAsset call.
    static constexpr int32 GMaxObjectRecursionDepth   = 24;
    static constexpr int32 GMaxPropertyRecursionDepth = 128;
    thread_local int32 GObjectRecursionDepth   = 0;
    thread_local int32 GPropertyRecursionDepth = 0;
    thread_local TSet<const UObject*> GObjectsAlreadyExported;
    thread_local TSet<const void*>    GStructMemoryAlreadyExported;

    // GetCPPType dereferences these without null checks; a field whose referenced type was deleted
    // (or sits behind a dead redirector) loads with a null class pointer and crashes the sweep.
    auto
    Get_IsCPPTypeResolvable(
        const FProperty* InProperty)
        -> bool
    {
        if (InProperty == nullptr)
        { return false; }

        if (const auto* ClassProperty = CastField<FClassProperty>(InProperty))
        { return ClassProperty->PropertyClass != nullptr && ClassProperty->MetaClass != nullptr; }

        if (const auto* SoftClassProperty = CastField<FSoftClassProperty>(InProperty))
        { return SoftClassProperty->PropertyClass != nullptr && SoftClassProperty->MetaClass != nullptr; }

        if (const auto* ObjectProperty = CastField<FObjectPropertyBase>(InProperty))
        { return ObjectProperty->PropertyClass != nullptr; }

        if (const auto* InterfaceProperty = CastField<FInterfaceProperty>(InProperty))
        { return InterfaceProperty->InterfaceClass != nullptr; }

        if (const auto* StructProperty = CastField<FStructProperty>(InProperty))
        { return StructProperty->Struct != nullptr; }

        if (const auto* EnumProperty = CastField<FEnumProperty>(InProperty))
        { return EnumProperty->GetEnum() != nullptr; }

        if (const auto* ArrayProperty = CastField<FArrayProperty>(InProperty))
        { return Get_IsCPPTypeResolvable(ArrayProperty->Inner); }

        if (const auto* SetProperty = CastField<FSetProperty>(InProperty))
        { return Get_IsCPPTypeResolvable(SetProperty->ElementProp); }

        if (const auto* MapProperty = CastField<FMapProperty>(InProperty))
        { return Get_IsCPPTypeResolvable(MapProperty->KeyProp) && Get_IsCPPTypeResolvable(MapProperty->ValueProp); }

        return true;
    }
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DataAssetExporter::
    ExportDataAsset(
        UDataAsset* InDataAsset,
        ECk_AssetExporter_SidecarFormats InFormats)
    -> FCk_DataAssetExportResult
{
    using namespace ck_data_asset_exporter_internal;

    auto Result = FCk_DataAssetExportResult{};

    if (ck::Is_NOT_Valid(InDataAsset))
    {
        Result.ErrorMessage = TEXT("Invalid DataAsset");
        return Result;
    }

    // Thread-locals persist for the thread's lifetime: without this, the second export sees the first export's
    // entries and marks everything "alreadyExported".
    ResetSharedRecursionState();

    Result.AssetName = InDataAsset->GetName();

    const auto JsonObject = DoSerializeToJson(InDataAsset);
    if (NOT JsonObject.IsValid())
    {
        Result.ErrorMessage = TEXT("Failed to serialize DataAsset to JSON");
        return Result;
    }

    auto JsonString = FString{};
    const auto JsonWriter = TJsonWriterFactory<>::Create(&JsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), JsonWriter);

    const auto WriteText = ck::asset_exporter::ShouldWrite_SummaryText(InFormats);

    const auto TextString = WriteText ? DoSerializeToText(InDataAsset) : FString{};

    const auto JsonPath = DoResolveOutputPath(InDataAsset, ck::asset_exporter::extension::Sidecar);
    const auto TextPath = WriteText ? DoResolveOutputPath(InDataAsset, ck::asset_exporter::extension::SummaryText) : FString{};

    if (JsonPath.IsEmpty() || (WriteText && TextPath.IsEmpty()))
    {
        Result.ErrorMessage = TEXT("Failed to resolve output file paths");
        return Result;
    }

    const auto JsonWritten = FFileHelper::SaveStringToFile(
        JsonString, *JsonPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    const auto TextWritten = NOT WriteText || FFileHelper::SaveStringToFile(
        TextString, *TextPath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);

    if (NOT JsonWritten || NOT TextWritten)
    {
        Result.ErrorMessage = ck::Format_UE(TEXT("Failed to write files (JSON: {}, Text: {})"),
            JsonWritten ? TEXT("OK") : TEXT("FAILED"),
            TextWritten ? TEXT("OK") : TEXT("FAILED"));
        return Result;
    }

    Result.Succeeded = true;
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
    RootObject->SetObjectField(TEXT("_meta"), FCk_AssetExportMeta::MakeMetaObject(InDataAsset, ck::asset_exporter::version::DataAsset));

    auto ParentChain = TArray<TSharedPtr<FJsonValue>>{};
    for (const auto* Class = InDataAsset->GetClass()->GetSuperClass();
         Class != nullptr && Class != UObject::StaticClass();
         Class = Class->GetSuperClass())
    {
        ParentChain.Add(MakeShared<FJsonValueString>(Class->GetName()));
    }
    RootObject->SetArrayField(TEXT("parentClasses"), ParentChain);

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

        // Only the stop class ITSELF is excluded — members declared by its child classes are still exported, which
        // is how UObject/UDataAsset internals drop out while PDA-hierarchy members survive.
        if (Property->GetOwnerClass() == InStopAtClass)
        { continue; }

        auto PropObject = MakeShared<FJsonObject>();
        PropObject->SetStringField(TEXT("name"), Property->GetName());
        PropObject->SetStringField(TEXT("type"), Get_SafeCPPType(Property));

        const auto Category = Property->GetMetaData(TEXT("Category"));
        if (NOT Category.IsEmpty())
        {
            PropObject->SetStringField(TEXT("category"), Category);
        }

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
    using namespace ck_data_asset_exporter_internal;

    if (InProperty == nullptr || InValuePtr == nullptr)
    { return MakeShared<FJsonValueNull>(); }

    // A property whose referenced type no longer exists cannot be value-serialized either —
    // ExportTextItem/struct walks dereference the same null pointer Get_SafeCPPType guards against.
    if (NOT Get_IsCPPTypeResolvable(InProperty))
    { return MakeShared<FJsonValueString>(TEXT("<unresolved type>")); }

    // ONE budget shared by every kind of descent (struct member, FInstancedStruct payload, container element,
    // instanced object): recursion can wander through structs and arrays indefinitely without ever reaching the
    // UObject-only guard below, and a pathologically deep property graph then blows the stack.
    if (GPropertyRecursionDepth >= GMaxPropertyRecursionDepth)
    { return MakeShared<FJsonValueString>(TEXT("<truncated: max property depth>")); }

    ++GPropertyRecursionDepth;
    ON_SCOPE_EXIT { --GPropertyRecursionDepth; };

    if (const auto* BoolProp = CastField<FBoolProperty>(InProperty))
    {
        return MakeShared<FJsonValueBoolean>(BoolProp->GetPropertyValue(InValuePtr));
    }

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

    if (const auto* ByteProp = CastField<FByteProperty>(InProperty))
    {
        const auto Value = ByteProp->GetSignedIntPropertyValue(InValuePtr);
        if (ByteProp->Enum != nullptr)
        {
            return MakeShared<FJsonValueString>(ByteProp->Enum->GetNameStringByValue(Value));
        }
        return MakeShared<FJsonValueNumber>(static_cast<double>(Value));
    }

    if (const auto* NumericProp = CastField<FNumericProperty>(InProperty))
    {
        if (NumericProp->IsFloatingPoint())
        {
            return MakeShared<FJsonValueNumber>(NumericProp->GetFloatingPointPropertyValue(InValuePtr));
        }
        return MakeShared<FJsonValueNumber>(static_cast<double>(NumericProp->GetSignedIntPropertyValue(InValuePtr)));
    }

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

        // FInstancedStruct has no reflected fields of its own (TFieldIterator over it yields nothing), so without
        // this branch every entry of a TArray<FInstancedStruct> exports as an empty {}.
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

            if (GStructMemoryAlreadyExported.Contains(InnerMemory))
            {
                WrapperObject->SetBoolField(TEXT("alreadyExported"), true);
                return MakeShared<FJsonValueObject>(WrapperObject);
            }

            GStructMemoryAlreadyExported.Add(InnerMemory);

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

        // NEVER address-dedup plain inline structs: embedded by value, an address only "repeats" for a member at
        // offset 0 of its parent (FTransform's first reflected member shares the FTransform's address), and deduping
        // there silently loses real data. FInstancedStruct payloads and UObjects have their own guards.
        auto StructObject = MakeShared<FJsonObject>();

        for (TFieldIterator<FProperty> It(StructProp->Struct); It; ++It)
        {
            const auto* InnerProp = *It;
            if (InnerProp == nullptr)
            { continue; }

            // Struct members are owned by a UScriptStruct, not a UClass, so DoShouldIncludeProperty's UClass-based
            // filter would reject every one of them — hence this lighter filter.
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

    if (const auto* ClassProp = CastField<FClassProperty>(InProperty))
    {
        const auto* ClassPtr = ClassProp->GetPropertyValue(InValuePtr).Get();
        return MakeShared<FJsonValueString>(ClassPtr != nullptr ? ClassPtr->GetPathName() : TEXT("None"));
    }

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

    auto FallbackStr = FString{};
    InProperty->ExportTextItem_Direct(FallbackStr, InValuePtr, nullptr, nullptr, PPF_None);
    return MakeShared<FJsonValueString>(FallbackStr);
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
    Result->SetStringField(TEXT("objectPath"), InObject->GetPathName());

    if (GObjectsAlreadyExported.Contains(InObject))
    {
        Result->SetBoolField(TEXT("alreadyExported"), true);
        return Result;
    }

    if (GObjectRecursionDepth >= GMaxObjectRecursionDepth)
    {
        Result->SetBoolField(TEXT("truncated"), true);
        return Result;
    }

    GObjectsAlreadyExported.Add(InObject);
    ++GObjectRecursionDepth;
    ON_SCOPE_EXIT { --GObjectRecursionDepth; };

    Result->SetArrayField(TEXT("properties"),
        DoSerializeProperties_Json(InObject, InStopAtClass));

    return Result;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_DataAssetExporter::
    DoSerializeToText(
        const UDataAsset* InDataAsset)
    -> FString
{
    auto Text = FCk_AssetExportMeta::Get_SummaryTextBanner(InDataAsset->GetName());

    Text += ck::Format_UE(TEXT("=== DataAsset: {} ===\n"), InDataAsset->GetName());
    Text += ck::Format_UE(TEXT("Path: {}\n"), InDataAsset->GetPathName());
    Text += ck::Format_UE(TEXT("Class: {}\n"), InDataAsset->GetClass()->GetName());

    auto ParentNames = TArray<FString>{};
    for (const auto* Class = InDataAsset->GetClass()->GetSuperClass();
         Class != nullptr && Class != UObject::StaticClass();
         Class = Class->GetSuperClass())
    {
        ParentNames.Add(Class->GetName());
    }
    Text += ck::Format_UE(TEXT("Parent Classes: {}\n"), FString::Join(ParentNames, TEXT(" -> ")));

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

    for (const auto& [Category, Props] : CategoryProperties)
    {
        OutText += ck::Format_UE(TEXT("{}  [{}]\n"), Indent, Category);

        ck::algo::ForEach(Props, [&](const FProperty* Property)
        {
            OutText += ck::Format_UE(TEXT("{}    ({}) {} = {}\n"),
                Indent,
                Get_SafeCPPType(Property),
                Property->GetName(),
                DoGetPropertyValueAsString(Property, InObject));
        });
    }

    OutText += TEXT("\n");
}

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
    if (NOT ck_data_asset_exporter_internal::Get_IsCPPTypeResolvable(InProperty))
    { return TEXT("<unresolved type>"); }

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

    if (InProperty->HasAnyPropertyFlags(CPF_Transient | CPF_Deprecated | CPF_DuplicateTransient))
    { return false; }

    const auto* OwnerClass = InProperty->GetOwnerClass();
    if (OwnerClass == nullptr)
    { return false; }

    if (OwnerClass == UObject::StaticClass())
    { return false; }

    if (InProperty->HasAnyPropertyFlags(CPF_Edit | CPF_BlueprintVisible))
    { return true; }

    return false;
}

auto
    FCk_DataAssetExporter::
    Get_SafeCPPType(
        const FProperty* InProperty)
    -> FString
{
    const auto TypeIsResolvable = ck_data_asset_exporter_internal::Get_IsCPPTypeResolvable(InProperty);
    CK_ENSURE_IF_NOT(TypeIsResolvable,
        TEXT("Property [{}] on [{}] has an unresolved type (deleted class/struct/enum or dead redirector) — "
             "exporting UNRESOLVED sentinel instead of its CPP type"),
        InProperty ? InProperty->GetName() : TEXT("<null>"),
        InProperty ? InProperty->GetOwnerVariant().GetName() : TEXT("<null>"))
    { return ck::Format_UE(TEXT("UNRESOLVED({})"), InProperty ? InProperty->GetName() : TEXT("<null>")); }

    return InProperty->GetCPPType();
}

auto
    FCk_DataAssetExporter::
    ResetSharedRecursionState()
    -> void
{
    using namespace ck_data_asset_exporter_internal;

    GObjectRecursionDepth   = 0;
    GPropertyRecursionDepth = 0;
    GObjectsAlreadyExported.Reset();
    GStructMemoryAlreadyExported.Reset();
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
