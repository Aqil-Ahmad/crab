#pragma once
#include <imgui.h>

/**
 * Modern UI Color Palette
 * 
 * High contrast, professional colors inspired by modern apps like VS Code and Claude.
 * Dark theme with excellent readability.
 */

namespace UIColors {
    // ==========================================================================
    // Background Colors (Dark Theme)
    // ==========================================================================
    inline ImVec4 WindowBg()        { return ImVec4(0.11f, 0.11f, 0.12f, 1.00f); }  // #1C1C1E
    inline ImVec4 ChildBg()         { return ImVec4(0.13f, 0.13f, 0.14f, 1.00f); }  // #212123
    inline ImVec4 PopupBg()         { return ImVec4(0.15f, 0.15f, 0.16f, 0.98f); }  // #262628
    inline ImVec4 Border()          { return ImVec4(0.25f, 0.25f, 0.26f, 1.00f); }  // #404042
    inline ImVec4 BorderShadow()    { return ImVec4(0.00f, 0.00f, 0.00f, 0.30f); }
    
    // ==========================================================================
    // Text Colors (High Contrast)
    // ==========================================================================
    inline ImVec4 Text()            { return ImVec4(0.95f, 0.95f, 0.96f, 1.00f); }  // #F2F2F5
    inline ImVec4 TextDim()         { return ImVec4(0.65f, 0.65f, 0.67f, 1.00f); }  // #A6A6AB
    inline ImVec4 TextDisabled()    { return ImVec4(0.45f, 0.45f, 0.47f, 1.00f); }  // #737378
    
    // ==========================================================================
    // Accent Colors
    // ==========================================================================
    inline ImVec4 Primary()         { return ImVec4(0.38f, 0.62f, 1.00f, 1.00f); }  // #6199FF Blue
    inline ImVec4 PrimaryHover()    { return ImVec4(0.48f, 0.68f, 1.00f, 1.00f); }
    inline ImVec4 PrimaryActive()   { return ImVec4(0.28f, 0.52f, 0.90f, 1.00f); }
    
    inline ImVec4 Success()         { return ImVec4(0.30f, 0.85f, 0.45f, 1.00f); }  // #4DD972 Green
    inline ImVec4 Warning()         { return ImVec4(1.00f, 0.75f, 0.20f, 1.00f); }  // #FFC033 Orange
    inline ImVec4 Danger()          { return ImVec4(1.00f, 0.35f, 0.35f, 1.00f); }  // #FF5959 Red
    
    // ==========================================================================
    // UI Element Colors
    // ==========================================================================
    inline ImVec4 Button()          { return ImVec4(0.20f, 0.22f, 0.24f, 1.00f); }  // #333840
    inline ImVec4 ButtonHover()     { return ImVec4(0.28f, 0.30f, 0.33f, 1.00f); }  // #474D54
    inline ImVec4 ButtonActive()    { return ImVec4(0.35f, 0.38f, 0.42f, 1.00f); }  // #59616B
    
    inline ImVec4 Header()          { return ImVec4(0.14f, 0.14f, 0.15f, 1.00f); }  // #242426
    inline ImVec4 HeaderHover()     { return ImVec4(0.20f, 0.20f, 0.21f, 1.00f); }  // #333335
    inline ImVec4 HeaderActive()    { return ImVec4(0.24f, 0.24f, 0.25f, 1.00f); }  // #3D3D40
    
    inline ImVec4 FrameBg()         { return ImVec4(0.16f, 0.17f, 0.18f, 1.00f); }  // #292A2D
    inline ImVec4 FrameBgHover()    { return ImVec4(0.22f, 0.23f, 0.24f, 1.00f); }  // #383A3D
    inline ImVec4 FrameBgActive()   { return ImVec4(0.28f, 0.29f, 0.30f, 1.00f); }  // #474A4D
    
    inline ImVec4 Tab()             { return ImVec4(0.14f, 0.14f, 0.15f, 1.00f); }
    inline ImVec4 TabHover()        { return ImVec4(0.20f, 0.20f, 0.21f, 1.00f); }
    inline ImVec4 TabActive()       { return ImVec4(0.24f, 0.24f, 0.25f, 1.00f); }
    
    // ==========================================================================
    // Slider & Scrollbar
    // ==========================================================================
    inline ImVec4 SliderGrab()      { return Primary(); }
    inline ImVec4 SliderGrabActive(){ return PrimaryActive(); }
    
    inline ImVec4 Scrollbar()       { return ImVec4(0.08f, 0.08f, 0.09f, 1.00f); }
    inline ImVec4 ScrollbarGrab()   { return ImVec4(0.30f, 0.30f, 0.32f, 1.00f); }
    inline ImVec4 ScrollbarHover()  { return ImVec4(0.40f, 0.40f, 0.42f, 1.00f); }
    inline ImVec4 ScrollbarActive() { return ImVec4(0.50f, 0.50f, 0.52f, 1.00f); }
    
    // ==========================================================================
    // Title Bar
    // ==========================================================================
    inline ImVec4 TitleBg()         { return ImVec4(0.10f, 0.10f, 0.11f, 1.00f); }
    inline ImVec4 TitleBgActive()   { return ImVec4(0.14f, 0.14f, 0.15f, 1.00f); }
}
