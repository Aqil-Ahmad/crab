/**
 * UI Highlighter Implementation
 * 
 * Renders visual overlays on ImGui interface to guide users through
 * step-by-step navigation instructions from AINavigator.
 */

#include "ui_highlighter.h"
#include <cmath>
#include <algorithm>

// ============================================================================
// Constructor
// ============================================================================

UIHighlighter::UIHighlighter()
    : m_current_step_index(0)
    , m_is_active(false)
    , m_should_execute_action(false)
{
}

// ============================================================================
// Navigation Control
// ============================================================================

void UIHighlighter::startNavigation(const std::vector<NavigationStep>& steps)
{
    if (steps.empty()) {
        return;
    }
    
    m_current_steps = steps;
    m_current_step_index = 0;
    m_is_active = true;
    
    auto now = std::chrono::steady_clock::now();
    m_step_start_time = now;
    m_animation_start_time = now;
}

void UIHighlighter::stopNavigation()
{
    m_is_active = false;
    m_current_steps.clear();
    m_current_step_index = 0;
}

void UIHighlighter::advanceToNextStep(bool executeAction)
{
    // If executeAction is requested and we have a current step, store the element ID
    if (executeAction && m_current_step_index < m_current_steps.size()) {
        m_should_execute_action = true;
        m_execute_element_id = m_current_steps[m_current_step_index].ui_element;
    }
    
    m_current_step_index++;
    
    if (m_current_step_index >= m_current_steps.size()) {
        stopNavigation();
        return;
    }
    
    m_step_start_time = std::chrono::steady_clock::now();
}

bool UIHighlighter::shouldExecuteAction()
{
    bool result = m_should_execute_action;
    m_should_execute_action = false;  // Reset after checking
    return result;
}

const NavigationStep* UIHighlighter::getCurrentStep() const
{
    if (!m_is_active || m_current_step_index >= m_current_steps.size()) {
        return nullptr;
    }
    return &m_current_steps[m_current_step_index];
}

// ============================================================================
// UI Element Registration
// ============================================================================

void UIHighlighter::registerUIElement(const std::string& id, ImVec2 pos, ImVec2 size)
{
    UIElementInfo info;
    info.position = pos;
    info.size = size;
    info.is_visible = true;
    info.element_id = id;
    
    m_ui_registry[id] = info;
}

void UIHighlighter::clearRegistry()
{
    // Mark all elements as not visible
    for (auto& pair : m_ui_registry) {
        pair.second.is_visible = false;
    }
}

void UIHighlighter::onElementClicked(const std::string& element_id)
{
    if (!m_is_active) {
        return;
    }
    
    const NavigationStep* current = getCurrentStep();
    if (!current) {
        return;
    }
    
    // Check if clicked element matches current step
    // Use case-insensitive partial matching for flexibility
    std::string clicked_lower = element_id;
    std::string target_lower = current->ui_element;
    
    std::transform(clicked_lower.begin(), clicked_lower.end(), clicked_lower.begin(), ::tolower);
    std::transform(target_lower.begin(), target_lower.end(), target_lower.begin(), ::tolower);
    
    // Check if either contains the other (flexible matching)
    bool matches = (clicked_lower.find(target_lower) != std::string::npos) ||
                   (target_lower.find(clicked_lower) != std::string::npos);
    
    if (matches) {
        advanceToNextStep();
    }
}

// ============================================================================
// Timing and Animation
// ============================================================================

float UIHighlighter::getAnimationTime() const
{
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_animation_start_time
    );
    return duration.count() / 1000.0f;
}

bool UIHighlighter::shouldAutoAdvance() const
{
    const NavigationStep* step = getCurrentStep();
    if (!step || step->delay_ms <= 0) {
        return false;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_step_start_time
    );
    
    return elapsed.count() >= step->delay_ms;
}

// ============================================================================
// Main Update / Render
// ============================================================================

