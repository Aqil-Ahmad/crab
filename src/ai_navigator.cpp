/**
 * AI Navigator Implementation
 * 
 * Uses llama.cpp to process natural language queries and generate
 * step-by-step navigation instructions for Grid Image Editor.
 */

#include "ai_navigator.h"

// llama.cpp includes
#include <llama.h>

// JSON parser
#include <json.hpp>

#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstring>

using json = nlohmann::json;

// ============================================================================
// Constructor / Destructor
// ============================================================================

AINavigator::AINavigator()
    : m_model(nullptr)
    , m_ctx(nullptr)
    , m_sampler(nullptr)
    , m_vocab(nullptr)
    , m_initialized(false)
{
}

AINavigator::~AINavigator()
{
    cleanup();
}

// ============================================================================
// Initialization
// ============================================================================

bool AINavigator::initialize(const std::string& model_path)
{
    if (m_initialized) {
        std::cerr << "[AINavigator] Already initialized, call cleanup() first\n";
        return false;
    }

    // Initialize llama backend
    llama_backend_init();

    // Load model with default parameters
    llama_model_params model_params = llama_model_default_params();
    model_params.n_gpu_layers = 0;  // CPU only for compatibility
    
    std::cout << "[AINavigator] Loading model: " << model_path << "\n";
    m_model = llama_model_load_from_file(model_path.c_str(), model_params);
    
    if (!m_model) {
        std::cerr << "[AINavigator] Failed to load model from: " << model_path << "\n";
        cleanup();
        return false;
    }
    
    // Get vocabulary from model
    m_vocab = llama_model_get_vocab(m_model);
    if (!m_vocab) {
        std::cerr << "[AINavigator] Failed to get vocabulary from model\n";
        cleanup();
        return false;
    }

    // Create context with specified parameters
    llama_context_params ctx_params = llama_context_default_params();
    ctx_params.n_ctx = CONTEXT_SIZE;
    ctx_params.n_threads = N_THREADS;
    ctx_params.n_threads_batch = N_THREADS;
    
    m_ctx = llama_init_from_model(m_model, ctx_params);
    
    if (!m_ctx) {
        std::cerr << "[AINavigator] Failed to create llama context\n";
        cleanup();
        return false;
    }

    // Create sampler chain with temperature for controlled randomness
    llama_sampler_chain_params sampler_params = llama_sampler_chain_default_params();
    m_sampler = llama_sampler_chain_init(sampler_params);
    
    if (!m_sampler) {
        std::cerr << "[AINavigator] Failed to create sampler chain\n";
        cleanup();
        return false;
    }
    
    // Add temperature sampler for slight variation, then greedy selection
    llama_sampler_chain_add(m_sampler, llama_sampler_init_temp(TEMPERATURE));
    llama_sampler_chain_add(m_sampler, llama_sampler_init_greedy());

    m_initialized = true;
    std::cout << "[AINavigator] Initialization complete\n";
    return true;
}

// ============================================================================
// Cleanup
// ============================================================================

void AINavigator::cleanup()
{
    if (m_sampler) {
        llama_sampler_free(m_sampler);
        m_sampler = nullptr;
    }
    
    if (m_ctx) {
        llama_free(m_ctx);
        m_ctx = nullptr;
    }
    
    if (m_model) {
        llama_model_free(m_model);
        m_model = nullptr;
    }
    
    m_vocab = nullptr;
    m_initialized = false;
    
    llama_backend_free();
}

// ============================================================================
// Query Processing
// ============================================================================

AIResponse AINavigator::processQuery(const std::string& query)
{
    if (!m_initialized) {
        return AIResponse::chat("AI assistant not initialized. Please wait for the model to load.");
    }
    
    // Use dual-prompt to generate BOTH text explanation AND optional navigation
    std::string dual_prompt = buildDualPrompt(query);
    std::string formatted = formatPromptForMode(dual_prompt, false);
    std::string llm_output = queryLLM(formatted);
    
    std::cout << "[AINavigator] Raw LLM output:\n" << llm_output << std::endl;
    
    // Extract structured sections from LLM output
    std::string text_response = extractTextSection(llm_output);
    bool has_nav = extractNavigationFlag(llm_output);
    
    std::cout << "[AINavigator] Text: " << text_response << std::endl;
    std::cout << "[AINavigator] Has navigation: " << (has_nav ? "YES" : "NO") << std::endl;
    
    if (has_nav) {
        std::string steps_json = extractStepsSection(llm_output);
        std::cout << "[AINavigator] Steps JSON: " << steps_json << std::endl;
        
        if (steps_json != "none" && !steps_json.empty()) {
            std::vector<NavigationStep> steps = parseJSONSteps(steps_json);
            
            if (!steps.empty()) {
                // Return both text AND navigation
                return AIResponse::navigation(text_response, steps);
            }
        }
    }
    
    // Text-only response
    if (!text_response.empty()) {
        return AIResponse::chat(text_response);
    }
    
    // Fallback
    return AIResponse::chat("I'm here to help with Grid Image Editor. Ask me how to use features or about the application!");
}


