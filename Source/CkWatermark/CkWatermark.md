# CkWatermark

In-game debug HUD showing performance metrics: FPS, ping, memory, VRAM, CPU, network type, ECS stats, and more. Configurable detail levels with color-coded quality indicators.

## Key Concepts

- **Display Policy** — Four levels: Hidden, Minimal, Regular, Detailed. Controls how much info is shown.
- **Stat Quality** — Color-coded: VeryGood (green) → Bad (red). Thresholds configurable per stat via color band structs.
- **Modular Widgets** — 20+ stat widgets (FPS, Ping, Memory, VRAM, CPU, FrameTime, etc.) that auto-query engine systems.
- **Activity Bar** — Visual indicator of current system activity.

## Example: Toggling Debug Overlay

```mermaid
flowchart LR
    A["Press debug key"] -->|"SetDisplayPolicy(Detailed)"| B["Full HUD appears:<br/>FPS, Ping, Memory, etc."]
    B -->|"SetDisplayPolicy(Hidden)"| C["HUD hidden"]
```

## Usage Examples

### Change display level (from Blueprint)

```cpp
WatermarkWidget->Request_SetDisplayPolicy(ECk_Watermark_DisplayPolicy::Detailed);
```

### Override in Blueprint

Override `OnDisplayPolicyChanged()` in your widget Blueprint to customize what's shown at each detail level.

## Tests

No tests found for this module in CkTest.