void UIHighlighter::update()
{
    if (!m_is_active || m_current_steps.empty()) {
        return;
    }
    
    // Validate step index
    if (m_current_step_index >= m_current_steps.size()) {
        stopNavigation();
        return;
    }
    
    const NavigationStep& step = m_current_steps[m_current_step_index];
    float anim_time = getAnimationTime();
    
    // Calculate pulsing effect
    float pulse = std::sin(anim_time * PULSE_SPEED) * 0.5f + 0.5f;
    
    // Draw overlay and highlights
    drawOverlay();
    
    // Try to find the highlighted element
    auto it = m_ui_registry.find(step.ui_element);
    bool element_found = (it != m_ui_registry.end()) && it->second.is_visible;
    
    if (element_found) {
        drawElementHighlight(it->second, pulse);
        drawInstructionBox(step, it->second.position);
    } else {
        // Element not found - show centered instruction
        drawCenteredInstruction(step);
    }
    
    // Draw step indicator (e.g., "Step 2 of 5")
    drawStepIndicator();
    
    // Auto-advance if delay has passed (only for demonstration, usually wait for click)
    // Uncomment if you want auto-advance behavior:
    // if (shouldAutoAdvance()) {
    //     advanceToNextStep();
    // }
}

// ============================================================================
// Drawing Helpers
// ============================================================================

void UIHighlighter::drawOverlay()
{
    // Draw a light semi-transparent overlay to slightly dim the background
    // This helps focus attention on the highlighted element
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 screen_size = io.DisplaySize;
    
    // Very light overlay - just enough to show guidance is active
    draw_list->AddRectFilled(
        ImVec2(0, 0),
        screen_size,
        IM_COL32(0, 0, 0, 60)  // Light overlay (was 180, now 60)
    );
}

void UIHighlighter::drawElementHighlight(const UIElementInfo& element, float pulse)
{
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    
    // Calculate highlight area (slightly larger than element)
    float padding = 4.0f;
    ImVec2 highlight_min(element.position.x - padding, element.position.y - padding);
    ImVec2 highlight_max(element.position.x + element.size.x + padding, 
                         element.position.y + element.size.y + padding);
    
    // Draw outer glow (golden, semi-transparent) - pulsing effect
    float glow_size = 6.0f + pulse * 4.0f;
    ImVec2 glow_min(highlight_min.x - glow_size, highlight_min.y - glow_size);
    ImVec2 glow_max(highlight_max.x + glow_size, highlight_max.y + glow_size);
    
    // Multiple layers of glow for nice effect
    draw_list->AddRect(
        glow_min, glow_max,
        IM_COL32(255, 215, 0, (int)(40 + pulse * 30)),
        CORNER_RADIUS + 6.0f,
        ImDrawFlags_RoundCornersAll,
        4.0f
    );
    
    draw_list->AddRect(
        ImVec2(glow_min.x + 3, glow_min.y + 3),
        ImVec2(glow_max.x - 3, glow_max.y - 3),
        IM_COL32(255, 215, 0, (int)(80 + pulse * 40)),
        CORNER_RADIUS + 3.0f,
        ImDrawFlags_RoundCornersAll,
        3.0f
    );
    
    // Draw main animated border around element (bright gold)
    float thickness = BORDER_THICKNESS + pulse * PULSE_AMPLITUDE;
    
    draw_list->AddRect(
        highlight_min, highlight_max,
        getHighlightColor(),
        CORNER_RADIUS,
        ImDrawFlags_RoundCornersAll,
        thickness
    );
    
    // Draw animated corner accents for extra emphasis
    float corner_size = 15.0f + pulse * 5.0f;
    ImU32 accent_color = getHighlightColor();
    
    // Top-left corner
    draw_list->AddLine(
        ImVec2(highlight_min.x, highlight_min.y + corner_size),
        highlight_min,
        accent_color, thickness + 1.0f
    );
    draw_list->AddLine(
        highlight_min,
        ImVec2(highlight_min.x + corner_size, highlight_min.y),
        accent_color, thickness + 1.0f
    );
    
    // Top-right corner
    draw_list->AddLine(
        ImVec2(highlight_max.x - corner_size, highlight_min.y),
        ImVec2(highlight_max.x, highlight_min.y),
        accent_color, thickness + 1.0f
    );
    draw_list->AddLine(
        ImVec2(highlight_max.x, highlight_min.y),
        ImVec2(highlight_max.x, highlight_min.y + corner_size),
        accent_color, thickness + 1.0f
    );
    
    // Bottom-left corner
    draw_list->AddLine(
        ImVec2(highlight_min.x, highlight_max.y - corner_size),
        ImVec2(highlight_min.x, highlight_max.y),
        accent_color, thickness + 1.0f
    );
    draw_list->AddLine(
        ImVec2(highlight_min.x, highlight_max.y),
        ImVec2(highlight_min.x + corner_size, highlight_max.y),
        accent_color, thickness + 1.0f
    );
    
    // Bottom-right corner
    draw_list->AddLine(
        ImVec2(highlight_max.x - corner_size, highlight_max.y),
        ImVec2(highlight_max.x, highlight_max.y),
        accent_color, thickness + 1.0f
    );
    draw_list->AddLine(
        ImVec2(highlight_max.x, highlight_max.y),
        ImVec2(highlight_max.x, highlight_max.y - corner_size),
        accent_color, thickness + 1.0f
    );
}

