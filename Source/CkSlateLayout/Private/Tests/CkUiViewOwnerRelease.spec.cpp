#include "CkSlateLayout/SCkUiSurface.h"

#include "Misc/AutomationTest.h"
#include "Widgets/Input/SButton.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace ck_ui_view_owner_release_test
{
    auto FindButton(const TSharedRef<SWidget>& InRoot) -> TSharedPtr<SButton>
    {
        if (InRoot->GetTypeAsString() == TEXT("SButton")
            || InRoot->GetTypeAsString() == TEXT("SCkUiStyledButton"))
        { return StaticCastSharedRef<SButton>(InRoot); }

        FChildren* Children = InRoot->GetChildren();
        for (int32 Index = 0; Children != nullptr && Index < Children->Num(); ++Index)
        {
            if (const auto Found = FindButton(ConstCastSharedRef<SWidget>(Children->GetChildAt(Index)));
                Found.IsValid())
            { return Found; }
        }
        return {};
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCkUiViewOwnerRelease,
    "Ck.UiAuthoring.SlateLayout.OwnerRelease",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

auto FCkUiViewOwnerRelease::RunTest(const FString&) -> bool
{
    auto DispatchCount = int32{0};
    auto Actions = FCkUiView::FActions{};
    Actions.Add(TEXT("activate"), FSimpleDelegate::CreateLambda([&DispatchCount]() { ++DispatchCount; }));

    const auto View = FCkUiView::Create({}, MoveTemp(Actions));
    const auto Main = View->GetRegion(TEXT("main"));
    const auto Markup = FString{TEXT(
        "<ui version=\"1\"><region name=\"main\"><button id=\"owner-action\" action=\"activate\">Run</button></region></ui>")};
    if (!TestTrue(TEXT("owner-release fixture admits an actionable view"),
        View->TryReload(Markup, TEXT(""), TEXT("OwnerRelease initial")).Succeeded))
    { return false; }

    const auto Button = ck_ui_view_owner_release_test::FindButton(Main);
    if (!TestTrue(TEXT("owner-release fixture mounts its real button"), Button.IsValid()))
    { return false; }

    Button->SimulateClick();
    TestEqual(TEXT("mounted view dispatches before owner release"), DispatchCount, 1);

    View->ReleaseOwnerInteractions();
    View->ReleaseOwnerInteractions();
    Button->SimulateClick();
    TestEqual(TEXT("held button is inert after idempotent owner release"), DispatchCount, 1);

    TestTrue(TEXT("held released view may reload for diagnostics"),
        View->TryReload(Markup, TEXT(""), TEXT("OwnerRelease retained reload")).Succeeded);
    const auto ReloadedButton = ck_ui_view_owner_release_test::FindButton(Main);
    if (ReloadedButton.IsValid()) { ReloadedButton->SimulateClick(); }
    TestEqual(TEXT("reload cannot revive owner interactions"), DispatchCount, 1);
    return true;
}

#endif
