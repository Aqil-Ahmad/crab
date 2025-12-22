# AI Navigation System - Integration Guide

This guide provides a complete overview of how the AI Navigation system is integrated into the Grid Image Editor.

## PART 1: Main Application Integration (`src/main.cpp`)

### 1. Headers
```cpp
#include "ai_navigator.h"
#include "ui_highlighter.h"
#include "ai_assistant_window.h"
```

### 2. Global Instances
```cpp
AINavigator ai_navigator;
UIHighlighter ui_highlighter;
AIAssistantWindow* ai_assistant = nullptr;
```

### 3. Initialization
In `main()`, after ImGui initialization:
```cpp
if (ai_navigator.initialize("dependencies/models/Llama-3.2-1B-Instruct-Q4_K_M.gguf")) {
    ai_assistant = new AIAssistantWindow(&ai_navigator, &ui_highlighter);
}
```

### 4. Input Handling
Inside the event loop (`sf::Event::KeyPressed`):
```cpp
// F2: Toggle AI Assistant
if (event.key.code == sf::Keyboard::F2) {
    if (ai_assistant) ai_assistant->toggle();
}

// Escape: Cancel active navigation
if (event.key.code == sf::Keyboard::Escape) {
    if (ui_highlighter.isActive()) ui_highlighter.stopNavigation();
}
```

### 5. Render Loop
At the very end of the frame, before `ImGui::SFML::Render()`:
```cpp
if (ai_assistant) ai_assistant->render();
ui_highlighter.update(); // Draws the dark overlay and highlights
```

---

## PART 2: UI Element Registration (`src/gui.cpp`)

To let the AI point to elements, they must be registered **every frame** immediately after they are created.

### 1. Registering Menus
```cpp
if (ImGui::BeginMenu("Manipulations")) {
    // Register the menu itself so the AI can point to it when closed
    ui_highlighter.registerUIElement("Manipulations menu", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
    
    if (ImGui::MenuItem("Gaussian Blur")) {
        ui_highlighter.onElementClicked("Gaussian Blur button"); // Notify highlighter the user clicked it
        show_gaussian_blur_popup = true;
    }
    // Register the specific menu item
    ui_highlighter.registerUIElement("Gaussian Blur button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
    
    ImGui::EndMenu();
}
```

### 2. Registering Icons/Buttons
```cpp
if (ImGui::Button("Create new layer")) {
    ui_highlighter.onElementClicked("Create new layer button");
    // ... logic ...
}
ui_highlighter.registerUIElement("Create new layer button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
```

---

## PART 3: Naming Convention

For the AI to successfully find a button, the name used in `registerUIElement` must match what the AI expects (defined in your system prompt/feature map).

| UI Element | Code Registration Name | AI Navigator Name |
| :--- | :--- | :--- |
| File Menu | `"File menu"` | `"File menu"` |
| New Image | `"New button"` | `"New button"` |
| Gaussian Blur | `"Gaussian Blur button"` | `"Gaussian Blur button"` |
| Layers Panel | `"Layers"` | `"Layers panel"` |
| Brush Tool | `tool_name` (e.g., `"BrushTool"`) | `"Brush Tool"` |

---

## PART 4: Build and Verify

1. **Build**:
   ```powershell
   cd build_main
   cmake .. -G "MinGW Makefiles"
   cmake --build .
   ```

2. **Run**:
   ```powershell
   ./bin/grid.exe
   ```

3. **Verify**:
   - Check console for: `AI Navigator initialized successfully!`
   - Press **F1**: The chat window should appear.
   - Ask: "How do I apply gaussian blur?"
   - AI Response: Should list steps and start the yellow pulsing highlight.

---

## PART 5: Troubleshooting

- **Highlight is in the wrong place**: Ensure `registerUIElement` is called **immediately after** the ImGui call for that element.
- **AI doesn't respond**: Ensure the model file exists at `dependencies/models/...`.
- **Linking errors**: Run `cmake ..` again to refresh the build files.