void UIHighlighter::drawInstructionBox(const NavigationStep& step, const ImVec2& anchor_pos)
{
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    
    // Build instruction text
    std::string text = step.description;
    if (!step.action.empty()) {
        text = "[" + step.action + "] " + text;
    }
    
    // Calculate text size
    ImVec2 text_size = ImGui::CalcTextSize(text.c_str());
    
    // Position the box above the element
    float box_width = text_size.x + INSTRUCTION_PADDING * 2;
    float box_height = text_size.y + INSTRUCTION_PADDING * 2;
    
    ImVec2 box_min(
        anchor_pos.x,
        anchor_pos.y - INSTRUCTION_OFFSET_Y - box_height
    );
    
    // Clamp to screen bounds
    if (box_min.x + box_width > io.DisplaySize.x) {
        box_min.x = io.DisplaySize.x - box_width - 10;
    }
    if (box_min.x < 10) {
        box_min.x = 10;
    }
    if (box_min.y < 10) {
        // If no room above, place below the element
        box_min.y = anchor_pos.y + INSTRUCTION_OFFSET_Y + 30;
    }
    
    ImVec2 box_max(box_min.x + box_width, box_min.y + box_height);
    
    // Draw background with rounded corners
    draw_list->AddRectFilled(
        box_min, box_max,
        getInstructionBgColor(),
        CORNER_RADIUS
    );
    
    // Draw border
    draw_list->AddRect(
        box_min, box_max,
        getHighlightColor(),
        CORNER_RADIUS,
        ImDrawFlags_RoundCornersAll,
        2.0f
    );
    
    // Draw text
    ImVec2 text_pos(
        box_min.x + INSTRUCTION_PADDING,
        box_min.y + INSTRUCTION_PADDING
    );
    draw_list->AddText(text_pos, getInstructionTextColor(), text.c_str());
    
    // Draw arrow pointing to element
    ImVec2 arrow_tip(anchor_pos.x + 20, anchor_pos.y - 5);
    ImVec2 arrow_left(arrow_tip.x - 8, box_max.y);
    ImVec2 arrow_right(arrow_tip.x + 8, box_max.y);
    
    if (box_min.y < anchor_pos.y) {
        // Arrow pointing down
        draw_list->AddTriangleFilled(
            arrow_left, arrow_right, arrow_tip,
            getInstructionBgColor()
        );
        draw_list->AddLine(arrow_left, arrow_tip, getHighlightColor(), 2.0f);
        draw_list->AddLine(arrow_right, arrow_tip, getHighlightColor(), 2.0f);
    }
}

