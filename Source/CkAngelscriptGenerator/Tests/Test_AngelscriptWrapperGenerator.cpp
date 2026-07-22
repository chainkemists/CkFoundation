#include "CkAngelscriptGenerator/Tests/Test_AngelscriptWrapperGenerator_Fixtures.h"

#include "CkAngelscriptGenerator/CkAngelscriptWrapperGenerator.h"

#include "Misc/AutomationTest.h"

auto
    UCkTest_AngelscriptWrapperGenerator_Library::
    HiddenFromAngelscript()
    -> int32
{
    return 1;
}

auto
    UCkTest_AngelscriptWrapperGenerator_Library::
    CallableFromAngelscript()
    -> int32
{
    return 2;
}

#if WITH_DEV_AUTOMATION_TESTS

// --------------------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_AngelscriptWrapperGenerator_FunctionFilter,
    "CkAngelscriptGenerator.UnitTests.WrapperGenerator.FunctionFilter_NotInAngelscript",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_AngelscriptWrapperGenerator_FunctionFilter::RunTest(const FString&)
{
    auto* LibraryClass = UCkTest_AngelscriptWrapperGenerator_Library::StaticClass();
    auto* HiddenFunction = LibraryClass->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UCkTest_AngelscriptWrapperGenerator_Library, HiddenFromAngelscript));
    auto* CallableFunction = LibraryClass->FindFunctionByName(
        GET_FUNCTION_NAME_CHECKED(UCkTest_AngelscriptWrapperGenerator_Library, CallableFromAngelscript));

    if (NOT TestNotNull(TEXT("NotInAngelscript function reflected"), HiddenFunction))
    { return false; }

    if (NOT TestNotNull(TEXT("neighboring callable function reflected"), CallableFunction))
    { return false; }

    const auto HiddenWrapper = FCkAngelscriptWrapperGenerator::Get_GeneratedWrapperFunction(
        HiddenFunction,
        LibraryClass->GetName(),
        false,
        false);
    const auto CallableWrapper = FCkAngelscriptWrapperGenerator::Get_GeneratedWrapperFunction(
        CallableFunction,
        LibraryClass->GetName(),
        false,
        false);

    TestTrue(TEXT("NotInAngelscript function is omitted"), HiddenWrapper.IsEmpty());
    TestFalse(TEXT("neighboring callable static is emitted"), CallableWrapper.IsEmpty());
    TestTrue(TEXT("emitted wrapper names the callable static"),
        CallableWrapper.Contains(TEXT("CallableFromAngelscript")));
    TestFalse(TEXT("emitted wrapper does not name the excluded static"),
        CallableWrapper.Contains(TEXT("HiddenFromAngelscript")));

    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
