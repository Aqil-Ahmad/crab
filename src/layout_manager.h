#pragma once

/**
 * Layout Manager - Window layout persistence and edit mode control
 * 
 * Provides:
 * - Locked window layout by default (non-draggable/resizable)
 * - Edit mode toggle to allow layout customization
 * - Save/load layout to JSON file for persistence across sessions
 */

#include <string>
#include <map>
#include <imgui.h>

/**
 * Stores position and size for a single window
 */
struct WindowLayout {
    ImVec2 position;
    ImVec2 size;
    bool is_open;
    
    WindowLayout() : position(100, 100), size(300, 400), is_open(true) {}
    WindowLayout(ImVec2 pos, ImVec2 sz) : position(pos), size(sz), is_open(true) {}
};

/**
 * Manages window layouts with optional editing and persistence
 */
class LayoutManager {
public:
    LayoutManager();
    
    // ==========================================================================
    // Mode Control
    // ==========================================================================
    
    /**
     * Enable or disable layout editing mode
     * When disabled: windows are locked (no move/resize/collapse)
     * When enabled: windows can be freely moved and resized
     */
    void setEditMode(bool enable);
    bool isEditMode() const { return m_edit_mode; }
    
    // ==========================================================================
    // Window Management
    // ==========================================================================
    
    /**
     * Register a window with default position and size
     * Only adds if window doesn't already exist in layout
     */
    void registerWindow(const std::string& name, ImVec2 default_pos, ImVec2 default_size);
    
    /**
     * Get window flags based on current mode
     * In locked mode: adds NoMove, NoResize, NoCollapse flags
     */
    ImGuiWindowFlags getWindowFlags() const;
    
    /**
     * Apply saved position/size before ImGui::Begin
     */
    void applyWindowLayout(const std::string& name);
    
    /**
     * Save current position/size after window is rendered
     */
    void saveWindowState(const std::string& name);
    
    // ==========================================================================
    // Persistence
    // ==========================================================================
    
    /**
     * Save all window layouts to a JSON file
     */
    bool saveLayout(const std::string& filepath = "layout.json");
    
    /**
     * Load window layouts from a JSON file
     */
    bool loadLayout(const std::string& filepath = "layout.json");
    
    /**
     * Reset all windows to their registered defaults
     */
    void resetToDefaults();

private:
    std::map<std::string, WindowLayout> m_layouts;
    std::map<std::string, WindowLayout> m_defaults;  // Original defaults for reset
    bool m_edit_mode;
    std::string m_layout_file;
};

// Global instance
extern LayoutManager g_layout_manager;
