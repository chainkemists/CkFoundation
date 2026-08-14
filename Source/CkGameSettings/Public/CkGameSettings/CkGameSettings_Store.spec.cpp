#include "CkGameSettings/Storage/CkGameSettings_IniStorageProvider.h"

#include "Misc/AutomationTest.h"

#include <Misc/FileHelper.h>
#include <Misc/Paths.h>
#include <HAL/FileManager.h>

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

namespace ck_game_settings_store_spec
{
    auto Make_Provider(const FString& InFilePath) -> UCk_GameSettings_IniStorageProvider_UE*
    {
        auto* Provider = NewObject<UCk_GameSettings_IniStorageProvider_UE>();
        Provider->Set_FilePathOverride(InFilePath);
        return Provider;
    }

    auto Make_TempFilePath() -> FString
    {
        const auto TempDir = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("Temp"));
        constexpr auto CreateParentTree = true;
        IFileManager::Get().MakeDirectory(*TempDir, CreateParentTree);
        return FPaths::CreateTempFilename(*TempDir, TEXT("CkGameSettingsStoreSpec"), TEXT(".ini"));
    }
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_Store_RoundTripPreservesOrder,
    "Ck.CkGameSettings.Store.RoundTripPreservesOrder",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_Store_RoundTripPreservesOrder::RunTest(const FString&)
{
    const auto TempFilePath = ck_game_settings_store_spec::Make_TempFilePath();

    auto* Writer = ck_game_settings_store_spec::Make_Provider(TempFilePath);
    Writer->Request_StoreValue(ECk_GameSettings_Scope::Machine, 0, TEXT("sg.quality"), TEXT("3"));
    Writer->Request_StoreValue(ECk_GameSettings_Scope::Machine, 0, TEXT("sg.shadows"), TEXT("2"));
    Writer->Request_StoreValue(ECk_GameSettings_Scope::Machine, 0, TEXT("sg.textures"), TEXT("1"));
    Writer->Request_StoreValue(ECk_GameSettings_Scope::Player, 7, TEXT("audio.master"), TEXT("0.8"));

    Writer->Request_StoreValue(ECk_GameSettings_Scope::Machine, 0, TEXT("sg.shadows"), TEXT("0"));

    Writer->Request_Flush();

    auto* Reader = ck_game_settings_store_spec::Make_Provider(TempFilePath);

    const auto MachineValues = Reader->Get_StoredValues(ECk_GameSettings_Scope::Machine, 0);
    TestEqual(TEXT("three Machine values round-tripped"), MachineValues.Num(), 3);
    if (MachineValues.Num() == 3)
    {
        TestEqual(TEXT("first key keeps insertion order"), MachineValues[0].Get_Key(), FName{TEXT("sg.quality")});
        TestEqual(TEXT("updated key keeps its ORIGINAL position"), MachineValues[1].Get_Key(), FName{TEXT("sg.shadows")});
        TestEqual(TEXT("updated key carries the new value"), MachineValues[1].Get_Value(), TEXT("0"));
        TestEqual(TEXT("third key keeps insertion order"), MachineValues[2].Get_Key(), FName{TEXT("sg.textures")});
    }

    const auto PlayerValues = Reader->Get_StoredValues(ECk_GameSettings_Scope::Player, 7);
    TestEqual(TEXT("Player.7 section round-tripped"), PlayerValues.Num(), 1);
    if (PlayerValues.Num() == 1)
    {
        TestEqual(TEXT("player value round-tripped"), PlayerValues[0].Get_Value(), TEXT("0.8"));
    }

    TestEqual(TEXT("other player ids are empty"), Reader->Get_StoredValues(ECk_GameSettings_Scope::Player, 0).Num(), 0);

    IFileManager::Get().Delete(*TempFilePath);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_Store_UnknownLinesSurviveRewrite,
    "Ck.CkGameSettings.Store.UnknownLinesSurviveRewrite",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_Store_UnknownLinesSurviveRewrite::RunTest(const FString&)
{
    const auto TempFilePath = ck_game_settings_store_spec::Make_TempFilePath();

    const auto HandWrittenContent = FString{
        TEXT("; hand-written preamble comment\r\n")
        TEXT("[Machine]\r\n")
        TEXT("# section comment\r\n")
        TEXT("sg.quality=3\r\n")
        TEXT("some unknown line without equals\r\n")
        TEXT("[FutureSection]\r\n")
        TEXT("future.key=42\r\n")};
    TestTrue(TEXT("fixture written"), FFileHelper::SaveStringToFile(HandWrittenContent, *TempFilePath));

    auto* Provider = ck_game_settings_store_spec::Make_Provider(TempFilePath);
    Provider->Request_StoreValue(ECk_GameSettings_Scope::Machine, 0, TEXT("sg.quality"), TEXT("1"));
    Provider->Request_StoreValue(ECk_GameSettings_Scope::Machine, 0, TEXT("sg.new"), TEXT("added"));
    Provider->Request_Flush();

    auto RewrittenContent = FString{};
    TestTrue(TEXT("rewritten file readable"), FFileHelper::LoadFileToString(RewrittenContent, *TempFilePath));

    TestTrue(TEXT("preamble comment preserved"), RewrittenContent.Contains(TEXT("; hand-written preamble comment")));
    TestTrue(TEXT("section comment preserved"), RewrittenContent.Contains(TEXT("# section comment")));
    TestTrue(TEXT("unknown non-KV line preserved"), RewrittenContent.Contains(TEXT("some unknown line without equals")));
    TestTrue(TEXT("unknown section preserved"), RewrittenContent.Contains(TEXT("[FutureSection]")));
    TestTrue(TEXT("unknown section content preserved"), RewrittenContent.Contains(TEXT("future.key=42")));
    TestTrue(TEXT("updated value written"), RewrittenContent.Contains(TEXT("sg.quality=1")));
    TestTrue(TEXT("appended value written"), RewrittenContent.Contains(TEXT("sg.new=added")));

    const auto CommentIndex = RewrittenContent.Find(TEXT("# section comment"));
    const auto QualityIndex = RewrittenContent.Find(TEXT("sg.quality=1"));
    const auto UnknownLineIndex = RewrittenContent.Find(TEXT("some unknown line"));
    TestTrue(TEXT("line order preserved: comment before KV"), CommentIndex < QualityIndex);
    TestTrue(TEXT("line order preserved: KV before unknown line"), QualityIndex < UnknownLineIndex);

    IFileManager::Get().Delete(*TempFilePath);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_Store_NewlineValueRejected,
    "Ck.CkGameSettings.Store.NewlineValueRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_Store_NewlineValueRejected::RunTest(const FString&)
{
    const auto TempFilePath = ck_game_settings_store_spec::Make_TempFilePath();

    auto* Provider = ck_game_settings_store_spec::Make_Provider(TempFilePath);
    Provider->Request_StoreValue(ECk_GameSettings_Scope::Machine, 0, TEXT("good.key"), TEXT("fine"));

    AddExpectedError(TEXT("contains a newline"), EAutomationExpectedErrorFlags::Contains, 0);
    Provider->Request_StoreValue(ECk_GameSettings_Scope::Machine, 0, TEXT("bad.key"), TEXT("line one\nline two"));

    const auto StoredValues = Provider->Get_StoredValues(ECk_GameSettings_Scope::Machine, 0);
    TestEqual(TEXT("only the good value is stored"), StoredValues.Num(), 1);
    if (StoredValues.Num() == 1)
    {
        TestEqual(TEXT("good value intact"), StoredValues[0].Get_Key(), FName{TEXT("good.key")});
    }

    Provider->Request_Flush();

    auto FlushedContent = FString{};
    TestTrue(TEXT("flushed file readable"), FFileHelper::LoadFileToString(FlushedContent, *TempFilePath));
    TestFalse(TEXT("rejected value never reaches the file"), FlushedContent.Contains(TEXT("bad.key")));

    IFileManager::Get().Delete(*TempFilePath);
    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
