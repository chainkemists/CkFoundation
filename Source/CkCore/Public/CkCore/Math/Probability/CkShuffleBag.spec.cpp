#include "CkCore/Math/Probability/CkShuffleBag.h"

#include "CkCore/Format/CkFormat.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ShuffleBag_ExactCountsPerCycle,
    "Ck.CkCore.Math.ShuffleBag_ExactCountsPerCycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ShuffleBag_ExactCountsPerCycle::RunTest(const FString&)
{
    constexpr auto Crit = 1;
    constexpr auto Miss = 0;
    constexpr auto Seed = 1337;

    auto Bag = ck::TShuffleBag<int32>{TArray<int32>{Crit, Crit, Miss, Miss, Miss, Miss, Miss, Miss}, Seed};

    TestEqual(TEXT("total matches contents"), Bag.Get_NumTotal(), 8);
    TestEqual(TEXT("cycle is full after construction"), Bag.Get_NumRemainingInCycle(), 8);

    for (auto Cycle = 0; Cycle < 3; ++Cycle)
    {
        auto NumCrits = 0;

        for (auto DrawIndex = 0; DrawIndex < 8; ++DrawIndex)
        {
            if (Bag.Draw() == Crit)
            { ++NumCrits; }
        }

        TestEqual(*ck::Format_UE(TEXT("cycle [{}] yields exactly 2 crits"), Cycle), NumCrits, 2);
    }

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ShuffleBag_SeededDeterminism,
    "Ck.CkCore.Math.ShuffleBag_SeededDeterminism",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ShuffleBag_SeededDeterminism::RunTest(const FString&)
{
    const auto Contents = TArray<int32>{0, 1, 2, 3, 4, 5, 6, 7};
    constexpr auto Seed = 42;
    constexpr auto OtherSeed = 43;
    constexpr auto NumDraws = 16;

    const auto DrawSequence = [&](int32 InSeed) -> TArray<int32>
    {
        auto Bag = ck::TShuffleBag<int32>{Contents, InSeed};
        auto Sequence = TArray<int32>{};

        for (auto DrawIndex = 0; DrawIndex < NumDraws; ++DrawIndex)
        { Sequence.Add(Bag.Draw()); }

        return Sequence;
    };

    TestTrue(TEXT("same seed produces the same sequence"), DrawSequence(Seed) == DrawSequence(Seed));
    TestTrue(TEXT("different seed produces a different sequence"), DrawSequence(Seed) != DrawSequence(OtherSeed));

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ShuffleBag_RefillAndReset,
    "Ck.CkCore.Math.ShuffleBag_RefillAndReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ShuffleBag_RefillAndReset::RunTest(const FString&)
{
    const auto Contents = TArray<int32>{0, 1, 2, 3, 4, 5, 6, 7};
    constexpr auto Seed = 7;

    auto Bag = ck::TShuffleBag<int32>{Contents, Seed};

    Bag.Draw();
    Bag.Draw();
    Bag.Draw();
    TestEqual(TEXT("3 draws leave 5 in the cycle"), Bag.Get_NumRemainingInCycle(), 5);

    Bag.Reset();
    TestEqual(TEXT("Reset restarts the cycle"), Bag.Get_NumRemainingInCycle(), 8);

    auto DrawnThisCycle = TSet<int32>{};
    for (auto DrawIndex = 0; DrawIndex < 8; ++DrawIndex)
    { DrawnThisCycle.Add(Bag.Draw()); }

    TestEqual(TEXT("a full cycle after Reset yields every outcome once"), DrawnThisCycle.Num(), 8);
    TestEqual(TEXT("cycle is exhausted"), Bag.Get_NumRemainingInCycle(), 0);

    Bag.Draw();
    TestEqual(TEXT("drawing past exhaustion auto-refills"), Bag.Get_NumRemainingInCycle(), 7);
    TestEqual(TEXT("total is unchanged by cycling"), Bag.Get_NumTotal(), 8);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_ShuffleBag_InvalidInput_RejectedLoudly,
    "Ck.CkCore.Math.ShuffleBag_InvalidInput_RejectedLoudly",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_ShuffleBag_InvalidInput_RejectedLoudly::RunTest(const FString&)
{
    AddExpectedError(TEXT("TShuffleBag constructed with empty Contents"), EAutomationExpectedErrorFlags::Contains, 0);
    AddExpectedError(TEXT("Draw called on an empty TShuffleBag"), EAutomationExpectedErrorFlags::Contains, 0);

    auto EmptyContentsBag = ck::TShuffleBag<int32>{TArray<int32>{}};
    TestTrue(TEXT("empty contents are rejected"), EmptyContentsBag.Get_IsEmpty());
    TestEqual(TEXT("no partial state - total is 0"), EmptyContentsBag.Get_NumTotal(), 0);
    TestEqual(TEXT("Draw returns a default value"), EmptyContentsBag.Draw(), 0);
    TestEqual(TEXT("no partial state - cycle is 0"), EmptyContentsBag.Get_NumRemainingInCycle(), 0);

    auto DefaultBag = ck::TShuffleBag<int32>{};
    TestTrue(TEXT("default-constructed bag is empty"), DefaultBag.Get_IsEmpty());
    TestEqual(TEXT("Draw on a default bag returns a default value"), DefaultBag.Draw(), 0);

    return true;
}

// --------------------------------------------------------------------------------------------------------------------

#endif // WITH_DEV_AUTOMATION_TESTS
