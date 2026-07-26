# CkAttributeEditor

**Editor module.** Details panels for attribute asset types — modifier stacks, provider pickers, min/max visualization.

**Runtime twin:** `CkAttribute`.

For the full reference — purpose, unique editor additions, rules, and shared infrastructure — see [`/Source/EDITOR_MODULES.md`](/Source/EDITOR_MODULES.md).

## Implementation notes

- **The `MinMaxMode_*` constants in the Byte/Float/Integer customizations mirror `ECk_MinMax`.**
  `IPropertyHandle::GetValue` exposes the enum property as its underlying `uint8`, so the
  customizations compare raw byte values (0/1/2/3) rather than the enum type. Keep the constants in
  step with `ECk_MinMax` if new modes are ever added.
- **`_RefillParams` is added with a plain `StructBuilder.AddProperty` on purpose** (Float and Integer
  customizations). The property's `InlineEditConditionToggle` already draws its own enable checkbox —
  wrapping it in a custom row would produce a second, redundant toggle.
