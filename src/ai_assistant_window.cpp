/**
 * AI Assistant Window Implementation
 * 
 * Provides an ImGui chat interface for AI-guided navigation.
 */

#include "ai_assistant_window.h"
#include "ai_navigator.h"
#include "ui_highlighter.h"

#include <cmath>
#include <algorithm>

// ============================================================================
// Static Data
// ============================================================================

const char* AIAssistantWindow::EXAMPLE_QUERIES[] = {
    "How do I apply gaussian blur?",
    "How do I add a new layer?",
    "How do I change brightness?",
    "How to use the brush tool?",
    "How do I save my image?",
    "How to flip an image?"
};

const int AIAssistantWindow::NUM_EXAMPLE_QUERIES = 
    sizeof(EXAMPLE_QUERIES) / sizeof(EXAMPLE_QUERIES[0]);

// ============================================================================
// Constructor
// ============================================================================

AIAssistantWindow::AIAssistantWindow(AINavigator* nav, UIHighlighter* highlight)
    : m_ai_navigator(nav)
    , m_ui_highlighter(highlight)
    , m_is_window_open(false)
    , m_is_processing(false)
    , m_scroll_to_bottom(false)
    , m_focus_input(false)
{
    memset(m_input_buffer, 0, sizeof(m_input_buffer));
    
    // Add welcome message
    addMessage("Hi! I'm your AI assistant. Ask me how to use any feature in Grid Image Editor!", false);
}

// ============================================================================
// Public Methods
// ============================================================================

void AIAssistantWindow::clearHistory()
{
    m_chat_history.clear();
    addMessage("Chat cleared. How can I help you?", false);
}

// ============================================================================
// Main Render
// ============================================================================

void AIAssistantWindow::render()
{
    if (!m_is_window_open) {
        return;
    }
    
    // Set initial window size and position
    ImGui::SetNextWindowSize(ImVec2(WINDOW_WIDTH, WINDOW_HEIGHT), ImGuiCond_FirstUseEver);
    
    // Window flags
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_None;
    
    // Begin window
    if (!ImGui::Begin("AI Assistant", &m_is_window_open, window_flags)) {
        ImGui::End();
        return;
    }
    
    // Render sections
    renderHeader();
    ImGui::Separator();
    renderChatHistory();
    ImGui::Separator();
    renderInputArea();
    
    // Handle focus
    if (m_focus_input) {
        ImGui::SetKeyboardFocusHere(-1);
        m_focus_input = false;
    }
    
    ImGui::End();
}

// ============================================================================
// Header Section
// ============================================================================

void AIAssistantWindow::renderHeader()
{
    // Title
    ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "Ask me how to use any feature!");
    
    // Show navigation status if active
    if (m_ui_highlighter && m_ui_highlighter->isActive()) {
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), " [Guiding...]");
        
        // Stop navigation button
        ImGui::SameLine();
        if (ImGui::SmallButton("Stop")) {
            m_ui_highlighter->stopNavigation();
        }
    }
    
    ImGui::Spacing();
    
    // Example queries (clickable)
    ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Try asking:");
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.25f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.25f, 0.35f, 1.0f));
    
    for (int i = 0; i < std::min(3, NUM_EXAMPLE_QUERIES); i++) {
        if (i > 0) ImGui::SameLine();
        
        // Truncate long examples
        std::string example = EXAMPLE_QUERIES[i];
        if (example.length() > 20) {
            example = example.substr(0, 17) + "...";
        }
        
        if (ImGui::SmallButton(example.c_str())) {
            // Auto-fill and submit
            strncpy(m_input_buffer, EXAMPLE_QUERIES[i], sizeof(m_input_buffer) - 1);
            processQuery(EXAMPLE_QUERIES[i]);
        }
    }
    
    ImGui::PopStyleColor(3);
}

// ============================================================================
// Chat History Section
// ============================================================================

void AIAssistantWindow::renderChatHistory()
{
    // Calculate available height for chat
    float input_area_height = 60.0f;
    float available_height = ImGui::GetContentRegionAvail().y - input_area_height;
    
    // Scrollable chat region
    ImGui::BeginChild("ChatHistory", ImVec2(0, available_height), true, 
                      ImGuiWindowFlags_HorizontalScrollbar);
    
    for (size_t i = 0; i < m_chat_history.size(); i++) {
        const ChatMessage& msg = m_chat_history[i];
        
        ImGui::PushID((int)i);
        
        if (msg.is_user) {
            // User message - light blue background
            pushUserMessageStyle();
            ImGui::TextColored(ImVec4(0.3f, 0.6f, 1.0f, 1.0f), "You:");
        } else {
            // AI message - gray background
            pushAIMessageStyle();
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, 1.0f), "AI:");
        }
        
        // Message text with wrapping
        ImGui::SameLine();
        ImGui::PushTextWrapPos(ImGui::GetContentRegionAvail().x);
        ImGui::TextUnformatted(msg.text.c_str());
        ImGui::PopTextWrapPos();
        
        // Navigation indicator
        if (msg.is_navigation) {
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(1.0f, 0.85f, 0.0f, 1.0f), "[Guiding]");
        }
        
        popMessageStyle();
        
        ImGui::Spacing();
        ImGui::PopID();
    }
    
    // Show loading indicator
    if (m_is_processing) {
        renderLoadingIndicator();
    }
    
    // Auto-scroll to bottom
    if (m_scroll_to_bottom) {
        ImGui::SetScrollHereY(1.0f);
        m_scroll_to_bottom = false;
    }
    
    ImGui::EndChild();
}