// ============================================================================
// System Prompt
// ============================================================================

std::string AINavigator::buildSystemPrompt() const
{
    return std::string(R"(You are Grid Image Editor's AI assistant.

=== CRITICAL: RESPONSE MODE DETECTION ===

NAVIGATION MODE (output JSON) - ONLY use when user asks HOW TO DO something:
- "how do I..." / "how can I..." / "how to..."
- "where is..." / "where can I find..."
- "show me how to..."

CHAT MODE (output plain text) - Use for EVERYTHING ELSE:
- "what is..." / "what does..." / "what are..."
- "hi" / "hello" / greetings
- "tell me about..." / "explain..."
- "list..." / "describe..."
- Any general question

=== CHAT MODE EXAMPLES ===

Q: "hi" or "hello"
A: Hello! I'm Grid's AI assistant. Ask me about features or how to do something!

Q: "what does this app do?"
A: Grid is a free image editor. You can create images, work with layers, draw with brushes, and apply filters like blur, sharpen, brightness, and more.

Q: "what features are available?"
A: Grid has: file operations, layer management, drawing tools (brush, eraser, fill), color picker, and filters (blur, sharpen, brightness, grayscale, sepia, etc).

=== NAVIGATION MODE (JSON) - Only for "how to" questions ===

Q: "how do I sharpen an image?"
A: {"steps":[{"ui_element":"Manipulations menu","action":"Click","description":"Click 'Manipulations' menu","delay_ms":500},{"ui_element":"Sharpen button","action":"Click","description":"Click 'Sharpen'","delay_ms":300}]}

Q: "how do I open an image?"
A: {"steps":[{"ui_element":"File menu","action":"Click","description":"Click 'File' menu","delay_ms":500},{"ui_element":"Open button","action":"Click","description":"Click 'Open'","delay_ms":300}]}

=== UI ELEMENTS (for navigation) ===
Menus: "File menu", "Edit menu", "Manipulations menu"
File: "New button", "Open button", "Export button"
Edit: "Undo button", "Redo button"
Filters: "Sharpen button", "Gaussian Blur button", "Brightness button", "Gray Scale button", "Invert button", "Sepia button"
Tools: "Brush Tool", "Eraser Tool", "Fill Tool", "Move Tool"
Layers: "Create new layer button"

IMPORTANT: For filters, ALWAYS open "Manipulations menu" first!)") + "\n\nResponse:";
}

std::string AINavigator::formatPromptForMode(const std::string& system_prompt, bool force_json) const
{
    std::ostringstream oss;
    
    // Use Llama 3.2 Instruct format
    // Note: BOS is added by llama_tokenize, so we don't add <|begin_of_text|> here
    oss << "<|start_header_id|>system<|end_header_id|>\n\n";
    oss << system_prompt;
    oss << "<|eot_id|><|start_header_id|>assistant<|end_header_id|>\n\n";
    
    if (force_json) {
        oss << "{";  // Force JSON start for navigation
    }
    
    return oss.str();
}

// ============================================================================
// Query Classification
// ============================================================================

std::string AINavigator::classifyQuery(const std::string& user_query)
{
    std::string classification_prompt = R"(You are a query classifier for a photo editor assistant.
Respond with EXACTLY ONE WORD: either "NAVIGATION" or "CONVERSATION"

NAVIGATION: User wants to know HOW to do something (e.g., "how do I blur", "where is brightness", "how to add layer")
CONVERSATION: User wants information, explanation, or general chat (e.g., "what is this app", "what can you do", "tell me about filters", "hi", "hello")

User query: )" + user_query + R"(

Classification:)";
    
    std::string formatted = formatPromptForMode(classification_prompt, false);
    std::string response = queryLLM(formatted);
    
    // Normalize response - convert to uppercase and look for keywords
    std::string upper;
    for (char c : response) {
        upper += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    
    std::cout << "[AINavigator] Classification response: " << response << std::endl;
    
    if (upper.find("NAVIGATION") != std::string::npos) {
        return "NAVIGATION";
    }
    return "CONVERSATION";
}

