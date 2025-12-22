/**
 * Layout Manager Implementation
 * 
 * Handles window layout persistence and edit mode control.
 */

#include "layout_manager.h"
#include <json.hpp>
#include <fstream>
#include <iostream>

using json = nlohmann::json;

// Global instance
LayoutManager g_layout_manager;

// ============================================================================
// Constructor
// ============================================================================

LayoutManager::LayoutManager()
    : m_edit_mode(false)
    , m_layout_file("layout.json")
{
}

// ============================================================================
// Mode Control
// ============================================================================

void LayoutManager::setEditMode(bool enable)
{
    m_edit_mode = enable;
    
    if (enable) {
        std::cout << "[LayoutManager] Edit mode ENABLED - windows can be moved/resized\n";
    } else {
        std::cout << "[LayoutManager] Edit mode DISABLED - windows locked\n";
        // Auto-save when exiting edit mode
        saveLayout();
    }
}

// ============================================================================
// Window Management
// ============================================================================

void LayoutManager::registerWindow(const std::string& name, ImVec2 default_pos, ImVec2 default_size)
{
    // Store defaults for reset functionality
    WindowLayout default_layout(default_pos, default_size);
    m_defaults[name] = default_layout;
    
    // Only add to layouts if not already present (preserves loaded values)
    if (m_layouts.find(name) == m_layouts.end()) {
        m_layouts[name] = default_layout;
    }
}

ImGuiWindowFlags LayoutManager::getWindowFlags() const
{
    ImGuiWindowFlags flags = 0;
    
    if (!m_edit_mode) {
        // Locked mode: prevent all window manipulation
        flags |= ImGuiWindowFlags_NoMove;
        flags |= ImGuiWindowFlags_NoResize;
        flags |= ImGuiWindowFlags_NoCollapse;
    }
    
    return flags;
}

void LayoutManager::applyWindowLayout(const std::string& name)
{
    auto it = m_layouts.find(name);
    if (it != m_layouts.end()) {
        const WindowLayout& layout = it->second;
        ImGui::SetNextWindowPos(layout.position, ImGuiCond_Once);
        ImGui::SetNextWindowSize(layout.size, ImGuiCond_Once);
    }
}

void LayoutManager::saveWindowState(const std::string& name)
{
    // Only save state when in edit mode
    if (m_edit_mode) {
        auto it = m_layouts.find(name);
        if (it != m_layouts.end()) {
            it->second.position = ImGui::GetWindowPos();
            it->second.size = ImGui::GetWindowSize();
        }
    }
}

// ============================================================================
// Persistence
// ============================================================================

bool LayoutManager::saveLayout(const std::string& filepath)
{
    try {
        json j;
        
        for (const auto& [name, layout] : m_layouts) {
            j[name] = {
                {"pos_x", layout.position.x},
                {"pos_y", layout.position.y},
                {"size_x", layout.size.x},
                {"size_y", layout.size.y},
                {"is_open", layout.is_open}
            };
        }
        
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[LayoutManager] Failed to save layout to " << filepath << "\n";
            return false;
        }
        
        file << j.dump(2);  // Pretty print with 2-space indent
        file.close();
        
        std::cout << "[LayoutManager] Layout saved to " << filepath << "\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[LayoutManager] Error saving layout: " << e.what() << "\n";
        return false;
    }
}

bool LayoutManager::loadLayout(const std::string& filepath)
{
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cout << "[LayoutManager] No saved layout found, using defaults\n";
            return false;
        }
        
        json j;
        file >> j;
        file.close();
        
        for (auto& [name, data] : j.items()) {
            WindowLayout layout;
            layout.position.x = data.value("pos_x", 100.0f);
            layout.position.y = data.value("pos_y", 100.0f);
            layout.size.x = data.value("size_x", 300.0f);
            layout.size.y = data.value("size_y", 400.0f);
            layout.is_open = data.value("is_open", true);
            m_layouts[name] = layout;
        }
        
        std::cout << "[LayoutManager] Layout loaded from " << filepath << "\n";
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[LayoutManager] Error loading layout: " << e.what() << "\n";
        return false;
    }
}

void LayoutManager::resetToDefaults()
{
    m_layouts = m_defaults;
    std::cout << "[LayoutManager] Layout reset to defaults\n";
}
