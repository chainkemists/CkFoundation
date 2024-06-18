#include "CkUI_Utils.h"

#include <UserWidget.h>
#include <NamedSlot.h>
#include <WidgetTree.h>

#include "CkCore/Ensure/CkEnsure.h"
#include "CkCore/Validation/CkIsValid.h"

auto
	UCk_Utils_UI_UE::
	FindNamedSlotAndInsertWidget(
		UUserWidget* InSourceWidget,
		UUserWidget* InInsertedWidget,
		FName InNamedSlotName,
		bool bEnsureSlotFound)
	-> UPanelSlot*
{
	CK_ENSURE_IF_NOT(ck::IsValid(InSourceWidget),
		TEXT("Widget source for NamedSlot [{}] is not valid"),
		InNamedSlotName)
	{ return nullptr; }
	
	CK_ENSURE_IF_NOT(ck::IsValid(InInsertedWidget),
		TEXT("Widget to be inserted into NamedSlot [{}] is not valid"),
		InNamedSlotName)
	{ return nullptr; }
	
	auto RootWidgetTree = InSourceWidget->WidgetTree.Get();
	
	CK_ENSURE_IF_NOT(ck::IsValid(RootWidgetTree),
		TEXT("Widget tree of widget [{}] is not valid"),
		InSourceWidget)
	{ return nullptr; }
	
	auto Widgets = TArray<UWidget*>{};
	RootWidgetTree->GetAllWidgets(Widgets);
	auto FoundNamedSlot = Widgets.FindByPredicate([&](UWidget* Widget)
	{
		return Widget->GetFName() == InNamedSlotName && ck::IsValid(Cast<UNamedSlot>(Widget));
	});
	if (ck::IsValid(FoundNamedSlot, ck::IsValid_Policy_NullptrOnly{}) &&
		ck::IsValid(Cast<UNamedSlot>(*FoundNamedSlot)))
	{
		return Cast<UNamedSlot>(*FoundNamedSlot)->AddChild(InInsertedWidget);
	}
	
	CK_ENSURE(!bEnsureSlotFound,
		TEXT("Widget [{}] does not contain named slot [{}]"),
		InSourceWidget,
		InNamedSlotName);
	
	return nullptr;
}