// ============================================================================
// Mode-Specific Prompts
// ============================================================================

std::string AINavigator::buildNavigationPrompt(const std::string& user_query) const
{
    return R"(You are a navigation assistant for Grid Image Editor.

AVAILABLE FEATURES:
- File Operations: New (Ctrl+N), Open (Ctrl+O), Export (Ctrl+Shift+S)
- Edit: Undo (Ctrl+Z), Redo (Ctrl+Y)
- Tools: Brush (B), Eraser (E), Fill (G), Move (V)
- Filters: Gaussian Blur, Box Blur, Sharpen, Edge Detection, Pixelate
- Adjustments: Brightness, Contrast, Gray Scale, Invert, Sepia
- Transforms: Flip Horizontal, Flip Vertical, Rotate (custom angle, 90° CW/CCW)
- Layers: Create, Duplicate, Delete, Reorder, Set Opacity, Blend Modes
- View: Reset Canvas (Ctrl+Shift+R), Toggle Menu Bar (Alt)

UI ELEMENT NAMES (use EXACTLY):
Menus: "File menu", "Edit menu", "Manipulations menu"
File menu items: "New button", "Open button", "Export button"
Edit menu items: "Undo button", "Redo button"
Filter buttons: "Sharpen button", "Gaussian Blur button", "Box Blur button", "Brightness button", "Contrast button", "Gray Scale button", "Invert button", "Sepia button", "Edge Detection button", "Pixelate button"
Transform buttons: "Flip Horizontal button", "Flip Vertical button", "Rotate button"
Tools: "Brush Tool", "Eraser Tool", "Fill Tool", "Move Tool"
Layers: "Create new layer button"
Dialog controls: "Apply button", "Blur Strength slider", "Brightness slider", "Contrast slider", "Pixelate slider", "Rotate slider"

RULES:
1. Respond with ONLY valid JSON (no markdown, no extra text)
2. For filters/transforms, ALWAYS open "Manipulations menu" first
3. Use 500ms delay for menu opens, 300ms for button clicks, 500ms for sliders

User query: )" + user_query + R"(

JSON:)";
}

std::string AINavigator::buildConversationPrompt(const std::string& user_query) const
{
    return R"(You are a helpful assistant for Grid Image Editor, a photo editing application.

Grid Image Editor features:
- Professional layer system with 24+ blend modes
- Brush, Eraser, and Fill tools with customizable settings
- Filters: blur effects, edge detection, pixelation, sharpening
- Adjustments: brightness, contrast, color correction, sepia, invert
- Transform tools: flip, rotate, resize
- File support: PNG, JPG, JPEG, BMP
- Canvas navigation: zoom, pan, reset view

Respond naturally and conversationally. Be friendly and informative. Keep responses under 3 sentences unless explaining something complex.

User: )" + user_query + R"(

Response:)";
}