// ============================================================================
// Input Area Section
// ============================================================================

void AIAssistantWindow::renderInputArea()
{
    bool is_disabled = m_is_processing || 
                       (m_ai_navigator && !m_ai_navigator->isInitialized());
    
    if (is_disabled) {
        ImGui::BeginDisabled();
    }
    
    // Input field
    ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x - 60);
    
    ImGuiInputTextFlags input_flags = ImGuiInputTextFlags_EnterReturnsTrue;
    bool submitted = ImGui::InputText("##Query", m_input_buffer, sizeof(m_input_buffer), input_flags);
    
    ImGui::PopItemWidth();
    
    ImGui::SameLine();
    
    // Ask button
    if (ImGui::Button("Ask", ImVec2(50, 0)) || submitted) {
        std::string query(m_input_buffer);
        
        // Trim whitespace
        query.erase(0, query.find_first_not_of(" \t\n\r"));
        query.erase(query.find_last_not_of(" \t\n\r") + 1);
        
        if (!query.empty()) {
            processQuery(query);
            memset(m_input_buffer, 0, sizeof(m_input_buffer));
            m_focus_input = true;
        }
    }
    
    if (is_disabled) {
        ImGui::EndDisabled();
    }
    
    // Show hint if navigator not initialized
    if (m_ai_navigator && !m_ai_navigator->isInitialized()) {
        ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.3f, 1.0f), 
                          "AI model not loaded. Please initialize first.");
    }
}

// ============================================================================
// Loading Indicator
// ============================================================================

void AIAssistantWindow::renderLoadingIndicator()
{
    // Animated dots
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - m_processing_start
    ).count();
    
    int dots = ((elapsed / 400) % 4);
    std::string thinking = "Thinking";
    for (int i = 0; i < dots; i++) {
        thinking += ".";
    }
    
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "%s", thinking.c_str());
}

// ============================================================================
// Query Processing
// ============================================================================

void AIAssistantWindow::processQuery(const std::string& query)
{
    if (!m_ai_navigator) {
        addMessage("Error: AI Navigator not available.", false);
        return;
    }
    
    if (!m_ai_navigator->isInitialized()) {
        addMessage("Error: AI model not loaded yet.", false);
        return;
    }
    
    // Add user message
    addMessage(query, true);
    
    // Set processing state
    m_is_processing = true;
    m_processing_start = std::chrono::steady_clock::now();
    
    // Process the query - returns AIResponse with both text and optional navigation
    AIResponse response = m_ai_navigator->processQuery(query);
    
    m_is_processing = false;
    
    // Always show the text response first
    if (!response.text_response.empty()) {
        addMessage(response.text_response, false, response.has_navigation);
    } else {
        addMessage("I couldn't understand that. Try asking about features, tools, or how to do something.", false);
    }
    
    // If navigation is available, trigger visual highlighting
    if (response.has_navigation && !response.steps.empty()) {
        if (m_ui_highlighter) {
            m_ui_highlighter->startNavigation(response.steps);
        }
    }
    
    m_scroll_to_bottom = true;
}

// ============================================================================
// Helper Methods
// ============================================================================

void AIAssistantWindow::addMessage(const std::string& text, bool is_user, bool is_nav)
{
    m_chat_history.emplace_back(text, is_user, is_nav);
    m_scroll_to_bottom = true;
    
    // Limit history size
    const size_t MAX_HISTORY = 100;
    while (m_chat_history.size() > MAX_HISTORY) {
        m_chat_history.erase(m_chat_history.begin());
    }
}

void AIAssistantWindow::pushUserMessageStyle()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.15f, 0.25f, 0.35f, 0.5f));
}

void AIAssistantWindow::pushAIMessageStyle()
{
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.2f, 0.2f, 0.2f, 0.5f));
}

void AIAssistantWindow::popMessageStyle()
{
    ImGui::PopStyleColor();
}
