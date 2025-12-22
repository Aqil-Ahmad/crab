#pragma once

/**
 * UI Configuration - Centralized UI scaling constants
 * 
 * All sizes are in pixels. Modify these values to adjust
 * the overall UI scale of the application.
 */

namespace UIConfig {
    // ==========================================================================
    // Font Sizes
    // ==========================================================================
    constexpr float FONT_SIZE_DEFAULT = 18.0f;     // Default UI text
    constexpr float FONT_SIZE_HEADING = 20.0f;     // Section headings
    constexpr float FONT_SIZE_SMALL = 14.0f;       // Small labels/hints
    
    // ==========================================================================
    // Button Dimensions
    // ==========================================================================
    constexpr float BUTTON_HEIGHT = 32.0f;         // Standard button height
    constexpr float BUTTON_WIDTH_SMALL = 80.0f;    // Small buttons (OK, Cancel)
    constexpr float BUTTON_WIDTH_MEDIUM = 120.0f;  // Medium buttons
    constexpr float BUTTON_WIDTH_LARGE = 160.0f;   // Large buttons
    
    // ==========================================================================
    // Icon Sizes
    // ==========================================================================
    constexpr float ICON_SIZE_TOOL = 32.0f;        // Tool panel icons
    constexpr float ICON_SIZE_SMALL = 20.0f;       // Small inline icons
    
    // ==========================================================================
    // Spacing and Padding
    // ==========================================================================
    constexpr float WINDOW_PADDING = 10.0f;        // Inside window padding
    constexpr float FRAME_PADDING_X = 8.0f;        // Button/input horizontal padding
    constexpr float FRAME_PADDING_Y = 6.0f;        // Button/input vertical padding
    constexpr float ITEM_SPACING = 6.0f;           // Space between items
    constexpr float ITEM_INNER_SPACING = 6.0f;     // Space inside compound items
    
    // ==========================================================================
    // Panel Dimensions
    // ==========================================================================
    constexpr float PANEL_MIN_WIDTH = 250.0f;      // Minimum panel width
    constexpr float SCROLLBAR_SIZE = 16.0f;        // Scrollbar width
    
    // ==========================================================================
    // Rounding
    // ==========================================================================
    constexpr float FRAME_ROUNDING = 4.0f;         // Button/input corner rounding
    constexpr float WINDOW_ROUNDING = 6.0f;        // Window corner rounding
    constexpr float SCROLLBAR_ROUNDING = 4.0f;     // Scrollbar corner rounding
}