std::string AINavigator::buildDualPrompt(const std::string& user_query) const
{
    return R"(You are an AI assistant for Grid Image Editor.

AVAILABLE FEATURES:
File: New (Ctrl+N), Open (Ctrl+O), Export (Ctrl+Shift+S)
Edit: Undo (Ctrl+Z), Redo (Ctrl+Y), Fill with primary color
View: Menu Bar (Alt), Transparent Checker Shader

Transform Menu: Flip Horizontal, Flip Vertical, Rotate Custom Angle, Rotate 90° CW, Rotate 90° CCW
Adjust Menu: Brightness, Contrast, Gray Scale, Invert Colors, Sepia Tone
Filters Menu: Box Blur, Gaussian Blur, Pixelate, Edge Detection, Sharpen

Layers Panel: Create new layer, Duplicate, Delete, Reorder, Opacity slider
BLEND MODES (24 modes): Normal, Dissolve, Darken, Multiply, Color Burn, Linear Burn, Darker Color, Lighten, Screen, Color Dodge, Linear Dodge, Lighter Color, Overlay, Soft Light, Hard Light, Vivid Light, Linear Light, Pin Light, Hard Mix, Difference, Exclusion, Subtract, Divide, Hue, Saturation, Color, Luminosity

Tools Panel: No Tool (N), Move (V), Brush (B), Eraser (E), Fill (G), Zoom (Z)
Color Panel: Hue Bar picker, RGB input, Quick color swatches, Primary/Secondary colors

RESPONSE FORMAT:
You MUST respond in this EXACT format with all three lines:

TEXT: [Your conversational response here - be natural and helpful]
NAVIGATION: [YES or NO]
STEPS: [If NAVIGATION is YES, provide JSON array, otherwise write "none"]

UI ELEMENT NAMES (use EXACTLY):
Menus: "File menu", "Edit menu", "Transform menu", "Adjust menu", "Filters menu"
File items: "New button", "Open button", "Export button"
Edit items: "Undo button", "Redo button", "Fill primary button"
Transform items: "Flip Horizontal button", "Flip Vertical button", "Rotate button", "Rotate 90 CW button", "Rotate 90 CCW button"
Adjust items: "Brightness button", "Contrast button", "Gray Scale button", "Invert button", "Sepia button"
Filter items: "Box Blur button", "Gaussian Blur button", "Pixelate button", "Edge Detection button", "Sharpen button"
Tools: "Brush Tool", "Eraser Tool", "Fill Tool", "Move Tool", "Zoom Tool"
Layers: "Create new layer button", "Blend Mode dropdown"
Controls: "Apply button", various sliders

EXAMPLES:

Query: "what is this app about"
TEXT: This is Grid Image Editor, a professional photo editing application with layers, blend modes, filters, and drawing tools!
NAVIGATION: NO
STEPS: none

Query: "how to apply box blur"
TEXT: To apply box blur, open the Filters menu and select Box Blur. You can adjust the blur strength using the slider.
NAVIGATION: YES
STEPS: [{"ui_element": "Filters menu", "action": "Click", "description": "Open Filters menu", "delay_ms": 500}, {"ui_element": "Box Blur button", "action": "Click", "description": "Select Box Blur", "delay_ms": 300}]

Query: "where is brightness"
TEXT: The Brightness adjustment is in the Adjust menu at the top of the screen.
NAVIGATION: YES
STEPS: [{"ui_element": "Adjust menu", "action": "Click", "description": "Open Adjust menu", "delay_ms": 500}, {"ui_element": "Brightness button", "action": "Click", "description": "Select Brightness", "delay_ms": 300}]

Query: "how to flip image"
TEXT: You can flip your image using the Transform menu. Choose Flip Horizontal or Flip Vertical.
NAVIGATION: YES
STEPS: [{"ui_element": "Transform menu", "action": "Click", "description": "Open Transform menu", "delay_ms": 500}, {"ui_element": "Flip Horizontal button", "action": "Click", "description": "Flip horizontally", "delay_ms": 300}]

Query: "what blend modes are available"
TEXT: Grid has 24 blend modes including Normal, Multiply, Screen, Overlay, Soft Light, Hard Light, Color Dodge, Color Burn, Difference, Exclusion, Hue, Saturation, Color, and Luminosity. Select a layer and use the Blend Mode dropdown in the Layers panel.
NAVIGATION: NO
STEPS: none

User Query: )" + user_query + R"(

)";
}

// ============================================================================
// Response Extraction Helpers
// ============================================================================

std::string AINavigator::extractTextSection(const std::string& llm_output) const
{
    // Find "TEXT: " and extract until "NAVIGATION:"
    size_t text_start = llm_output.find("TEXT:");
    if (text_start == std::string::npos) {
        text_start = llm_output.find("Text:");
    }
    
    size_t text_end = llm_output.find("NAVIGATION:");
    if (text_end == std::string::npos) {
        text_end = llm_output.find("Navigation:");
    }
    
    if (text_start != std::string::npos) {
        text_start += 5; // Skip "TEXT:"
        // Skip any leading whitespace
        while (text_start < llm_output.size() && 
               (llm_output[text_start] == ' ' || llm_output[text_start] == '\t')) {
            text_start++;
        }
        
        if (text_end != std::string::npos && text_end > text_start) {
            std::string result = llm_output.substr(text_start, text_end - text_start);
            // Trim trailing whitespace
            size_t end = result.find_last_not_of(" \t\n\r");
            if (end != std::string::npos) {
                result = result.substr(0, end + 1);
            }
            return result;
        }
    }
    
    // Fallback: return entire output if format not found, cleaned up
    std::string result = llm_output;
    size_t start = result.find_first_not_of(" \t\n\r");
    size_t end = result.find_last_not_of(" \t\n\r");
    if (start != std::string::npos && end != std::string::npos) {
        return result.substr(start, end - start + 1);
    }
    return result;
}

