#include "CkMessageDialog.h"

#include "CkCore/Algorithms/CkAlgorithms.h"
#include "CkCore/Ensure/CkEnsure.h"

#include <Dialog/SCustomDialog.h>
#include <Misc/MessageDialog.h>

// --------------------------------------------------------------------------------------------------------------------

namespace ck_message_dialog
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION < 3
    auto
        DoGet_TitleOrNullptr(
            const FText& InTitle)
        -> const FText*
    {
        return InTitle.IsEmpty() ? nullptr : &InTitle;
    }
#else
    auto
        DoGet_TitleOrNullptr(
            const FText& InTitle)
        -> FText
    {
        return InTitle;
    }
#endif
}

// --------------------------------------------------------------------------------------------------------------------

auto
    UCk_Utils_MessageDialog_UE::
    Ok(
        FText InMessage,
        FText InTitle)
    -> void
{
    FMessageDialog::Open(
        EAppMsgType::Ok,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle));
}

auto
    UCk_Utils_MessageDialog_UE::
    YesNo(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_YesNo
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::YesNo,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Cancel:
        case EAppReturnType::No:
        {
            return ECk_MessageDialog_YesNo::No;
        }
        case EAppReturnType::Yes:
        {
            return ECk_MessageDialog_YesNo::Yes;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_YesNo::No;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    OkCancel(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_OkCancel
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::OkCancel,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Ok:
        {
            return ECk_MessageDialog_OkCancel::Okay;
        }
        case EAppReturnType::Cancel:
        {
            return ECk_MessageDialog_OkCancel::Cancel;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_OkCancel::Cancel;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    YesNoCancel(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_YesNoCancel
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::YesNoCancel,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Yes:
        {
            return ECk_MessageDialog_YesNoCancel::Yes;
        }
        case EAppReturnType::No:
        {
            return ECk_MessageDialog_YesNoCancel::No;
        }
        case EAppReturnType::Cancel:
        {
            return ECk_MessageDialog_YesNoCancel::Cancel;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_YesNoCancel::Cancel;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    CancelRetryContinue(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_CancelRetryContinue
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::CancelRetryContinue,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Cancel:
        {
            return ECk_MessageDialog_CancelRetryContinue::Cancel;
        }
        case EAppReturnType::Retry:
        {
            return ECk_MessageDialog_CancelRetryContinue::Retry;
        }
        case EAppReturnType::Continue:
        {
            return ECk_MessageDialog_CancelRetryContinue::Continue;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_CancelRetryContinue::Cancel;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    YesNoYesAllNoAll(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_YesNoYesAllNoAll
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::YesNoYesAllNoAll,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Yes:
        {
            return ECk_MessageDialog_YesNoYesAllNoAll::Yes;
        }
        case EAppReturnType::Cancel:
        case EAppReturnType::No:
        {
            return ECk_MessageDialog_YesNoYesAllNoAll::No;
        }
        case EAppReturnType::YesAll:
        {
            return ECk_MessageDialog_YesNoYesAllNoAll::YesAll;
        }
        case EAppReturnType::NoAll:
        {
            return ECk_MessageDialog_YesNoYesAllNoAll::NoAll;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_YesNoYesAllNoAll::NoAll;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    YesNoYesAllNoAllCancel(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_YesNoYesAllNoAllCancel
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::YesNoYesAllNoAllCancel,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Yes:
        {
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::Yes;
        }
        case EAppReturnType::No:
        {
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::No;
        }
        case EAppReturnType::YesAll:
        {
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::YesAll;
        }
        case EAppReturnType::NoAll:
        {
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::NoAll;
        }
        case EAppReturnType::Cancel:
        {
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::Cancel;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::NoAll;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    YesNoYesAll(
        FText InMessage,
        FText InTitle)
    -> ECk_MessageDialog_YesNoYesAll
{
    switch (const auto Res = FMessageDialog::Open(
        EAppMsgType::YesNoYesAll,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle)))
    {
        case EAppReturnType::Yes:
        {
            return ECk_MessageDialog_YesNoYesAll::Yes;
        }
        case EAppReturnType::Cancel:
        case EAppReturnType::No:
        {
            return ECk_MessageDialog_YesNoYesAll::No;
        }
        case EAppReturnType::YesAll:
        {
            return ECk_MessageDialog_YesNoYesAll::YesAll;
        }
        default:
        {
            CK_INVALID_ENUM(Res);
            return ECk_MessageDialog_YesNoYesAll::No;
        }
    }
}

auto
    UCk_Utils_MessageDialog_UE::
    CustomDialog(
        const FText& InMessage,
        const FText& InTitle,
        const TArray<DialogButton> InButtons)
    -> int32
{
    CK_ENSURE_IF_NOT(NOT InButtons.IsEmpty(), TEXT("Cannot create Custom Dialog with no buttons"))
    { return INDEX_NONE; }

    const auto& Buttons = ck::algo::Transform<TArray<SCustomDialog::FButton>>(InButtons, [&](const DialogButton& InButton)
    {
        auto Button = SCustomDialog::FButton{InButton.Name, InButton.OnClicked};
        Button.bIsPrimary = InButton.IsPrimary;
        Button.bShouldFocus = InButton.ShouldFocus;

        return Button;
    });

    const auto Dialog = SNew(SCustomDialog)
        .Title(InTitle)
        .Content()
        [
            SNew(STextBlock).Text(InMessage)
        ]
        .Buttons(Buttons);

    return Dialog->ShowModal();
}

// --------------------------------------------------------------------------------------------------------------------
