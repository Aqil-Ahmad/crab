#pragma once

/**
 * AI Assistant Window - Chat interface for AI-guided navigation
 * 
 * This module provides an ImGui-based chat window where users can ask
 * questions about how to use the photo editor. The AI responds with
 * step-by-step instructions that are visualized using the UIHighlighter.
 */

#include <imgui.h>
#include <string>
#include <vector>
#include <chrono>

// Forward declarations - actual includes in .cpp
class AINavigator;
class UIHighlighter;
struct NavigationStep;

/**
 * Represents a single message in the chat history
 */
struct ChatMessage {
    std::string text;
    bool is_user;           // true = user message, false = AI response
    bool is_navigation;     // true = this message triggered navigation
    
    ChatMessage(const std::string& msg, bool user, bool nav = false)
        : text(msg), is_user(user), is_navigation(nav) {}
};

/**
 * ImGui window providing a chat interface for AI navigation assistance
 * 
 * Features:
 * - Text input for user queries
 * - Scrollable chat history
 * - Integration with AINavigator for query processing
 * - Integration with UIHighlighter for visual guidance
 * - Loading state while AI processes queries
 * 
 * Usage:
 *   AINavigator nav;
 *   UIHighlighter highlighter;
 *   AIAssistantWindow assistant(&nav, &highlighter);
 *   
 *   // In render loop:
 *   assistant.render();
 *   
 *   // Toggle with F1:
 *   if (ImGui::IsKeyPressed(ImGuiKey_F1)) assistant.toggle();
 */
class AIAssistantWindow {
public:
    /**
     * Construct the assistant window with references to navigation components
     * @param nav Pointer to initialized AINavigator instance
     * @param highlight Pointer to UIHighlighter instance
     */
    AIAssistantWindow(AINavigator* nav, UIHighlighter* highlight);
    ~AIAssistantWindow() = default;
    
    // Prevent copying
    AIAssistantWindow(const AIAssistantWindow&) = delete;
    AIAssistantWindow& operator=(const AIAssistantWindow&) = delete;
    
    /**
     * Render the assistant window (call every frame)
     */
    void render();
    
    /**
     * Toggle window visibility
     */
    void toggle() { m_is_window_open = !m_is_window_open; }
    
    /**
     * Open the window
     */
    void open() { m_is_window_open = true; }
    
    /**
     * Close the window
     */
    void close() { m_is_window_open = false; }
    
    /**
     * Check if window is currently open
     */
    bool isOpen() const { return m_is_window_open; }
    
    /**
     * Check if AI is currently processing a query
     */
    bool isProcessing() const { return m_is_processing; }
    
    /**
     * Clear the chat history
     */
    void clearHistory();

private:
    // Dependencies (not owned)
    AINavigator* m_ai_navigator;
    UIHighlighter* m_ui_highlighter;
    
    // UI State
    char m_input_buffer[512];
    std::vector<ChatMessage> m_chat_history;
    bool m_is_window_open;
    bool m_is_processing;
    bool m_scroll_to_bottom;
    bool m_focus_input;
    
    // Animation
    std::chrono::steady_clock::time_point m_processing_start;
    
    // Window settings
    static constexpr float WINDOW_WIDTH = 400.0f;
    static constexpr float WINDOW_HEIGHT = 500.0f;
    
    // Example queries to show users
    static const char* EXAMPLE_QUERIES[];
    static const int NUM_EXAMPLE_QUERIES;
    
    // Helper methods
    void renderHeader();
    void renderChatHistory();
    void renderInputArea();
    void renderLoadingIndicator();
    void processQuery(const std::string& query);
    void addMessage(const std::string& text, bool is_user, bool is_nav = false);
    
    // Styling helpers
    void pushUserMessageStyle();
    void pushAIMessageStyle();
    void popMessageStyle();
};