bool AINavigator::extractNavigationFlag(const std::string& llm_output) const
{
    size_t nav_pos = llm_output.find("NAVIGATION:");
    if (nav_pos == std::string::npos) {
        nav_pos = llm_output.find("Navigation:");
    }
    
    if (nav_pos != std::string::npos) {
        // Get the value after "NAVIGATION:"
        size_t value_start = nav_pos + 11; // Length of "NAVIGATION:"
        size_t value_end = llm_output.find('\n', value_start);
        if (value_end == std::string::npos) {
            value_end = llm_output.length();
        }
        
        std::string nav_value = llm_output.substr(value_start, value_end - value_start);
        
        // Check for YES (case insensitive)
        for (char& c : nav_value) c = std::toupper(static_cast<unsigned char>(c));
        return (nav_value.find("YES") != std::string::npos);
    }
    return false;
}

std::string AINavigator::extractStepsSection(const std::string& llm_output) const
{
    size_t steps_start = llm_output.find("STEPS:");
    if (steps_start == std::string::npos) {
        steps_start = llm_output.find("Steps:");
    }
    
    if (steps_start != std::string::npos) {
        steps_start += 6; // Skip "STEPS:"
        
        // Skip whitespace
        while (steps_start < llm_output.size() && 
               (llm_output[steps_start] == ' ' || llm_output[steps_start] == '\t')) {
            steps_start++;
        }
        
        // Check if it starts with "none"
        if (llm_output.substr(steps_start, 4) == "none") {
            return "none";
        }
        
        // Find the JSON array
        size_t json_start = llm_output.find('[', steps_start);
        size_t json_end = llm_output.rfind(']');
        
        if (json_start != std::string::npos && json_end != std::string::npos && json_end > json_start) {
            return llm_output.substr(json_start, json_end - json_start + 1);
        }
    }
    return "none";
}

std::vector<NavigationStep> AINavigator::parseJSONSteps(const std::string& json_str)
{
    std::vector<NavigationStep> steps;
    
    if (json_str.empty() || json_str == "none") {
        return steps;
    }
    
    try {
        json j = json::parse(json_str);
        
        // Expecting a JSON array directly
        if (!j.is_array()) {
            std::cerr << "[AINavigator] Expected JSON array for steps\n";
            return steps;
        }
        
        for (const auto& step_json : j) {
            NavigationStep step;
            
            if (step_json.contains("ui_element") && step_json["ui_element"].is_string()) {
                step.ui_element = step_json["ui_element"].get<std::string>();
            }
            
            if (step_json.contains("action") && step_json["action"].is_string()) {
                step.action = step_json["action"].get<std::string>();
            }
            
            if (step_json.contains("description") && step_json["description"].is_string()) {
                step.description = step_json["description"].get<std::string>();
            }
            
            if (step_json.contains("delay_ms") && step_json["delay_ms"].is_number()) {
                step.delay_ms = step_json["delay_ms"].get<int>();
            } else {
                step.delay_ms = 300; // Default
            }
            
            // Only add steps with at least ui_element
            if (!step.ui_element.empty()) {
                steps.push_back(step);
            }
        }
        
    } catch (const json::parse_error& e) {
        std::cerr << "[AINavigator] JSON parse error in parseJSONSteps: " << e.what() << "\n";
    } catch (const json::exception& e) {
        std::cerr << "[AINavigator] JSON error in parseJSONSteps: " << e.what() << "\n";
    }
    
    return steps;
}

// ============================================================================
// LLM Query
// ============================================================================