void UIHighlighter::drawCenteredInstruction(const NavigationStep& step)
{
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 screen_center(io.DisplaySize.x * 0.5f, io.DisplaySize.y * 0.5f);
    
    // Build instruction text
    std::string main_text = step.description;
    std::string hint_text = "Press SPACE or ENTER to continue, ESC to cancel";
    
    ImVec2 main_size = ImGui::CalcTextSize(main_text.c_str());
    ImVec2 hint_size = ImGui::CalcTextSize(hint_text.c_str());
    
    float max_width = std::max(main_size.x, hint_size.x);
    float total_height = main_size.y + hint_size.y + 15;
    
    // Box dimensions
    float box_width = max_width + INSTRUCTION_PADDING * 4;
    float box_height = total_height + INSTRUCTION_PADDING * 2;
    
    ImVec2 box_min(
        screen_center.x - box_width * 0.5f,
        screen_center.y - box_height * 0.5f
    );
    ImVec2 box_max(
        screen_center.x + box_width * 0.5f,
        screen_center.y + box_height * 0.5f
    );
    
    // Draw background
    draw_list->AddRectFilled(
        box_min, box_max,
        getInstructionBgColor(),
        CORNER_RADIUS * 2
    );
    
    // Draw border
    draw_list->AddRect(
        box_min, box_max,
        getHighlightColor(),
        CORNER_RADIUS * 2,
        ImDrawFlags_RoundCornersAll,
        3.0f
    );
    
    // Draw main instruction text (centered)
    ImVec2 main_pos(
        screen_center.x - main_size.x * 0.5f,
        box_min.y + INSTRUCTION_PADDING
    );
    draw_list->AddText(main_pos, getInstructionTextColor(), main_text.c_str());
    
    // Draw hint text (smaller, dimmer)
    ImVec2 hint_pos(
        screen_center.x - hint_size.x * 0.5f,
        main_pos.y + main_size.y + 10
    );
    draw_list->AddText(hint_pos, IM_COL32(100, 200, 255, 200), hint_text.c_str());
}

void UIHighlighter::drawStepIndicator()
{
    ImDrawList* draw_list = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    
    // Build step indicator text
    char indicator[64];
    snprintf(indicator, sizeof(indicator), "Step %zu of %zu", 
             m_current_step_index + 1, m_current_steps.size());
    
    ImVec2 text_size = ImGui::CalcTextSize(indicator);
    
    // Position in top-right corner
    float padding = 15.0f;
    ImVec2 box_max(io.DisplaySize.x - padding, padding + text_size.y + padding);
    ImVec2 box_min(box_max.x - text_size.x - padding * 2, padding);
    
    // Draw background
    draw_list->AddRectFilled(
        box_min, box_max,
        IM_COL32(0, 0, 0, 200),
        CORNER_RADIUS
    );
    
    // Draw border
    draw_list->AddRect(
        box_min, box_max,
        getStepIndicatorColor(),
        CORNER_RADIUS,
        ImDrawFlags_RoundCornersAll,
        1.5f
    );
    
    // Draw text
    ImVec2 text_pos(box_min.x + padding, box_min.y + padding * 0.5f);
    draw_list->AddText(text_pos, getStepIndicatorColor(), indicator);
    
    // Draw "Press ESC to cancel" hint
    const char* cancel_hint = "Press ESC to cancel";
    ImVec2 hint_size = ImGui::CalcTextSize(cancel_hint);
    ImVec2 hint_pos(
        io.DisplaySize.x - hint_size.x - padding,
        io.DisplaySize.y - hint_size.y - padding
    );
    draw_list->AddText(hint_pos, IM_COL32(150, 150, 150, 180), cancel_hint);
}
