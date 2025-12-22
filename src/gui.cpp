#include <imgui.h>
#include <ImGuiFileDialog.h>
#include "gui.h"
#include "Undo_redo.h"
#include "ui_highlighter.h"
#include "layout_manager.h"

extern UIHighlighter ui_highlighter;

void show_filter_popup(const char* popup_name, const char* slider_label, int& filter_strength, int min_value, int max_value, Filters& filters, const std::string& filter_name, bool& show_popup)
{
    if (show_popup)
    {
        ImGui::OpenPopup(popup_name);
    }
    if (ImGui::BeginPopupModal(popup_name, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        ImGui::SliderInt(slider_label, &filter_strength, min_value, max_value);
        // Register slider for AI highlighting
        ui_highlighter.registerUIElement(std::string(filter_name) + " slider", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
        
        if (ImGui::Button("Apply"))
        {
            ui_highlighter.onElementClicked("Apply button");
            filters.apply_filter(filter_name);
            ImGui::CloseCurrentPopup();
            show_popup = false;
        }
        ui_highlighter.registerUIElement("Apply button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
            show_popup = false;
        }
        ui_highlighter.registerUIElement("Cancel button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
        
        ImGui::EndPopup();
    }
}

void show_angle_popup(const char* popup_name, const char* slider_label, int& angle, Filters& filters, const std::string& filter_name, bool& show_popup)
{
    if (show_popup)
    {
        ImGui::OpenPopup(popup_name);
    }
    if (ImGui::BeginPopupModal(popup_name, NULL, ImGuiWindowFlags_AlwaysAutoResize))
    {
        static float angle_rad = 0;
        angle_rad = angle * 3.14159265358979323846 / 180;
        ImGui::SliderAngle(slider_label, &angle_rad, -360, 360);
        angle = angle_rad * 180 / 3.14159265358979323846;
        // Register slider for AI highlighting
        ui_highlighter.registerUIElement("Rotate slider", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
        
        if (ImGui::Button("Apply"))
        {
            ui_highlighter.onElementClicked("Apply button");
            filters.apply_filter(filter_name);
            ImGui::CloseCurrentPopup();
            show_popup = false;
        }
        ui_highlighter.registerUIElement("Apply button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel"))
        {
            ImGui::CloseCurrentPopup();
            show_popup = false;
        }
        ui_highlighter.registerUIElement("Cancel button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
        
        ImGui::EndPopup();
    }
}

//..................................................................................................
void gui::menu_bar(Variables& vars, Filters& filters, Undo_redo& ur)
{
    static bool show_rotate_popup = false;
    static bool show_brightness_popup = false;
    static bool show_contrast_popup = false;
    static bool show_box_blur_popup = false;
    static bool show_gaussian_blur_popup = false;
    static bool show_pixelate_popup = false;

    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            // Menu was clicked/opened - notify highlighter
            ui_highlighter.onElementClicked("File menu");
            ui_highlighter.registerUIElement("File menu", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
            if (ImGui::MenuItem("New", "Ctrl+N"))
            {
                ui_highlighter.onElementClicked("New button");
                vars.show_new_img_dialog = true;
                vars.show_open_img_dialog = false;
                vars.show_saveas_img_dialog = false;
            }
            ui_highlighter.registerUIElement("New button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            if (ImGui::MenuItem("Open", "Ctrl+O"))
            {
                ui_highlighter.onElementClicked("Open button");
                vars.show_new_img_dialog = false;
                vars.show_open_img_dialog = true;
                vars.show_saveas_img_dialog = false;
            }
            ui_highlighter.registerUIElement("Open button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            // if (ImGui::MenuItem("Save", "Ctrl+S")); // add this when a custom save file format (e.g .ps for Photoshop) has been implemented
            if (ImGui::MenuItem("Export..", "Ctrl+Shift+S"))
            {
                ui_highlighter.onElementClicked("Export button");
                vars.show_new_img_dialog = false;
                vars.show_open_img_dialog = false;
                vars.show_saveas_img_dialog = true;
            }
            ui_highlighter.registerUIElement("Export button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Edit"))
        {
            // Menu was clicked/opened - notify highlighter
            ui_highlighter.onElementClicked("Edit menu");
            ui_highlighter.registerUIElement("Edit menu", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
            if (ImGui::MenuItem("Undo", "CTRL+Z")) {
                ui_highlighter.onElementClicked("Undo button");
                ur.undo();
            }
            ui_highlighter.registerUIElement("Undo button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            if (ImGui::MenuItem("Redo", "CTRL+Y")) {
                ui_highlighter.onElementClicked("Redo button");
                ur.redo();
            }
            ui_highlighter.registerUIElement("Redo button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            ImGui::Separator();

            if (ImGui::MenuItem("Fill with primary color")) {
                ui_highlighter.onElementClicked("Fill primary button");
                Canvas* canvas = filters.canv;
                Layer* layer = canvas->current_layer();
                if (layer && layer->type == Layer::RASTER) {
                    Raster* raster = (Raster*)layer->graphic;
                    
                    // Create undo point
                    Edit e(Edit::FILL, canvas->current_layer_index);
                    e.tex = std::make_shared<sf::Texture>(raster->texture);
                    ur.undostack.push_back(e);
                    ur.redostack.clear();
                    
                    // Perform fill
                    sf::Color color((ui32)(canvas->primary_color.x * 255),
                                   (ui32)(canvas->primary_color.y * 255),
                                   (ui32)(canvas->primary_color.z * 255),
                                   (ui32)(canvas->primary_color.w * 255));
                    
                    sf::Image img;
                    img.create(raster->texture.getSize().x, raster->texture.getSize().y, color);
                    raster->texture.loadFromImage(img);
                }
            }
            ui_highlighter.registerUIElement("Fill primary button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("View"))
        {
            // Menu was clicked/opened - notify highlighter
            ui_highlighter.onElementClicked("View menu");
            ui_highlighter.registerUIElement("View menu", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
            if (ImGui::MenuItem("Menu bar", "Alt", &vars.show_menu_bar));
            ImGui::Separator();
            if (ImGui::MenuItem("Use Transparent Checker Shader", NULL, &vars.use_checker_shader));
            ImGui::EndMenu();
        }
        // ============ TRANSFORM MENU (Flip, Rotate) ============
        if (ImGui::BeginMenu("Transform"))
        {
            ui_highlighter.onElementClicked("Transform menu");
            ui_highlighter.registerUIElement("Transform menu", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
            
            if (ImGui::MenuItem("Flip Horizontal"))
            {
                ui_highlighter.onElementClicked("Flip Horizontal button");
                filters.apply_filter("FlipX");
            }
            ui_highlighter.registerUIElement("Flip Horizontal button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            if (ImGui::MenuItem("Flip Vertical"))
            {
                ui_highlighter.onElementClicked("Flip Vertical button");
                filters.apply_filter("FlipY");
            }
            ui_highlighter.registerUIElement("Flip Vertical button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            ImGui::Separator();

            if (ImGui::MenuItem("Rotate Custom Angle"))
            {
                ui_highlighter.onElementClicked("Rotate button");
                show_rotate_popup = true;
            }
            ui_highlighter.registerUIElement("Rotate button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
            
            if (ImGui::MenuItem("Rotate 90° CW"))
            {
                filters.apply_filter("RotateCW");
            }
            ui_highlighter.registerUIElement("Rotate 90 CW button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
            
            if (ImGui::MenuItem("Rotate 90° CCW"))
            {
                filters.apply_filter("RotateCCW");
            }
            ui_highlighter.registerUIElement("Rotate 90 CCW button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            ImGui::EndMenu();
        }
        
        // ============ ADJUST MENU (Brightness, Contrast, Color) ============
        if (ImGui::BeginMenu("Adjust"))
        {
            ui_highlighter.onElementClicked("Adjust menu");
            ui_highlighter.registerUIElement("Adjust menu", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
            
            if (ImGui::MenuItem("Brightness"))
            {
                ui_highlighter.onElementClicked("Brightness button");
                show_brightness_popup = true;
            }
            ui_highlighter.registerUIElement("Brightness button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            if (ImGui::MenuItem("Contrast"))
            {
                ui_highlighter.onElementClicked("Contrast button");
                show_contrast_popup = true;
            }
            ui_highlighter.registerUIElement("Contrast button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            ImGui::Separator();

            if (ImGui::MenuItem("Gray Scale"))
            {
                ui_highlighter.onElementClicked("Gray Scale button");
                filters.apply_filter("GrayScale");
            }
            ui_highlighter.registerUIElement("Gray Scale button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            if (ImGui::MenuItem("Invert Colors"))
            {
                ui_highlighter.onElementClicked("Invert button");
                filters.apply_filter("Invert");
            }
            ui_highlighter.registerUIElement("Invert button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            if (ImGui::MenuItem("Sepia Tone"))
            {
                ui_highlighter.onElementClicked("Sepia button");
                filters.apply_filter("Sepia");
            }
            ui_highlighter.registerUIElement("Sepia button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            ImGui::EndMenu();
        }
        
        // ============ FILTERS MENU (Blur, Sharpen, Effects) ============
        if (ImGui::BeginMenu("Filters"))
        {
            ui_highlighter.onElementClicked("Filters menu");
            ui_highlighter.registerUIElement("Filters menu", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());
            
            if (ImGui::MenuItem("Box Blur"))
            {
                ui_highlighter.onElementClicked("Box Blur button");
                show_box_blur_popup = true;
            }
            ui_highlighter.registerUIElement("Box Blur button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            if (ImGui::MenuItem("Gaussian Blur"))
            {
                ui_highlighter.onElementClicked("Gaussian Blur button");
                show_gaussian_blur_popup = true;
            }
            ui_highlighter.registerUIElement("Gaussian Blur button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            ImGui::Separator();

            if (ImGui::MenuItem("Pixelate"))
            {
                ui_highlighter.onElementClicked("Pixelate button");
                show_pixelate_popup = true;
            }
            ui_highlighter.registerUIElement("Pixelate button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            if (ImGui::MenuItem("Edge Detection"))
            {
                ui_highlighter.onElementClicked("Edge Detection button");
                filters.apply_filter("EdgeDetection");
            }
            ui_highlighter.registerUIElement("Edge Detection button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            if (ImGui::MenuItem("Sharpen"))
            {
                ui_highlighter.onElementClicked("Sharpen button");
                filters.apply_filter("Sharpen");
            }
            ui_highlighter.registerUIElement("Sharpen button", ImGui::GetItemRectMin(), ImGui::GetItemRectSize());

            ImGui::EndMenu();
        }
        
        // ============ SETTINGS MENU ============
        if (ImGui::BeginMenu("Settings"))
        {
            bool edit_mode = g_layout_manager.isEditMode();
            
            if (ImGui::Checkbox("Edit Layout Mode", &edit_mode)) {
                g_layout_manager.setEditMode(edit_mode);
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Enable to drag and resize windows.\nDisable to lock layout.");
            }
            
            ImGui::Separator();
            
            if (ImGui::MenuItem("Save Layout")) {
                g_layout_manager.saveLayout("layout.json");
            }
            
            if (ImGui::MenuItem("Load Layout")) {
                g_layout_manager.loadLayout("layout.json");
            }
            
            if (ImGui::MenuItem("Reset to Default Layout")) {
                g_layout_manager.resetToDefaults();
            }
            
            ImGui::EndMenu();
        }
        
        ImGui::EndMainMenuBar();
    }

    // the filters settings popups / dialogs
    show_angle_popup("Set Rotate Angle", "Rotate Angle", filters.rotate_angle, filters, "Rotate", show_rotate_popup);
    show_filter_popup("Set Brightness Strength", "Brightness Strength", filters.brightness_strength, -150, 150, filters, "Brightness", show_brightness_popup);
    show_filter_popup("Set Contrast Strength", "Contrast Strength", filters.contrast_strength, -100, 100, filters, "Contrast", show_contrast_popup);
    show_filter_popup("Set Box Blur Strength", "Box Blur Strength", filters.box_blur_strength, 1, 100, filters, "BoxBlur", show_box_blur_popup);
    show_filter_popup("Set Gaussian Blur Strength", "Gaussian Blur Strength", filters.gauss_blur_strength, 1, 100, filters, "GaussianBlur", show_gaussian_blur_popup);
    show_filter_popup("Set Pixelate Size", "Pixelate Size", filters.pixelate_size, 2, 100, filters, "Pixelate", show_pixelate_popup);
}

//..................................................................................................
void gui::open_dialog(Variables& vars)
{
    if (!vars.show_open_img_dialog) return;

    IGFD::FileDialogConfig config;
    config.path = "./test_imgs/"; // don't forget to change this back to something like "."
    config.flags = ImGuiFileDialogFlags_Modal; // prevent interacting with app if dialog is open
    config.fileName = "";

    // open the dialog box
    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Select an Image..", "Image Files{.png,.jpg,.jpeg,.bmp},.png,.jpg,.jpeg,.bmp", config);

    // display dialog box
    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse))
    {
        // if action is ok
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            // load and open the image
            vars.open_path = ImGuiFileDialog::Instance()->GetFilePathName();
            vars.open_image = true;
        }
        ImGuiFileDialog::Instance()->Close();
        vars.show_open_img_dialog = false;
    }
}

//..................................................................................................
void gui::saveas_dialog(Variables& vars)
{
    if (!vars.show_saveas_img_dialog) return;

    IGFD::FileDialogConfig config;
    config.path = ".";
    config.flags = ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_ConfirmOverwrite; // prevent interacting with app if dialog is open + notify overwrite of a file
    config.fileName = "untitled.png";

    ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Save Image As..", "Image Files{.png,.jpg,.jpeg,.bmp}", config);

    if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey", ImGuiWindowFlags_NoCollapse))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            // save the image
            vars.save_path = ImGuiFileDialog::Instance()->GetFilePathName();
            vars.save_image = true;
        }
        ImGuiFileDialog::Instance()->Close();
        vars.show_saveas_img_dialog = false;
    }
}