std::string AINavigator::queryLLM(const std::string& prompt)
{
    if (!m_initialized || !m_ctx || !m_model || !m_vocab) {
        return "";
    }

    // Tokenize the prompt
    int n_prompt_tokens = prompt.length() + 32;  // estimate
    std::vector<llama_token> tokens(n_prompt_tokens);
    
    int n_tokens = llama_tokenize(
        m_vocab,
        prompt.c_str(),
        prompt.length(),
        tokens.data(),
        n_prompt_tokens,
        true,   // add special tokens (BOS)
        true    // parse special tokens
    );
    
    if (n_tokens < 0) {
        // Need more space
        tokens.resize(-n_tokens);
        n_tokens = llama_tokenize(
            m_vocab,
            prompt.c_str(),
            prompt.length(),
            tokens.data(),
            tokens.size(),
            true,
            true
        );
    }
    
    if (n_tokens <= 0) {
        std::cerr << "[AINavigator] Tokenization failed\n";
        return "";
    }
    
    tokens.resize(n_tokens);

    // Clear KV cache for a fresh start
    llama_memory_t mem = llama_get_memory(m_ctx);
    if (mem) {
        llama_memory_clear(mem, true);
    }

    // Create batch for prompt processing
    llama_batch batch = llama_batch_get_one(tokens.data(), tokens.size());
    
    // Process the prompt
    if (llama_decode(m_ctx, batch) != 0) {
        std::cerr << "[AINavigator] Failed to decode prompt\n";
        return "";
    }

    // Generate response tokens
    std::string response;
    int n_generated = 0;
    
    while (n_generated < MAX_TOKENS) {
        // Sample next token
        llama_token new_token = llama_sampler_sample(m_sampler, m_ctx, -1);
        
        // Check for end of generation
        if (llama_vocab_is_eog(m_vocab, new_token)) {
            break;
        }
        
        // Convert token to text
        char buf[256];
        int n = llama_token_to_piece(m_vocab, new_token, buf, sizeof(buf), 0, true);
        if (n > 0) {
            response.append(buf, n);
            // If we've generated enough and find a closing brace that completes the JSON, we can stop
            if (response.find('}') != std::string::npos && response.length() > 20) {
                // Potential optimization: check if JSON is balanced
            }
        }
        
        // Prepare next token for decoding
        llama_batch next_batch = llama_batch_get_one(&new_token, 1);
        if (llama_decode(m_ctx, next_batch) != 0) {
            std::cerr << "[AINavigator] Failed during generation\n";
            break;
        }
        
        n_generated++;
    }
    
    std::cout << "[AINavigator] Raw Response: " << response << std::endl;
    return response;
}

// ============================================================================
// JSON Parsing
// ============================================================================

std::vector<NavigationStep> AINavigator::parseJSONResponse(const std::string& json_str)
{
    std::vector<NavigationStep> steps;
    
    if (json_str.empty()) {
        return steps;
    }

    // Prepend the '{' we forced in the prompt
    std::string full_json = "{" + json_str;

    // Find JSON object boundaries
    size_t json_start = full_json.find('{');
    size_t json_end = full_json.rfind('}');
    
    if (json_start == std::string::npos || json_end == std::string::npos || json_end <= json_start) {
        std::cerr << "[AINavigator] No valid JSON object found in response\n";
        return steps;
    }
    
    std::string clean_json = full_json.substr(json_start, json_end - json_start + 1);

    try {
        json j = json::parse(clean_json);
        
        // Check if "steps" array exists
        if (!j.contains("steps") || !j["steps"].is_array()) {
            std::cerr << "[AINavigator] JSON missing 'steps' array\n";
            return steps;
        }
        
        // Parse each step
        for (const auto& step_json : j["steps"]) {
            NavigationStep step;
            
            if (step_json.contains("ui_element") && step_json["ui_element"].is_string()) {
                step.ui_element = step_json["ui_element"].get<std::string>();
            }
            
            if (step_json.contains("action") && step_json["action"].is_string()) {
                step.action = step_json["action"].get<std::string>();
            }
            
            if (step_json.contains("description") && step_json["description"].is_string()) {
                step.description = step_json["description"].get<std::string>();
            }
            
            if (step_json.contains("delay_ms") && step_json["delay_ms"].is_number()) {
                step.delay_ms = step_json["delay_ms"].get<int>();
            }
            
            // Only add steps with at least a description
            if (!step.description.empty()) {
                steps.push_back(step);
            }
        }
        
    } catch (const json::parse_error& e) {
        std::cerr << "[AINavigator] JSON parse error: " << e.what() << "\n";
    } catch (const json::exception& e) {
        std::cerr << "[AINavigator] JSON error: " << e.what() << "\n";
    }
    
    return steps;
}
