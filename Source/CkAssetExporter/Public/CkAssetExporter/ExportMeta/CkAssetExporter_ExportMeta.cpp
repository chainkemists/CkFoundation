#include "CkAssetExporter_ExportMeta.h"

#include "CkCore/Macros/CkMacros.h"
#include "CkCore/Validation/CkIsValid.h"

#include <Dom/JsonObject.h>
#include <HAL/FileManager.h>
#include <Misc/FileHelper.h>
#include <Misc/PackageName.h>
#include <Misc/SecureHash.h>
#include <Serialization/JsonReader.h>
#include <Serialization/JsonSerializer.h>
#include <UObject/Object.h>
#include <UObject/Package.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExportMeta::
    Get_SourcePackageFilename(
        const UObject* InAsset)
    -> FString
{
    if (ck::Is_NOT_Valid(InAsset))
    { return {}; }

    const auto PackageName = InAsset->GetOutermost()->GetName();

    const auto AssetFile = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
    if (IFileManager::Get().FileExists(*AssetFile))
    { return AssetFile; }

    const auto MapFile = FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetMapPackageExtension());
    if (IFileManager::Get().FileExists(*MapFile))
    { return MapFile; }

    return {};
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExportMeta::
    Get_SourceHash(
        const UObject* InAsset)
    -> FString
{
    const auto Filename = Get_SourcePackageFilename(InAsset);
    if (Filename.IsEmpty())
    { return {}; }

    const auto Hash = FMD5Hash::HashFile(*Filename);
    if (NOT Hash.IsValid())
    { return {}; }

    return LexToString(Hash);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExportMeta::
    MakeMetaObject(
        const UObject* InAsset,
        int32 InExporterVersion)
    -> TSharedPtr<FJsonObject>
{
    auto Meta = MakeShared<FJsonObject>();
    Meta->SetStringField(TEXT("sourceHash"), Get_SourceHash(InAsset));
    Meta->SetNumberField(TEXT("exporterVersion"), InExporterVersion);
    return Meta;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExportMeta::
    TryRead_SiblingMeta(
        const FString& InJsonFilePath,
        FString& OutSourceHash,
        int32& OutExporterVersion)
    -> bool
{
    OutSourceHash.Reset();
    OutExporterVersion = 0;

    if (NOT IFileManager::Get().FileExists(*InJsonFilePath))
    { return false; }

    auto JsonString = FString{};
    if (NOT FFileHelper::LoadFileToString(JsonString, *InJsonFilePath))
    { return false; }

    auto RootObject = TSharedPtr<FJsonObject>{};
    const auto Reader = TJsonReaderFactory<>::Create(JsonString);
    if (NOT FJsonSerializer::Deserialize(Reader, RootObject) || NOT RootObject.IsValid())
    { return false; }

    const TSharedPtr<FJsonObject>* MetaPtr = nullptr;
    if (NOT RootObject->TryGetObjectField(TEXT("_meta"), MetaPtr) || MetaPtr == nullptr || NOT MetaPtr->IsValid())
    { return false; }

    const auto& Meta = *MetaPtr;

    auto Hash = FString{};
    if (NOT Meta->TryGetStringField(TEXT("sourceHash"), Hash))
    { return false; }

    auto Version = double{0.0};
    if (NOT Meta->TryGetNumberField(TEXT("exporterVersion"), Version))
    { return false; }

    OutSourceHash = Hash;
    OutExporterVersion = static_cast<int32>(Version);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    FCk_AssetExportMeta::
    Get_IsExportFresh(
        const UObject* InAsset,
        const FString& InSiblingJsonPath,
        int32 InCurrentExporterVersion)
    -> bool
{
    auto StoredHash = FString{};
    auto StoredVersion = int32{0};
    if (NOT TryRead_SiblingMeta(InSiblingJsonPath, StoredHash, StoredVersion))
    { return false; }

    if (StoredVersion != InCurrentExporterVersion)
    { return false; }

    const auto CurrentHash = Get_SourceHash(InAsset);
    if (CurrentHash.IsEmpty())
    { return false; }

    return StoredHash == CurrentHash;
}

// --------------------------------------------------------------------------------------------------------------------
