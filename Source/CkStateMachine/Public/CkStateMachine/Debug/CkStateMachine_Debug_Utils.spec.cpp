#include "CkStateMachine/Debug/CkStateMachine_Debug_Utils.h"

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkTest_StateMachine_DebugCaptureVisibilityGeneration,
    "Ck.StateMachine.DebugCapture.VisibilityGeneration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCkTest_StateMachine_DebugCaptureVisibilityGeneration::RunTest(const FString&)
{
    UCk_Utils_StateMachineDebug_UE::Set_IsDebuggerCaptureVisible(false);
    const auto InitialGeneration =
        UCk_Utils_StateMachineDebug_UE::Get_DebuggerCaptureGeneration();

    UCk_Utils_StateMachineDebug_UE::Set_IsDebuggerCaptureVisible(true);
    TestTrue(TEXT("opening arms debugger capture"),
        UCk_Utils_StateMachineDebug_UE::Get_IsDebuggerCaptureVisible());
    TestEqual(TEXT("opening starts a new capture generation"),
        UCk_Utils_StateMachineDebug_UE::Get_DebuggerCaptureGeneration(),
        InitialGeneration + 1);

    UCk_Utils_StateMachineDebug_UE::Set_IsDebuggerCaptureVisible(true);
    TestEqual(TEXT("repeated visible updates keep the same generation"),
        UCk_Utils_StateMachineDebug_UE::Get_DebuggerCaptureGeneration(),
        InitialGeneration + 1);

    UCk_Utils_StateMachineDebug_UE::Set_IsDebuggerCaptureVisible(false);
    TestFalse(TEXT("closing disarms debugger capture"),
        UCk_Utils_StateMachineDebug_UE::Get_IsDebuggerCaptureVisible());
    TestEqual(TEXT("closing does not create a generation"),
        UCk_Utils_StateMachineDebug_UE::Get_DebuggerCaptureGeneration(),
        InitialGeneration + 1);

    UCk_Utils_StateMachineDebug_UE::Set_IsDebuggerCaptureVisible(true);
    TestEqual(TEXT("reopening starts another capture generation"),
        UCk_Utils_StateMachineDebug_UE::Get_DebuggerCaptureGeneration(),
        InitialGeneration + 2);

    UCk_Utils_StateMachineDebug_UE::Set_IsDebuggerCaptureVisible(false);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && !UE_BUILD_SHIPPING
