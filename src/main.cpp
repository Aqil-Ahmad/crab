#include <list>
#include <map>
#include <SFML/Graphics.hpp>
#include <imgui.h>
#include <imgui-SFML.h>
#include <ImGuiFileDialog.h>

#include "Action.h"
#include "gui.h"
#include "Variables.h"
#include "util.h"
#include "Canvas.h"
#include "Assets.h"
#include "Tools.h"
#include "Filters.h"
#include "Undo_Redo.h"
#include "ai_navigator.h"
#include "ui_highlighter.h"
#include "ai_assistant_window.h"
#include "ui_config.h"
#include "layout_manager.h"
#include "ui_colors.h"

//................................................. MACROS .................................................
#define DEBUG 1 // macro to enable or disable custom debugging

//................................................. GLOBALS VARIABLES! .................................................
std::map<std::list<i32>, std::string> action_map;   // maps keys or shortcuts to action names
std::list<i32> currently_pressed_keys;              // to track the current shortcut / pressed keys' sequence
Variables vars;                                     // variables that imgui and actions use and change
bool shaders_available = true;                      // tracks whether shaders are available in the machine
extern const char* layer_types_str[];               // see Layer.h

// AI Navigation System
AINavigator ai_navigator;
UIHighlighter ui_highlighter;
AIAssistantWindow* ai_assistant = nullptr;

//................................................. SOME USEFUL FUNCS .................................................
// draws a line from point p1 to point p2 of the specified color in the window
void drawline(vec2 p1, vec2 p2, sf::Color color, sf::RenderWindow& window)
{
    sf::Vertex line[] = {
        { vec2(p1.x, p1.y), color },
        { vec2(p2.x, p2.y), color }
    };
    window.draw(line, 2, sf::Lines);   
}

// just stops the loop and closes the window
void quit(bool& running, sf::RenderWindow& window)
{
    running = false;
    window.close();
}

