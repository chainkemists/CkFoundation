#include "CkGameSettings_IniStorageProvider.h"

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Format/CkFormat.h"

#include "CkGameSettings/CkGameSettings_Log.h"

#include <HAL/FileManager.h>
#include <HAL/PlatformProperties.h>
#include <Misc/FileHelper.h>
#include <Misc/Paths.h>

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettings_IniStorageProvider_UE::
    Get_StorageFilePath() const
    -> FString
{
    if (NOT _FilePathOverride.IsEmpty())
    { return _FilePathOverride; }

    return FPaths::Combine(FPaths::GeneratedConfigDir(), FPlatformProperties::PlatformName(), TEXT("CkGameSettings.ini"));
}

auto
    UCk_GameSettings_IniStorageProvider_UE::
    Set_FilePathOverride(
        const FString& InFilePath)
    -> void
{
    _FilePathOverride = InFilePath;
    _PreambleLines.Reset();
    _Sections.Reset();
    _Loaded = false;
    _Dirty = false;
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettings_IniStorageProvider_UE::
    Get_StoredValues_Implementation(
        ECk_GameSettings_Scope InScope,
        int32 InPlatformUserId) const
    -> TArray<FCk_GameSettings_StoredValue>
{
    DoEnsureLoaded();

    const auto* Section = DoFindSection(Get_SectionName(InScope, InPlatformUserId));

    if (Section == nullptr)
    { return {}; }

    auto Result = TArray<FCk_GameSettings_StoredValue>{};

    for (const auto& Line : Section->_Lines)
    {
        if (Line._Key.IsNone())
        { continue; }

        auto EqualsIndex = int32{INDEX_NONE};
        Line._Raw.FindChar(TEXT('='), EqualsIndex);
        Result.Add(FCk_GameSettings_StoredValue{Line._Key, Line._Raw.Mid(EqualsIndex + 1)});
    }

    return Result;
}

auto
    UCk_GameSettings_IniStorageProvider_UE::
    Request_StoreValue_Implementation(
        ECk_GameSettings_Scope InScope,
        int32 InPlatformUserId,
        FName InKey,
        const FString& InValue)
    -> void
{
    const auto ValueHasNewline = InValue.Contains(TEXT("\n")) || InValue.Contains(TEXT("\r"));
    CK_ENSURE_IF_NOT(NOT ValueHasNewline, TEXT("GameSettings value for key [{}] contains a newline and cannot be stored in the ini provider"), InKey)
    { return; }

    DoEnsureLoaded();

    auto& Section = DoFindOrAddSection(Get_SectionName(InScope, InPlatformUserId));
    const auto NewRawLine = ck::Format_UE(TEXT("{}={}"), InKey, InValue);

    if (auto* ExistingLine = Section._Lines.FindByPredicate([&](const FIniLine& InLine) { return InLine._Key == InKey; }))
    {
        if (ExistingLine->_Raw.Equals(NewRawLine))
        { return; }

        ExistingLine->_Raw = NewRawLine;
    }
    else
    {
        Section._Lines.Add(FIniLine{NewRawLine, InKey});
    }

    _Dirty = true;
}

auto
    UCk_GameSettings_IniStorageProvider_UE::
    Request_RemoveValue_Implementation(
        ECk_GameSettings_Scope InScope,
        int32 InPlatformUserId,
        FName InKey)
    -> void
{
    DoEnsureLoaded();

    auto* Section = DoFindSection(Get_SectionName(InScope, InPlatformUserId));

    if (Section == nullptr)
    { return; }

    const auto RemovedCount = Section->_Lines.RemoveAll([&](const FIniLine& InLine) { return InLine._Key == InKey; });

    if (RemovedCount > 0)
    { _Dirty = true; }
}

auto
    UCk_GameSettings_IniStorageProvider_UE::
    Request_Flush_Implementation()
    -> void
{
    if (NOT _Dirty)
    { return; }

    auto Lines = TArray<FString>{};
    Lines.Append(_PreambleLines);

    for (const auto& Section : _Sections)
    {
        Lines.Add(ck::Format_UE(TEXT("[{}]"), Section._Name));

        for (const auto& Line : Section._Lines)
        { Lines.Add(Line._Raw); }
    }

    const auto Content = FString::Join(Lines, LINE_TERMINATOR) + LINE_TERMINATOR;
    const auto FilePath = Get_StorageFilePath();
    const auto TempFilePath = FilePath + TEXT(".tmp");

    constexpr auto CreateParentTree = true;
    IFileManager::Get().MakeDirectory(*FPaths::GetPath(FilePath), CreateParentTree);

    const auto TempFileSaved = FFileHelper::SaveStringToFile(Content, *TempFilePath, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    CK_ENSURE_IF_NOT(TempFileSaved, TEXT("GameSettings ini flush could not write the temp file [{}], values remain unflushed"), TempFilePath)
    { return; }

    constexpr auto ReplaceExisting = true;
    constexpr auto EvenIfReadOnly = true;
    const auto TempFileMoved = IFileManager::Get().Move(*FilePath, *TempFilePath, ReplaceExisting, EvenIfReadOnly);
    CK_ENSURE_IF_NOT(TempFileMoved, TEXT("GameSettings ini flush could not move the temp file into [{}], values remain unflushed"), FilePath)
    { return; }

    _Dirty = false;
    ck::game_settings::Verbose(TEXT("Flushed GameSettings store to [{}]"), FilePath);
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_GameSettings_IniStorageProvider_UE::
    DoEnsureLoaded() const
    -> void
{
    if (_Loaded)
    { return; }

    _Loaded = true;
    _PreambleLines.Reset();
    _Sections.Reset();

    auto FileContent = FString{};
    if (NOT FFileHelper::LoadFileToString(FileContent, *Get_StorageFilePath()))
    { return; }

    auto FileLines = TArray<FString>{};
    constexpr auto CullEmptyLines = false;
    FileContent.ParseIntoArrayLines(FileLines, CullEmptyLines);

    auto CurrentSectionIndex = int32{INDEX_NONE};

    for (const auto& FileLine : FileLines)
    {
        const auto TrimmedLine = FileLine.TrimStartAndEnd();

        if (TrimmedLine.StartsWith(TEXT("[")) && TrimmedLine.EndsWith(TEXT("]")))
        {
            CurrentSectionIndex = _Sections.Emplace(FIniSection{TrimmedLine.Mid(1, TrimmedLine.Len() - 2), {}});
            continue;
        }

        auto Key = FName{};
        const auto IsCommentLine = TrimmedLine.StartsWith(TEXT(";")) || TrimmedLine.StartsWith(TEXT("#"));

        if (NOT IsCommentLine)
        {
            auto EqualsIndex = int32{INDEX_NONE};
            if (FileLine.FindChar(TEXT('='), EqualsIndex) && EqualsIndex > 0)
            { Key = FName{*FileLine.Left(EqualsIndex).TrimStartAndEnd()}; }
        }

        if (CurrentSectionIndex == INDEX_NONE)
        {
            _PreambleLines.Add(FileLine);
            continue;
        }

        _Sections[CurrentSectionIndex]._Lines.Add(FIniLine{FileLine, Key});
    }
}

auto
    UCk_GameSettings_IniStorageProvider_UE::
    DoFindSection(
        const FString& InSectionName) const
    -> const FIniSection*
{
    return _Sections.FindByPredicate([&](const FIniSection& InSection) { return InSection._Name.Equals(InSectionName); });
}

auto
    UCk_GameSettings_IniStorageProvider_UE::
    DoFindSection(
        const FString& InSectionName)
    -> FIniSection*
{
    return _Sections.FindByPredicate([&](const FIniSection& InSection) { return InSection._Name.Equals(InSectionName); });
}

auto
    UCk_GameSettings_IniStorageProvider_UE::
    DoFindOrAddSection(
        const FString& InSectionName)
    -> FIniSection&
{
    if (auto* ExistingSection = _Sections.FindByPredicate([&](const FIniSection& InSection) { return InSection._Name.Equals(InSectionName); }))
    { return *ExistingSection; }

    const auto NewSectionIndex = _Sections.Emplace(FIniSection{InSectionName, {}});
    return _Sections[NewSectionIndex];
}

auto
    UCk_GameSettings_IniStorageProvider_UE::
    Get_SectionName(
        ECk_GameSettings_Scope InScope,
        int32 InPlatformUserId)
    -> FString
{
    switch (InScope)
    {
        case ECk_GameSettings_Scope::Machine:
        {
            return TEXT("Machine");
        }
        case ECk_GameSettings_Scope::Player:
        {
            return ck::Format_UE(TEXT("Player.{}"), InPlatformUserId);
        }
        default:
        {
            CK_INVALID_ENUM(InScope);
            return TEXT("Machine");
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
