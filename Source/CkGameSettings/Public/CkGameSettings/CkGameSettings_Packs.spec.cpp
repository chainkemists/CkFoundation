#include "CkGameSettings/Packs/CkGameSettings_VideoPack.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_GameSettings_Packs_VideoValueMapping,
    "Ck.CkGameSettings.Packs.VideoValueMapping",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_GameSettings_Packs_VideoValueMapping::RunTest(const FString&)
{
    auto Resolution = FIntPoint{};

    TestTrue(TEXT("plain form parses"), ck::game_settings::TryParse_Resolution(TEXT("1920x1080"), Resolution));
    TestEqual(TEXT("width parsed"), Resolution.X, 1920);
    TestEqual(TEXT("height parsed"), Resolution.Y, 1080);

    TestTrue(TEXT("separator is case-insensitive"), ck::game_settings::TryParse_Resolution(TEXT("2560X1440"), Resolution));
    TestEqual(TEXT("uppercase separator width"), Resolution.X, 2560);

    TestTrue(TEXT("surrounding whitespace tolerated"), ck::game_settings::TryParse_Resolution(TEXT(" 1280 x 720 "), Resolution));
    TestEqual(TEXT("whitespace form width"), Resolution.X, 1280);

    TestFalse(TEXT("zero dimension rejected"), ck::game_settings::TryParse_Resolution(TEXT("0x1080"), Resolution));
    TestFalse(TEXT("negative dimension rejected"), ck::game_settings::TryParse_Resolution(TEXT("-1920x1080"), Resolution));
    TestFalse(TEXT("non-numeric rejected"), ck::game_settings::TryParse_Resolution(TEXT("axb"), Resolution));
    TestFalse(TEXT("empty rejected"), ck::game_settings::TryParse_Resolution(TEXT(""), Resolution));
    TestFalse(TEXT("missing height rejected"), ck::game_settings::TryParse_Resolution(TEXT("1920x"), Resolution));
    TestFalse(TEXT("extra separator rejected"), ck::game_settings::TryParse_Resolution(TEXT("1x2x3"), Resolution));
    TestFalse(TEXT("float dimensions rejected"), ck::game_settings::TryParse_Resolution(TEXT("1920.5x1080"), Resolution));

    TestEqual(TEXT("format round-trips"), ck::game_settings::Format_Resolution(FIntPoint{3440, 1440}), TEXT("3440x1440"));

    TestTrue(TEXT("format-then-parse round-trips"), ck::game_settings::TryParse_Resolution(
        ck::game_settings::Format_Resolution(FIntPoint{1234, 567}), Resolution));
    TestEqual(TEXT("round-tripped width"), Resolution.X, 1234);
    TestEqual(TEXT("round-tripped height"), Resolution.Y, 567);

    TestEqual(TEXT("0 maps to Fullscreen"), ck::game_settings::Get_WindowModeFromInt(0), EWindowMode::Fullscreen);
    TestEqual(TEXT("1 maps to WindowedFullscreen"), ck::game_settings::Get_WindowModeFromInt(1), EWindowMode::WindowedFullscreen);
    TestEqual(TEXT("2 maps to Windowed"), ck::game_settings::Get_WindowModeFromInt(2), EWindowMode::Windowed);
    TestEqual(TEXT("out-of-range collapses to Windowed"), ck::game_settings::Get_WindowModeFromInt(99), EWindowMode::Windowed);

    TestEqual(TEXT("twelve video keys registered by the pack"), ck::game_settings::Get_VideoSettingKeys().Num(), 12);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