//................................................. MAIN .................................................
i32 main()
{
    //................................................. INITIALIZATION .................................................
    sf::RenderWindow window(sf::VideoMode(1280, 720), "Grid", sf::Style::Default);
    window.setKeyRepeatEnabled(false);
    window.setFramerateLimit(60);
    sf::Clock delta_clock;
    bool running = true;
    ui64 frames = 0;
    bool first_frame = true;

    ImGui::SFML::Init(window);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    // ============ MODERN UI STYLING ============
    // Load font with better rendering
    io.Fonts->Clear();
    ImFontConfig font_config;
    font_config.OversampleH = 2;  // Sharper font rendering
    font_config.OversampleV = 2;
    font_config.PixelSnapH = true;
    io.Fonts->AddFontFromFileTTF("assets/fonts/caviar-dreams.ttf", 20.0f, &font_config);
    ImGui::SFML::UpdateFontTexture();
    
    // Apply modern color scheme and styling
    ImGuiStyle& style = ImGui::GetStyle();
    
    // Modern Colors (high contrast dark theme)
    style.Colors[ImGuiCol_Text]                  = UIColors::Text();
    style.Colors[ImGuiCol_TextDisabled]          = UIColors::TextDisabled();
    style.Colors[ImGuiCol_WindowBg]              = UIColors::WindowBg();
    style.Colors[ImGuiCol_ChildBg]               = UIColors::ChildBg();
    style.Colors[ImGuiCol_PopupBg]               = UIColors::PopupBg();
    style.Colors[ImGuiCol_Border]                = UIColors::Border();
    style.Colors[ImGuiCol_BorderShadow]          = UIColors::BorderShadow();
    style.Colors[ImGuiCol_FrameBg]               = UIColors::FrameBg();
    style.Colors[ImGuiCol_FrameBgHovered]        = UIColors::FrameBgHover();
    style.Colors[ImGuiCol_FrameBgActive]         = UIColors::FrameBgActive();
    style.Colors[ImGuiCol_TitleBg]               = UIColors::TitleBg();
    style.Colors[ImGuiCol_TitleBgActive]         = UIColors::TitleBgActive();
    style.Colors[ImGuiCol_TitleBgCollapsed]      = UIColors::TitleBg();
    style.Colors[ImGuiCol_MenuBarBg]             = UIColors::Header();
    style.Colors[ImGuiCol_ScrollbarBg]           = UIColors::Scrollbar();
    style.Colors[ImGuiCol_ScrollbarGrab]         = UIColors::ScrollbarGrab();
    style.Colors[ImGuiCol_ScrollbarGrabHovered]  = UIColors::ScrollbarHover();
    style.Colors[ImGuiCol_ScrollbarGrabActive]   = UIColors::ScrollbarActive();
    style.Colors[ImGuiCol_CheckMark]             = UIColors::Primary();
    style.Colors[ImGuiCol_SliderGrab]            = UIColors::SliderGrab();
    style.Colors[ImGuiCol_SliderGrabActive]      = UIColors::SliderGrabActive();
    style.Colors[ImGuiCol_Button]                = UIColors::Button();
    style.Colors[ImGuiCol_ButtonHovered]         = UIColors::ButtonHover();
    style.Colors[ImGuiCol_ButtonActive]          = UIColors::ButtonActive();
    style.Colors[ImGuiCol_Header]                = UIColors::Header();
    style.Colors[ImGuiCol_HeaderHovered]         = UIColors::HeaderHover();
    style.Colors[ImGuiCol_HeaderActive]          = UIColors::HeaderActive();
    style.Colors[ImGuiCol_Separator]             = UIColors::Border();
    style.Colors[ImGuiCol_SeparatorHovered]      = UIColors::Primary();
    style.Colors[ImGuiCol_SeparatorActive]       = UIColors::PrimaryActive();
    style.Colors[ImGuiCol_ResizeGrip]            = UIColors::Primary();
    style.Colors[ImGuiCol_ResizeGripHovered]     = UIColors::PrimaryHover();
    style.Colors[ImGuiCol_ResizeGripActive]      = UIColors::PrimaryActive();
    style.Colors[ImGuiCol_Tab]                   = UIColors::Tab();
    style.Colors[ImGuiCol_TabHovered]            = UIColors::TabHover();
    style.Colors[ImGuiCol_TabActive]             = UIColors::TabActive();
    style.Colors[ImGuiCol_TabUnfocused]          = UIColors::Tab();
    style.Colors[ImGuiCol_TabUnfocusedActive]    = UIColors::Header();
    style.Colors[ImGuiCol_DockingPreview]        = UIColors::Primary();
    style.Colors[ImGuiCol_DockingEmptyBg]        = UIColors::WindowBg();
    
    // Spacing and Sizing (generous, modern feel)
    style.WindowPadding     = ImVec2(12.0f, 12.0f);
    style.FramePadding      = ImVec2(10.0f, 6.0f);
    style.ItemSpacing       = ImVec2(8.0f, 6.0f);
    style.ItemInnerSpacing  = ImVec2(6.0f, 4.0f);
    style.IndentSpacing     = 20.0f;
    style.ScrollbarSize     = 14.0f;
    style.GrabMinSize       = 12.0f;
    
    // Borders and Rounding (modern look)
    style.WindowBorderSize  = 1.0f;
    style.ChildBorderSize   = 1.0f;
    style.PopupBorderSize   = 1.0f;
    style.FrameBorderSize   = 0.0f;
    style.TabBorderSize     = 0.0f;
    
    style.WindowRounding    = 6.0f;
    style.ChildRounding     = 4.0f;
    style.FrameRounding     = 4.0f;
    style.PopupRounding     = 4.0f;
    style.ScrollbarRounding = 9.0f;
    style.GrabRounding      = 4.0f;
    style.TabRounding       = 4.0f;

    if (!sf::Shader::isAvailable())
    {
        // handle this more gracefully
        std::cerr << "Your machine does not support shaders. Without shaders, `grid` may not work properly or efficiently.\n";
        std::cerr << "Perhaps your GPU drivers need to be updated?\n";
        shaders_available = false;
    }

    // Initialize AI System
    if (ai_navigator.initialize("dependencies/models/Llama-3.2-1B-Instruct-Q4_K_M.gguf")) {
        ai_assistant = new AIAssistantWindow(&ai_navigator, &ui_highlighter);
    }

    // ============ LAYOUT MANAGER: Load saved layout or use defaults ============
    g_layout_manager.loadLayout("layout.json");
    // Register default window positions (only applied if no saved layout)
    g_layout_manager.registerWindow("Layers", ImVec2(20, 60), ImVec2(280, 500));
    g_layout_manager.registerWindow("Color", ImVec2(320, 60), ImVec2(280, 350));
    g_layout_manager.registerWindow("Tools", ImVec2(620, 60), ImVec2(200, 300));
    g_layout_manager.registerWindow("Canvas", ImVec2(300, 100), ImVec2(800, 600));
    g_layout_manager.registerWindow("AI Assistant", ImVec2(840, 60), ImVec2(350, 400));
    g_layout_manager.registerWindow("Debug Information", ImVec2(840, 480), ImVec2(350, 200));

    // load the assets and register keybinds or shortcuts
    Assets assets;
    assets.loadfromfile("assets.txt");
    // register_action({sf::Keyboard::LAlt}, "toggle_menubar");
    register_action({sf::Keyboard::LControl, sf::Keyboard::N}, "new");
    register_action({sf::Keyboard::LControl, sf::Keyboard::O}, "open");
    register_action({sf::Keyboard::LControl, sf::Keyboard::LShift, sf::Keyboard::S}, "saveas");
    register_action({sf::Keyboard::LControl, sf::Keyboard::Z}, "undo");
    register_action({sf::Keyboard::LControl, sf::Keyboard::Y}, "redo");
    register_action({sf::Keyboard::LControl, sf::Keyboard::LShift, sf::Keyboard::R}, "reset_canvas_navigation");
    register_action({sf::Keyboard::N}, "no_tool");
    register_action({sf::Keyboard::V}, "move_tool");
    register_action({sf::Keyboard::B}, "brush_tool");
    register_action({sf::Keyboard::LBracket}, "tool_size_down");
    register_action({sf::Keyboard::RBracket}, "tool_size_up");
    register_action({sf::Keyboard::B}, "brush_tool");
    register_action({sf::Keyboard::E}, "eraser_tool");
    register_action({sf::Keyboard::G}, "fill_tool");
    register_action({sf::Keyboard::Z}, "zoom_tool");

    // the one and the only, canvas!
    Canvas canvas(vec2(window.getSize().x * 0.7, window.getSize().y * 0.85));
    Undo_redo ur;
    ur.canv = &canvas;

    // da tools
    Tools tools(&canvas, &ur);
    const char* tools_tooltips[Tools::NUM_TOOLS] = {    // used by imgui tooltips
        "No tool. Literally no tool is selected at all.. (N)",
        "Move tool. Moves layers using the mouse input. (V)",
        "Brush tool. The typical paint brush tool that can paint over layers, given its settings. (B)",
        "Eraser tool. Erases brush strokes on a layer. (E)",
        "Fill tool. Flood fills a color onto a valid region of a layer. (G)",
        "Zoom tool. Left-click to zoom in, right-click to zoom out. (Z)",
    };
    canvas.assets = &assets;
    canvas.tools = &tools;

    // da filters
    Filters filters(&canvas, &ur);
    ur.filters = &filters;

    // cursor
    sf::CircleShape circ(5 * canvas.zoom_factor, 128);
    circ.setFillColor(sf::Color(207, 207, 196, 75));
    circ.setOutlineColor(sf::Color(196, 196, 207, 175));

    //................................................. MAIN LOOP .................................................
    while (running && window.isOpen())
    {
        // Update delta time at the start of each frame
        vars.delta_time = delta_clock.restart().asSeconds();

        //................................................. EVENT HANDLING .................................................
        sf::Event event;
        while (window.pollEvent(event))
        {
            // pass the event to imgui to be parsed
            ImGui::SFML::ProcessEvent(window, event);

            // when the X button is clicked
            if (event.type == sf::Event::Closed)
                quit(running, window);

            // when the window is resized update the window's view and canvas' view-related variables to  ensure no stretching occurs
            if (event.type == sf::Event::Resized)
            {
                sf::FloatRect visibleArea(0.f, 0.f, event.size.width, event.size.height);
                window.setView(sf::View(visibleArea));
            }

            // when the window is not focused clear the pressed keys to avoid weird behaviors
            if (event.type == sf::Event::LostFocus)
                currently_pressed_keys.clear();

            // main keyboard events below
            if (event.type == sf::Event::KeyPressed)
            {
                if (event.key.code == sf::Keyboard::Escape)
                {
                    if (vars.show_new_img_dialog) vars.show_new_img_dialog = false;
                    else if (vars.show_open_img_dialog) vars.show_open_img_dialog = false;
                    else if (vars.show_saveas_img_dialog) vars.show_saveas_img_dialog = false;

                    if (ImGuiFileDialog::Instance()->IsOpened())
                        ImGuiFileDialog::Instance()->Close();
                }

                // AI Shortcuts
                if (event.key.code == sf::Keyboard::F2) {
                    if (ai_assistant) ai_assistant->toggle();
                }
                if (event.key.code == sf::Keyboard::Escape) {
                    if (ui_highlighter.isActive()) ui_highlighter.stopNavigation();
                }
                // Advance to next step with Space, Enter, or Right arrow
                // Pass true to advanceToNextStep to also execute the action
                if (ui_highlighter.isActive()) {
                    if (event.key.code == sf::Keyboard::Space || 
                        event.key.code == sf::Keyboard::Enter ||
                        event.key.code == sf::Keyboard::Right) {
                        ui_highlighter.advanceToNextStep(true);  // true = execute the action
                    }
                }

                // remember the pressed key
                currently_pressed_keys.emplace_back(event.key.code);

                // no need to process the keys if it isn't in the action map
                if (action_map.find(currently_pressed_keys) == action_map.end())
                    continue;

                // call do_action with the correct action data
                Action a = Action(action_map.at(currently_pressed_keys), Action::START);
                a.tools = &tools;
                a.ur = &ur;
                do_action(a);
            }
            else if (event.type == sf::Event::KeyReleased)
            {
                // forget the released key
                currently_pressed_keys.remove(event.key.code);
            }

            // main mouse events below
            auto mpos = sf::Mouse::getPosition(window);
            vars.mouse_pos = vec2(mpos.x, mpos.y);

            // when a mouse button is pressed
            if (event.type == sf::Event::MouseButtonPressed)
            {
                switch (event.mouseButton.button)
                {
                case sf::Mouse::Left:
                {
                    Layer* current_layer = canvas.current_layer();
                    if (current_layer)
                    {
                        // the following is the move tool's activation logic
                        vec2 layer_size;
                        if (current_layer->type == Layer::RASTER)
                            layer_size = ((Raster*)current_layer->graphic)->texture.getSize();
                        else;

                        sf::FloatRect bounds(current_layer->pos, layer_size);
                        if (bounds.contains(canvas.mouse_p))
                        {
                            tools.is_dragging = true;
                            tools.layer_offset = canvas.mouse_p - current_layer->pos;
                        }
                    }

                    do_action(Action("left_click", Action::START, vars.mouse_pos));
                    break;
                }
                case sf::Mouse::Middle: { do_action(Action("middle_click", Action::START, vars.mouse_pos)); break; }
                case sf::Mouse::Right:  { do_action(Action("right_click", Action::START, vars.mouse_pos)); break; }
                default: { break; }
                }
            }

            // when a mouse button is released from pressage
            if (event.type == sf::Event::MouseButtonReleased)
            {
                switch (event.mouseButton.button)
                {
                case sf::Mouse::Left:
                {
                    tools.is_dragging = false;

                    do_action(Action("left_click", Action::END, vars.mouse_pos));
                    break;
                }
                case sf::Mouse::Middle: { do_action(Action("middle_click", Action::END, vars.mouse_pos)); break; }
                case sf::Mouse::Right:  { do_action(Action("right_click", Action::END, vars.mouse_pos)); break; }
                default: { break; }
                }
            }

            // when mouse is just moved
            if (event.type == sf::Event::MouseMoved)
            {   
                static vec2 prev_pos;
                do_action(Action("mouse_move", vec2(event.mouseMove.x, event.mouseMove.y), prev_pos));
                prev_pos = vec2(event.mouseMove.x, event.mouseMove.y);
            }
            
            // when mouse is scrolled
            if (event.type == sf::Event::MouseWheelScrolled)
            {
                do_action(Action("mouse_scroll", event.mouseWheelScroll.delta));
            }
        }

        //................................................. UPDATE THE STATE OF THE PROGRAM, DRAW / RENDER GUI, CANVAS, AND EVERYTHING ELSE BELOW! .................................................
        ImGui::SFML::Update(window, delta_clock.restart());
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport());
#if 0
        ImGui::ShowDemoWindow();
#endif

        //................................................. CANVAS DRAWING & WINDOW .................................................
        // draw canvas' stuff to the RenderTexture
        if (window.hasFocus()) canvas.draw();

        // the canvas window
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::Begin("Canvas", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        // detect whether the canvas is resized
        ImVec2 current_canv_win_size = ImGui::GetWindowSize();
        if (current_canv_win_size.x != canvas.window_size.x || current_canv_win_size.y != canvas.window_size.y)
            canvas.window_size = vec2(ImGui::GetWindowSize().x, ImGui::GetWindowSize().y);
        // draw the canvas' RenderTexture
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0.f, 0.f));
        ImGui::ImageButton("Canvas", canvas.window_texture.getTexture().getNativeHandle(), ImVec2(canvas.window_texture.getSize().x, canvas.window_texture.getSize().y), ImVec2(0, 1), ImVec2(1, 0));
        ImGui::PopStyleVar();
        // compute the relative coordinates for mouse position
        ImVec2 abs_mouse_p = ImGui::GetMousePos();
        ImVec2 abs_screen_p = ImGui::GetItemRectMin();
        canvas.mouse_p = vec2(abs_mouse_p.x - abs_screen_p.x, abs_mouse_p.y - abs_screen_p.y);
        canvas.mouse_p = canvas.window_texture.mapPixelToCoords((vec2i)(canvas.mouse_p));
        vars.canvas_focused = ImGui::IsItemHovered();
        ImGui::End();
        ImGui::PopStyleVar();

        // mouse cursors
        window.setMouseCursorVisible(!vars.canvas_focused || tools.current_tool == Tools::NO); // show window cursor when not on canvas or show always when no tool is selected
        if ((tools.current_tool == Tools::MOVE || tools.current_tool == Tools::FILL || tools.current_tool == Tools::ZOOM) && vars.canvas_focused)
        {
            circ.setRadius(7 * canvas.zoom_factor);
            circ.setOutlineThickness(1 * canvas.zoom_factor);
            circ.setPosition(canvas.mouse_p - vec2(circ.getRadius(), circ.getRadius()));
            canvas.window_texture.draw(circ);
        }
        else if ((tools.current_tool == Tools::BRUSH || tools.current_tool == Tools::ERASER) && vars.canvas_focused)
        {
            circ.setRadius((tools.current_tool == Tools::BRUSH ? tools.brush_size : tools.eraser_size) / 2.f);
            circ.setOutlineThickness(1 * canvas.zoom_factor);
            circ.setPosition(canvas.mouse_p - vec2(circ.getRadius(), circ.getRadius()));
            canvas.window_texture.draw(circ);
        }
        canvas.window_texture.display();

        //................................................. THE LAYERS PANEL .................................................
        ImGui::Begin("Layers", nullptr, g_layout_manager.getWindowFlags());
        // gives ability to customize the currently selected layer
        Layer* current_layer = canvas.current_layer();
        if (current_layer)
        {
            ImGui::Combo("Blend Mode", (i32*)&current_layer->blend, layer_blend_str, IM_ARRAYSIZE(layer_blend_str));
            ImGui::DragFloat("Opecity", &current_layer->opacity, 0.5f, 0.f, 100.f, "%.1f");
            ImGui::InputText("Name", current_layer->name, IM_ARRAYSIZE(current_layer->name));
            ImGui::Text("Layer type: %s", current_layer->type_or_blend_to_cstr());
            ImGui::Spacing(); ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
        }
        
        // each layer interactable widget
        for (i32 n = canvas.layers.size() - 1; n >= 0; n--)
        {
            Layer& layer = canvas.layers[n];
            if (layer.is_deleted)
                continue;

            ImGui::PushID(n);
            
            // un/hides the layer
            ImGui::Checkbox("##", &layer.is_visible);
            ImGui::SameLine();

            // duplicates the layer and puts it on top of the original layer
            if (ImGui::Button("D"))
            {
                Layer duplicate((const Layer&)layer);
                strncpy(duplicate.name, canvas.default_layer_name(), LAYER_NAME_MAX_LENGTH - 1);
                canvas.layers.insert(canvas.layers.begin() + n + 1, duplicate);

                Edit e(Edit::LAYER_ADD, n + 1);
                ur.undostack.push_back(e);
                ur.redostack.clear();
            }
            ImGui::SameLine();

            // "deletes" the layer
            if (ImGui::Button("X"))
            {
                Edit e(Edit::LAYER_REMOVE, n);
                e.removed_layer = std::make_shared<Layer>(layer);
                ur.undostack.push_back(e);
                ur.redostack.clear();

                if (canvas.current_layer_index == n)
                    canvas.current_layer_index = -1;
                else if (canvas.current_layer_index > n)
                    canvas.current_layer_index--;
                layer.is_deleted = true; // maybe add a button that deletes all the layers that are marked as deleted, so user can free the memory
            }
            ImGui::SameLine();
            
            // shows the current layer and allows it to be selected
            if (ImGui::Selectable(layer.name, canvas.current_layer_index == n))
                canvas.current_layer_index = n;

            // drag and drop functionality
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
            {
                ImGui::SetDragDropPayload("LAYER_REORDER", &n, sizeof(i32));
                ImGui::Text("Move %s", layer.name);
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("LAYER_REORDER"))
                {
                    IM_ASSERT(payload->DataSize == sizeof(i32));
                    i32 payload_n = *(const i32*)payload->Data;
                    // inserting the dragged layer to the target location and shift others accordingly
                    if (payload_n != n)
                    {
                        Layer moved_layer = std::move(canvas.layers[payload_n]);
                        canvas.layers.erase(canvas.layers.begin() + payload_n);
                        canvas.layers.insert(canvas.layers.begin() + n, std::move(moved_layer));

                        // update current layer index after the move
                        if (canvas.current_layer_index == payload_n)
                            canvas.current_layer_index = n;
                        else if (canvas.current_layer_index > payload_n && canvas.current_layer_index <= n)
                            canvas.current_layer_index--;
                        else if (canvas.current_layer_index < payload_n && canvas.current_layer_index >= n)
                            canvas.current_layer_index++;
                    }
                }
                ImGui::EndDragDropTarget();
            }

            ImGui::SameLine();
            ImGui::Text("Pos: (%.0f, %.0f)", layer.pos.x, layer.pos.y);
            ImGui::PopID();
        }

        canvas.remove_deleted_layers();

        // creates a new empty layer
        if (canvas.initialized)
        {
            ImGui::Spacing(); ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing(); ImGui::Spacing(); ImGui::Spacing();
            if (ImGui::Button("Create new layer"))
            {
                Raster* raster = new Raster();
                raster->create_blank(canvas.size, sf::Color(255, 255, 255, 0));

                canvas.layers.emplace_back(
                    canvas.default_layer_name(),
                    (canvas.window_size - canvas.size) / 2,
                    raster, Layer::RASTER, Layer::NORMAL
                );

                Edit e(Edit::LAYER_ADD, canvas.layers.size() - 1);
                ur.undostack.push_back(e);
                ur.redostack.clear();
            }
            ui_highlighter.registerUIElement("Create new layer button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
        }
        ImGui::End();

        //................................................. THE COLOR PANEL .................................................
        ImGui::Begin("Color", nullptr, g_layout_manager.getWindowFlags());
        
        float* color_to_edit = (float*)&(canvas.current_color == 0 ? canvas.primary_color : canvas.secondary_color);
        
        // Modern color picker with vertical hue bar (not wheel)
        ImGui::ColorPicker3("##picker", color_to_edit,
            ImGuiColorEditFlags_NoSidePreview |
            ImGuiColorEditFlags_NoSmallPreview |
            ImGuiColorEditFlags_NoAlpha |
            ImGuiColorEditFlags_PickerHueBar);  // Vertical hue bar
        
        ImGui::Separator();
        
        // RGB/HSV inputs
        ImGui::ColorEdit3("RGB", color_to_edit, ImGuiColorEditFlags_DisplayRGB);
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Primary/Secondary color selection with larger buttons
        ImGui::TextColored(UIColors::TextDim(), "COLOR SELECTION");
        ImGui::Spacing();
        
        bool is_primary = (canvas.current_color == 0);
        
        // Primary color button
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, is_primary ? 2.0f : 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, UIColors::Primary());
        if (ImGui::ColorButton("##Primary", canvas.primary_color, ImGuiColorEditFlags_NoAlpha, ImVec2(60, 40))) {
            canvas.current_color = 0;
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::SameLine();
        ImGui::Text("Primary%s", is_primary ? " *" : "");
        
        // Secondary color button
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, !is_primary ? 2.0f : 0.0f);
        ImGui::PushStyleColor(ImGuiCol_Border, UIColors::Primary());
        if (ImGui::ColorButton("##Secondary", canvas.secondary_color, ImGuiColorEditFlags_NoAlpha, ImVec2(60, 40))) {
            canvas.current_color = 1;
        }
        ImGui::PopStyleColor();
        ImGui::PopStyleVar();
        ImGui::SameLine();
        ImGui::Text("Secondary%s", !is_primary ? " *" : "");
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        
        // Quick color swatches
        ImGui::TextColored(UIColors::TextDim(), "QUICK COLORS");
        ImGui::Spacing();
        
        static const ImVec4 quick_colors[] = {
            ImVec4(1.0f, 1.0f, 1.0f, 1.0f),  // White
            ImVec4(0.0f, 0.0f, 0.0f, 1.0f),  // Black
            ImVec4(1.0f, 0.0f, 0.0f, 1.0f),  // Red
            ImVec4(1.0f, 0.5f, 0.0f, 1.0f),  // Orange
            ImVec4(1.0f, 1.0f, 0.0f, 1.0f),  // Yellow
            ImVec4(0.0f, 1.0f, 0.0f, 1.0f),  // Green
            ImVec4(0.0f, 0.7f, 1.0f, 1.0f),  // Cyan
            ImVec4(0.2f, 0.4f, 1.0f, 1.0f),  // Blue
            ImVec4(0.6f, 0.2f, 1.0f, 1.0f),  // Purple
            ImVec4(1.0f, 0.4f, 0.7f, 1.0f),  // Pink
        };
        
        ImVec2 swatch_size(32.0f, 32.0f);
        for (int i = 0; i < 10; i++) {
            if (i > 0 && i % 5 != 0) ImGui::SameLine();
            ImGui::PushID(i);
            if (ImGui::ColorButton("##swatch", quick_colors[i], ImGuiColorEditFlags_NoAlpha, swatch_size)) {
                if (canvas.current_color == 0)
                    canvas.primary_color = quick_colors[i];
                else
                    canvas.secondary_color = quick_colors[i];
            }
            ImGui::PopID();
        }
        
        ImGui::End();

        //................................................. THE TOOLS PANEL .................................................
        ImGui::Begin("Tools", nullptr, g_layout_manager.getWindowFlags());
        // show each tool and allow them to be selected
        i32 i = 0;
        for (auto& [tool_name, tool_texture] : assets.texture_map)
        {
            bool is_selected = i == tools.current_tool;
            // push highlighted style if this is the current tool (blue accent)
            if (is_selected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, UIColors::Primary());
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, UIColors::PrimaryHover());
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, UIColors::PrimaryActive());
            }

            // clicked tool is activated, rest are deactivated
            if (ImGui::ImageButton(tool_name.c_str(), tool_texture.getNativeHandle(), ImVec2(UIConfig::ICON_SIZE_TOOL, UIConfig::ICON_SIZE_TOOL))) {
                tools.current_tool = i;
                ui_highlighter.onElementClicked(tool_name);
            }
            ui_highlighter.registerUIElement(tool_name, ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            // only pop if we pushed
            if (is_selected)
                ImGui::PopStyleColor(3);

            if (ImGui::IsItemHovered(ImGuiHoveredFlags_Stationary))
                ImGui::SetTooltip(tools_tooltips[i]);
            ImGui::SameLine();
            i++;
        }

        //................................................. THE TOOLS SETTINGS AREA .................................................
        ImGui::Spacing();
        if (tools.current_tool == Tools::BRUSH)
        {
            ImGui::SeparatorText("Brush Settings");
            ImGui::DragInt("Size", &tools.brush_size, 1, 1, 2000, "%dpx");
            ImGui::SliderFloat("Hardness", &tools.brush_hardness, 0.f, 100.f, "%.1f%%");
            ImGui::SliderFloat("Opacity", &tools.brush_opacity, 0.f, 100.f, "%.1f%%");
            // ImGui::Checkbox("Anti-aliasing", &tools.brush_anti_aliasing);
        }
        else if (tools.current_tool == Tools::ERASER)
        {
            ImGui::SeparatorText("Eraser Settings");
            ImGui::DragInt("Size", &tools.eraser_size, 1, 1, 2000, "%dpx");
            ImGui::SliderFloat("Hardness", &tools.eraser_hardness, 0.f, 100.f, "%.1f%%");
            ImGui::SliderFloat("Opacity", &tools.eraser_opacity, 0.f, 100.f, "%.1f%%");
            // ImGui::Checkbox("Anti-aliasing", &tools.eraser_anti_aliasing);
        }
        else if (tools.current_tool == Tools::FILL)
        {
            ImGui::SeparatorText("Fill Settings");
            ImGui::SliderFloat("Tolerance", &tools.fill_tolerance, 0.f, 255.f, "%.0f");
            ImGui::SliderFloat("Opacity", &tools.fill_opacity, 0.f, 100.f, "%.1f%%");
            ImGui::Checkbox("Contiguous", &tools.fill_contiguous);
            // ImGui::Checkbox("Anti-aliasing", &tools.fill_anti_aliasing);
        }
        ImGui::End();

        //................................................. USING THE CURRENT TOOL .................................................
        if (canvas.initialized && vars.canvas_focused) tools.use_current_tool[tools.current_tool](tools);

        //................................................. OTHER GUI STUFF LIKE MENU BAR AND DIALOGS .................................................
        if (vars.show_menu_bar) gui::menu_bar(vars, filters, ur);
        if (vars.show_open_img_dialog) gui::open_dialog(vars);
        if (vars.show_saveas_img_dialog) gui::saveas_dialog(vars);
        
        //................................................. AI ACTION DISPATCHER .................................................
        // Check if the AI navigation wants to execute an action
        if (ui_highlighter.shouldExecuteAction()) {
            std::string element_id = ui_highlighter.getExecuteElementId();
            
            // Menu opening actions
            if (element_id == "File menu") {
                // File menu will be opened by menu_bar on next frame
            } else if (element_id == "Transform menu") {
                // Transform menu will be opened by menu_bar on next frame
            } else if (element_id == "Adjust menu") {
                // Adjust menu will be opened by menu_bar on next frame
            } else if (element_id == "Filters menu") {
                // Filters menu will be opened by menu_bar on next frame
            }
            // File operations
            else if (element_id == "New button") {
                vars.show_new_img_dialog = true;
            } else if (element_id == "Open button") {
                vars.show_open_img_dialog = true;
            } else if (element_id == "Export button") {
                vars.show_saveas_img_dialog = true;
            }
            // Edit operations
            else if (element_id == "Undo button") {
                ur.undo();
            } else if (element_id == "Redo button") {
                ur.redo();
            }
            // Tool selection
            else if (element_id == "No Tool") {
                tools.current_tool = Tools::NO;
            } else if (element_id == "Move Tool") {
                tools.current_tool = Tools::MOVE;
            } else if (element_id == "Brush Tool") {
                tools.current_tool = Tools::BRUSH;
            } else if (element_id == "Eraser Tool") {
                tools.current_tool = Tools::ERASER;
            } else if (element_id == "Fill Tool") {
                tools.current_tool = Tools::FILL;
            } else if (element_id == "Zoom Tool") {
                tools.current_tool = Tools::ZOOM;
            }
            // Layer operations
            else if (element_id == "Create new layer button" && canvas.initialized) {
                Raster* raster = new Raster();
                raster->create_blank(canvas.size, sf::Color(255, 255, 255, 0));
                canvas.layers.emplace_back(
                    canvas.default_layer_name(),
                    (canvas.window_size - canvas.size) / 2,
                    raster, Layer::RASTER, Layer::NORMAL
                );
                Edit e(Edit::LAYER_ADD, canvas.layers.size() - 1);
                ur.undostack.push_back(e);
                ur.redostack.clear();
            }
            // Filter operations (simple ones without popups)
            else if (element_id == "Gray Scale button") {
                filters.apply_filter("GrayScale");
            } else if (element_id == "Invert button") {
                filters.apply_filter("Invert");
            } else if (element_id == "Sepia button") {
                filters.apply_filter("Sepia");
            } else if (element_id == "Edge Detection button") {
                filters.apply_filter("EdgeDetection");
            } else if (element_id == "Sharpen button") {
                filters.apply_filter("Sharpen");
            } else if (element_id == "Flip Horizontal button") {
                filters.apply_filter("FlipX");
            } else if (element_id == "Flip Vertical button") {
                filters.apply_filter("FlipY");
            }
            // Note: Filters with sliders (Brightness, Blur, etc.) would need popup handling
            // which requires more complex state management
        }

        //................................................. NEW IMAGE .................................................
        if (vars.show_new_img_dialog)
        {
            ImGui::OpenPopup("New Image##modal");
            ImVec2 center = ImGui::GetMainViewport()->GetCenter();
            ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

            static i32 width = 1280, height = 720;
            static const char* bg[] = {"Black", "White", "Transparent", "Custom"};
            static i32 current_item = 0;
            static float bg_color[4] = { 0, 0, 0, 1 };

            if (ImGui::BeginPopupModal("New Image##modal", &vars.show_new_img_dialog))
            {
                ImGui::InputInt("Width", &width);
                ImGui::InputInt("Height", &height);
                ImGui::Combo("Background", &current_item, bg, IM_ARRAYSIZE(bg));
                if (current_item == 0)
                {
                    bg_color[0] = bg_color[1] = bg_color[2] = 0;
                    bg_color[3] = 1;
                }
                else if (current_item == 1)
                {
                    bg_color[0] = bg_color[1] = bg_color[2] = bg_color[3] = 1;
                }
                else if (current_item == 2)
                {
                    bg_color[0] = bg_color[1] = bg_color[2] = bg_color[3] = 0;
                }
                else if (current_item == 3)
                {
                    ImGui::ColorEdit4("Custom Color", bg_color);
                }
                
                ImGui::Separator();
                if (ImGui::Button("Create"))
                {
                    canvas.size = vec2(width, height);
                    canvas.view_center = canvas.window_size / 2;
                    float image_scale = std::max(
                        canvas.size.x / canvas.window_size.x,
                        canvas.size.y / canvas.window_size.y
                    );
                    canvas.relative_zoom_factor = canvas.zoom_factor = vars.canvas_zoom_factor = 2.3 * image_scale;
                    canvas.navigate();
                    canvas.initialized = true;

                    canvas.layers.clear();
                    Raster* raster = new Raster();
                    raster->create_blank(canvas.size, sf::Color(
                        (ui8)(bg_color[0] * 255),
                        (ui8)(bg_color[1] * 255),
                        (ui8)(bg_color[2] * 255),
                        (ui8)(bg_color[3] * 255)
                    ));

                    canvas.layers.emplace_back(
                        canvas.default_layer_name(),
                        (canvas.window_size - canvas.size) / 2,
                        raster, Layer::RASTER, Layer::NORMAL
                    );
                    canvas.current_layer_index = 0;

                    vars.show_new_img_dialog = false;
                    ImGui::CloseCurrentPopup();
                }         
                ImGui::EndPopup();
            }
        }

        //................................................. OPEN IMAGE .................................................
        // in a separate canvas window?
        // (maybe ask the user a question to see whether they want to open the image in a new canvas or in the same)
        // (if in the same canvas then the following behaviour is correct, i think)
        // (otherwise, maybe create a new canvas variable and push it to std::vector<Canvas>??)
        if (vars.open_image)
        {
            Raster* raster = new Raster();
            if (raster->loadfromfile(vars.open_path, &canvas))
            {
                vec2 img_size = raster->texture.getSize();

                // the first opened image defines the canvas size and other variables
                if (canvas.initialized == false)
                {
                    canvas.size = img_size;
                    canvas.view_center = canvas.window_size / 2;
                    float image_scale = std::max(
                        canvas.size.x / canvas.window_size.x,
                        canvas.size.y / canvas.window_size.y
                    );
                    canvas.relative_zoom_factor = canvas.zoom_factor = vars.canvas_zoom_factor = 2.3 * image_scale;
                    canvas.navigate();
                    canvas.initialized = true;
                }

                vec2 img_pos = (canvas.window_size - img_size) / 2;

                // add to canvas' layers
                canvas.layers.emplace_back(
                    canvas.default_layer_name(),
                    img_pos,
                    raster, Layer::RASTER, Layer::NORMAL
                );
                canvas.current_layer_index = canvas.layers.size() - 1;
            }
            else
            {
                delete raster;
                std::cerr << "Failed to open image: " << vars.open_path << std::endl;
            }
            vars.open_image = false;
        }

        //................................................. SAVE IMAGE .................................................
        if (vars.save_image)
        {
            if (canvas.layers.empty())
            {
                std::cout << "there is no image to save...\n";
            }
            else
            {
                canvas.texture.getTexture().copyToImage().saveToFile(vars.save_path);            
                std::cout << "Saving image to: " << vars.save_path << std::endl;
            }
            vars.save_image = false;
        }

        //................................................. CANVAS NAVIGATION .................................................
        // update the view to "navigate the canvas"
        // but prevent navigating if mouse not over the canvas window or if a modal window is there (see Action.cpp where this prevention occurs)        
        // prevent navigation when canvas is not initialized as well
        vars.canvas_focused = vars.canvas_focused && (!vars.show_open_img_dialog && !vars.show_saveas_img_dialog);
        if (canvas.initialized || true)
        {
            if (vars.navigate_canvas_right_now)
            {
                canvas.zoom_factor = vars.canvas_zoom_factor;
                canvas.view_center += vars.pan_delta;
                vars.pan_delta = vec2(0, 0);
                canvas.navigate();
                vars.navigate_canvas_right_now = false;
            }

            // if asked to reset the canvas navigation
            if (vars.navigate_canvas_reset)
            {
                vars.canvas_zoom_factor = canvas.zoom_factor = canvas.relative_zoom_factor;
                canvas.view_center = canvas.window_size / 2;
                canvas.navigate();
                vars.navigate_canvas_reset = false;
            }
        }
        else
        {
            vars.canvas_zoom_factor = 1;
            vars.pan_delta *= canvas.initialized;
        }

        //................................................. DEBUG WINDOW .................................................
