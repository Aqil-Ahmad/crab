#pragma once

/**
 * AI Navigator - LLM-powered navigation assistant for Grid Image Editor
 * 
 * This module uses llama.cpp to process natural language queries and generate
 * step-by-step navigation instructions for the Grid Image Editor UI.
 */

#include <string>
#include <vector>

// Forward declarations for llama.cpp types
struct llama_model;
struct llama_context;
struct llama_sampler;
struct llama_vocab;

/**
 * Represents a single navigation step in the UI
 */
struct NavigationStep {
    std::string ui_element;   // Target UI element (e.g., "File menu", "Layers Panel")
    std::string action;       // Action to perform (e.g., "Click", "Select", "Toggle")
    std::string description;  // Human-readable instruction
    int delay_ms;             // Wait time before next step (milliseconds)
    
    NavigationStep() : delay_ms(300) {}
    NavigationStep(const std::string& elem, const std::string& act, 
                   const std::string& desc, int delay = 300)
        : ui_element(elem), action(act), description(desc), delay_ms(delay) {}
};

/**
 * Represents the AI's response - can include BOTH text AND navigation steps
 * 
 * This enables dual-mode responses where conversational text is shown
 * while optional navigation highlighting guides the user visually.
 */
struct AIResponse {
    std::string text_response;              // Conversational text to display
    std::vector<NavigationStep> steps;      // Optional navigation steps (empty if no navigation)
    bool has_navigation;                    // True if visual guidance should be shown
    
    AIResponse() : has_navigation(false) {}
    
    // Create a text-only response (conversation mode)
    static AIResponse chat(const std::string& text) {
        AIResponse r;
        r.text_response = text;
        r.has_navigation = false;
        return r;
    }
    
    // Create a navigation response with text explanation
    static AIResponse navigation(const std::string& text, const std::vector<NavigationStep>& nav_steps) {
        AIResponse r;
        r.text_response = text;
        r.steps = nav_steps;
        r.has_navigation = !nav_steps.empty();
        return r;
    }
    
    // Create a navigation-only response (legacy compatibility)
    static AIResponse navigation(const std::vector<NavigationStep>& nav_steps) {
        AIResponse r;
        r.steps = nav_steps;
        r.has_navigation = !nav_steps.empty();
        // Generate default text from steps
        if (!nav_steps.empty()) {
            r.text_response = "I'll guide you through " + std::to_string(nav_steps.size()) + " step(s).";
        }
        return r;
    }
};

/**
 * AI-powered navigation assistant using llama.cpp
 * 
 * Example usage:
 *   AINavigator nav;
 *   if (nav.initialize("path/to/model.gguf")) {
 *       auto response = nav.processQuery("How do I apply gaussian blur?");
 *       if (response.is_navigation) {
 *           for (const auto& step : response.steps) {
 *               std::cout << step.description << std::endl;
 *           }
 *       } else {
 *           std::cout << response.chat_response << std::endl;
 *       }
 *       nav.cleanup();
 *   }
 */
class AINavigator {
public:
    AINavigator();
    ~AINavigator();
    
    // Prevent copying (llama resources are not copyable)
    AINavigator(const AINavigator&) = delete;
    AINavigator& operator=(const AINavigator&) = delete;
    
    /**
     * Initialize the LLM with the specified model file
     * @param model_path Path to the GGUF model file
     * @return true if initialization succeeded, false otherwise
     */
    bool initialize(const std::string& model_path);
    
    /**
     * Process a natural language query and generate a response
     * @param query User's question
     * @return AIResponse containing either chat text or navigation steps
     */
    AIResponse processQuery(const std::string& query);
    
    /**
     * Check if the navigator has been initialized successfully
     */
    bool isInitialized() const { return m_initialized; }
    
    /**
     * Clean up all llama.cpp resources
     */
    void cleanup();

private:
    // llama.cpp components
    llama_model* m_model;
    llama_context* m_ctx;
    llama_sampler* m_sampler;
    const llama_vocab* m_vocab;
    
    bool m_initialized;
    
    // Configuration - Optimized for speed on quad-core CPU
    static constexpr int CONTEXT_SIZE = 2048;  // Larger context for dual-prompt
    static constexpr int N_THREADS = 4;        // Use all 4 cores
    static constexpr float TEMPERATURE = 0.2f; // Lower for more deterministic output
    static constexpr int MAX_TOKENS = 400;     // Larger for dual-mode responses with text+JSON

    
    /**
     * Query the LLM with a formatted prompt
     * @param prompt Full prompt including system message and user query
     * @return Raw text response from the LLM
     */
    std::string queryLLM(const std::string& prompt);
    
    /**
     * Parse JSON response into NavigationStep objects
     * @param json_str JSON string containing steps array
     * @return Vector of NavigationStep objects, empty if parsing failed
     */
    std::vector<NavigationStep> parseJSONResponse(const std::string& json_str);
    
    /**
     * Build the system prompt with feature map context
     */
    std::string buildSystemPrompt() const;
    
    /**
     * Classify the query as NAVIGATION or CONVERSATION
     * @param user_query The user's input query
     * @return "NAVIGATION" or "CONVERSATION"
     */
    std::string classifyQuery(const std::string& user_query);
    
    /**
     * Build prompt for navigation queries (how-to questions)
     * @param user_query The user's navigation request
     * @return Formatted prompt with UI element mapping
     */
    std::string buildNavigationPrompt(const std::string& user_query) const;
    
    /**
     * Build prompt for conversation queries (general questions)
     * @param user_query The user's question
     * @return Formatted conversational prompt
     */
    std::string buildConversationPrompt(const std::string& user_query) const;
    
    /**
     * Format a prompt with Llama 3.2 Instruct template
     * @param system_prompt The system instructions
     * @param force_json If true, force JSON output by starting with '{'
     * @return Formatted prompt ready for LLM
     */
    std::string formatPromptForMode(const std::string& system_prompt, bool force_json) const;
    
    /**
     * Build dual-mode prompt that generates BOTH text explanation AND navigation steps
     * @param user_query The user's query
     * @return Formatted prompt for dual response generation
     */
    std::string buildDualPrompt(const std::string& user_query) const;
    
    /**
     * Extract the TEXT section from structured LLM output
     * @param llm_output Raw output from LLM
     * @return The text response portion
     */
    std::string extractTextSection(const std::string& llm_output) const;
    
    /**
     * Extract whether navigation is requested from LLM output
     * @param llm_output Raw output from LLM
     * @return true if NAVIGATION: YES was found
     */
    bool extractNavigationFlag(const std::string& llm_output) const;
    
    /**
     * Extract the STEPS JSON section from LLM output
     * @param llm_output Raw output from LLM
     * @return JSON string of steps, or "none" if not found
     */
    std::string extractStepsSection(const std::string& llm_output) const;
    
    /**
     * Parse a JSON array of steps directly (without wrapper object)
     * @param json_str JSON array string
     * @return Vector of NavigationStep objects
     */
    std::vector<NavigationStep> parseJSONSteps(const std::string& json_str);
};
