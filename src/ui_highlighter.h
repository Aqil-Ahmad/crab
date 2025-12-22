#pragma once

/**
 * UI Highlighter - Visual overlay system for AI-guided navigation
 * 
 * This module draws visual highlights on ImGui elements to guide users
 * through step-by-step navigation instructions from AINavigator.
 */

#include <imgui.h>
#include <vector>
#include <string>
#include <map>
#include <chrono>

#include "ai_navigator.h"

/**
 * Information about a registered UI element for highlighting
 */
struct UIElementInfo {
    ImVec2 position;        // Top-left position of the element
    ImVec2 size;            // Width and height of the element
    bool is_visible;        // Whether element is currently visible/rendered
    std::string element_id; // Unique identifier for the element
    
    UIElementInfo() 
        : position(0, 0), size(0, 0), is_visible(false) {}
    
    UIElementInfo(const ImVec2& pos, const ImVec2& sz, const std::string& id)
        : position(pos), size(sz), is_visible(true), element_id(id) {}
};

/**
 * Visual overlay system for highlighting UI elements during guided navigation
 * 
 * Usage:
 * 1. Call registerUIElement() for each UI element during rendering
 * 2. Call startNavigation() with steps from AINavigator
 * 3. Call update() at the end of each frame to draw overlays
 * 4. Call onElementClicked() when user interacts with elements
 * 
 * Example:
 *   UIHighlighter highlighter;
 *   
 *   // Start navigation with steps from AI
 *   highlighter.startNavigation(ai_steps);
 *   
 *   // In render loop:
 *   if (ImGui::Button("Apply Filter")) {
 *       highlighter.registerUIElement("Apply Filter button", 
 *                                      ImGui::GetItemRectMin(), 
 *                                      ImGui::GetItemRectSize());
 *       highlighter.onElementClicked("Apply Filter button");
 *   }
 *   
 *   // At the end of frame:
 *   highlighter.update();
 */
class UIHighlighter {
public:
    UIHighlighter();
    ~UIHighlighter() = default;
    
    // Prevent copying
    UIHighlighter(const UIHighlighter&) = delete;
    UIHighlighter& operator=(const UIHighlighter&) = delete;
    
    /**
     * Start a navigation sequence with the given steps
     * @param steps Navigation steps from AINavigator::processQuery()
     */
    void startNavigation(const std::vector<NavigationStep>& steps);
    
    /**
     * Update and render the highlight overlay
     * Call this at the end of each frame, after all ImGui rendering
     */
    void update();
    
    /**
     * Register a UI element for potential highlighting
     * Call this every frame for each element that might be highlighted
     * @param id Unique identifier matching NavigationStep::ui_element
     * @param pos Top-left position (use ImGui::GetItemRectMin())
     * @param size Element size (use ImGui::GetItemRectSize())
     */
    void registerUIElement(const std::string& id, ImVec2 pos, ImVec2 size);
    
    /**
     * Notify the highlighter that an element was clicked
     * If it matches the current step, advances to next step
     * @param element_id The ID of the clicked element
     */
    void onElementClicked(const std::string& element_id);
    
    /**
     * Stop the current navigation sequence
     */
    void stopNavigation();
    
    /**
     * Check if navigation is currently active
     */
    bool isActive() const { return m_is_active; }
    
    /**
     * Get the current step index
     */
    size_t getCurrentStepIndex() const { return m_current_step_index; }
    
    /**
     * Get total number of steps
     */
    size_t getTotalSteps() const { return m_current_steps.size(); }
    
    /**
     * Get the current step (or nullptr if no active navigation)
     */
    const NavigationStep* getCurrentStep() const;
    
    /**
     * Clear all registered UI elements
     * Call at the start of each frame before registering elements
     */
    void clearRegistry();
    
    /**
     * Advance to the next navigation step
     * Called when user acknowledges current step
     * @param executeAction If true, sets flag to execute the action
     */
    void advanceToNextStep(bool executeAction = false);
    
    /**
     * Check if an action should be executed this frame
     * Resets the flag after checking
     */
    bool shouldExecuteAction();
    
    /**
     * Get the UI element ID that should be executed
     */
    std::string getExecuteElementId() const { return m_execute_element_id; }

private:
    // Current navigation state
    std::vector<NavigationStep> m_current_steps;
    size_t m_current_step_index;
    bool m_is_active;
    
    // Action execution state
    bool m_should_execute_action;
    std::string m_execute_element_id;
    
    // UI element registry (refreshed each frame)
    std::map<std::string, UIElementInfo> m_ui_registry;
    
    // Timing for animations and auto-advance
    std::chrono::steady_clock::time_point m_step_start_time;
    std::chrono::steady_clock::time_point m_animation_start_time;
    
    // Visual settings
    static constexpr float OVERLAY_ALPHA = 180.0f / 255.0f;
    static constexpr float BORDER_THICKNESS = 3.0f;
    static constexpr float PULSE_SPEED = 4.0f;
    static constexpr float PULSE_AMPLITUDE = 2.0f;
    static constexpr float INSTRUCTION_PADDING = 10.0f;
    static constexpr float INSTRUCTION_OFFSET_Y = 40.0f;
    static constexpr float CORNER_RADIUS = 5.0f;
    
    // Colors
    ImU32 getOverlayColor() const { return IM_COL32(0, 0, 0, 180); }
    ImU32 getHighlightColor() const { return IM_COL32(255, 215, 0, 255); } // Gold
    ImU32 getInstructionBgColor() const { return IM_COL32(30, 30, 30, 240); }
    ImU32 getInstructionTextColor() const { return IM_COL32(255, 255, 255, 255); }
    ImU32 getStepIndicatorColor() const { return IM_COL32(100, 200, 255, 255); } // Light blue
    
    // Helper methods
    void drawOverlay();
    void drawElementHighlight(const UIElementInfo& element, float pulse);
    void drawInstructionBox(const NavigationStep& step, const ImVec2& anchor_pos);
    void drawCenteredInstruction(const NavigationStep& step);
    void drawStepIndicator();
    bool shouldAutoAdvance() const;
    float getAnimationTime() const;
};