#if DEBUG == 1 // this is for debugging only
        // display the current pressed keys as they are pressed or released
        ImGui::Begin("Debug Information", nullptr, g_layout_manager.getWindowFlags());

        std::string keys_pressed = "Keys pressed: ";
        for (auto key : currently_pressed_keys)
        {
            keys_pressed += std::to_string(key) + std::string(" ");
        }
        ImGui::Text(keys_pressed.c_str());
        // display window, canvas, view, etc's info
        ImGui::Text("Window size: (%i, %i)", window.getSize().x , window.getSize().y);
        ImGui::Separator();
        ImGui::Text("Canvas window size: (%.1f, %.1f)", canvas.window_size.x, canvas.window_size.y);
        ImGui::Text("Canvas texture size: (%i, %i)", canvas.window_texture.getSize().x , canvas.window_texture.getSize().y);
        ImGui::Text("Canvas size: (%.1f, %.1f)", canvas.size.x, canvas.size.y);
        ImGui::Text("Canvas start position: (%.1f, %.1f)", canvas.start_pos.x, canvas.start_pos.y);
        ImGui::Text("Canvas zoom factor: %f", canvas.zoom_factor);
        ImGui::Text("Canvas relative zoom factor: %f", canvas.relative_zoom_factor);
        ImGui::Text("Canvas view center: (%.1f, %.1f)", canvas.view_center.x, canvas.view_center.y);
        ImGui::Text("Pan delta: (%.1f, %.1f)", vars.pan_delta.x, vars.pan_delta.y);
        ImGui::Text("Canvas focused: %i", vars.canvas_focused);
        ImGui::Separator();
        ImGui::Text("Mouse position: (%.1f, %.1f)", vars.mouse_pos.x, vars.mouse_pos.y);
        ImGui::Text("Mouse canvas world position: (%.1f, %.1f)", canvas.mouse_p.x, canvas.mouse_p.y);
        ImGui::Text("Mouse left held: %i", vars.mouse_l_held);
        ImGui::Text("Mouse right held: %i", vars.mouse_r_held);
        ImGui::Separator();
        ImGui::Text("Current layer index: %i", canvas.current_layer_index);
        Layer* l = canvas.current_layer();
        ImGui::Text("Current layer name: %s", l ? l->name : "*NULL*");
        ImGui::End();
#endif

        //................................................. END OF FRAME .................................................
        // Render AI System
        if (ai_assistant) ai_assistant->render();
        ui_highlighter.update(); // Draws overlay on top

        ImGui::SFML::Render(window);
        window.display();
        frames++;
    }

    //................................................. DE-INITIALIZATION .................................................    
    // Save layout before exit
    g_layout_manager.saveLayout("layout.json");
    
    // Cleanup AI
    if (ai_assistant) {
        delete ai_assistant;
        ai_assistant = nullptr;
    }
    ai_navigator.cleanup();
    
    ImGui::SFML::Shutdown();
    return 0;
}