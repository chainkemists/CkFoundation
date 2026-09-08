# CkSlateLayout

For file-based HTML/CSS-like authoring and live reload, start with [AUTHORING.md](AUTHORING.md).
FCkUiView supplies named regions, retained native bindings, actions, styles, and atomic reload.
The C++ primitives below remain the underlying integration API.

`SCkFlexBox` keeps the full typed-slot API for padding, min/max sizes, alignment, and custom measurement. For common debugger composition, `CkFlex.h` offers compact item helpers:

```cpp
return ck::slate::Row(8.0f,
{
    ck::slate::Content(SNew(STextBlock).Text(Label)),
    ck::slate::Fill(SNew(SScrollBox)),
});
```

`Content` accepts horizontal and vertical alignment and uses grow/shrink `0`; `Fill` uses grow/shrink `1`. `Row` and `Column` accept optional panel padding. Declarative `+ SCkFlexBox::Slot()` entries are admitted before `.Items(...)` entries when both are supplied.
