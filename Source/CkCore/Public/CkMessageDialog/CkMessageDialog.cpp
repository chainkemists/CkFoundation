#include "CkMessageDialog.h"

#include <Misc/MessageDialog.h>

#include "CkEnsure/CkEnsure.h"

// --------------------------------------------------------------------------------------------------------------------

namespace ck_message_dialog
{
    auto
    DoGet_TitleOrNullptr(const FText& InTitle)
        -> const FText*
    {
        return InTitle.IsEmpty() ? nullptr : &InTitle;
    }

}

// --------------------------------------------------------------------------------------------------------------------

auto UCk_Utils_MessageDialog_UE::
    Ok(FText InMessage, FText InTitle) -> void
{
    FMessageDialog::Open(
        EAppMsgType::Ok,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle));
}

auto UCk_Utils_MessageDialog_UE::
    YesNo(FText InMessage, FText InTitle)
        -> ECk_MessageDialog_YesNo
{

    const auto res = FMessageDialog::Open(
        EAppMsgType::YesNo,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle));

    switch (res)
    {
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
            CK_INVALID_ENUM(res);
            return ECk_MessageDialog_YesNo::No;
        }
    }
}

auto UCk_Utils_MessageDialog_UE::
    OkCancel(FText InMessage, FText InTitle)
        -> ECk_MessageDialog_OkCancel
{
    const auto res = FMessageDialog::Open(
        EAppMsgType::OkCancel,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle));

    switch (res)
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
            CK_INVALID_ENUM(res);
            return ECk_MessageDialog_OkCancel::Cancel;
        }
    }
}

auto UCk_Utils_MessageDialog_UE::
    YesNoCancel(FText InMessage, FText InTitle)
        -> ECk_MessageDialog_YesNoCancel
{
    const auto res = FMessageDialog::Open(
        EAppMsgType::YesNoCancel,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle));

    switch (res)
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
            CK_INVALID_ENUM(res);
            return ECk_MessageDialog_YesNoCancel::Cancel;
        }
    }
}

auto UCk_Utils_MessageDialog_UE::
    CancelRetryContinue(FText InMessage, FText InTitle)
        -> ECk_MessageDialog_CancelRetryContinue
{
    const auto res = FMessageDialog::Open(
        EAppMsgType::CancelRetryContinue,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle));

    switch (res)
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
            CK_INVALID_ENUM(res);
            return ECk_MessageDialog_CancelRetryContinue::Cancel;
        }
    }
}

auto UCk_Utils_MessageDialog_UE::
    YesNoYesAllNoAll(FText InMessage, FText InTitle)
        -> ECk_MessageDialog_YesNoYesAllNoAll
{
    const auto res = FMessageDialog::Open(
        EAppMsgType::YesNoYesAllNoAll,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle));

    switch (res)
    {
        case EAppReturnType::Yes:
        {
            return ECk_MessageDialog_YesNoYesAllNoAll::Yes;
        }
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
            CK_INVALID_ENUM(res);
            return ECk_MessageDialog_YesNoYesAllNoAll::NoAll;
        }
    }
}

auto UCk_Utils_MessageDialog_UE::
    YesNoYesAllNoAllCancel(FText InMessage, FText InTitle)
        -> ECk_MessageDialog_YesNoYesAllNoAllCancel
{
    const auto res = FMessageDialog::Open(
        EAppMsgType::YesNoYesAllNoAllCancel,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle));

    switch (res)
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
            CK_INVALID_ENUM(res);
            return ECk_MessageDialog_YesNoYesAllNoAllCancel::NoAll;
        }
    }
}

auto UCk_Utils_MessageDialog_UE::
    YesNoYesAll(FText InMessage, FText InTitle)
        -> ECk_MessageDialog_YesNoYesAll
{
    const auto res = FMessageDialog::Open(
        EAppMsgType::YesNoYesAll,
        InMessage,
        ck_message_dialog::DoGet_TitleOrNullptr(InTitle));

    switch (res)
    {
        case EAppReturnType::Yes:
        {
            return ECk_MessageDialog_YesNoYesAll::Yes;
        }
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
            CK_INVALID_ENUM(res);
            return ECk_MessageDialog_YesNoYesAll::No;
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
