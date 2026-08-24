#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include "moduleManager/moduleBase.h"

class Frame {
public:
    Frame(std::string name, int index);
    
    void draw(int mouseX, int mouseY);
    void mouseClicked(int mouseX, int mouseY, int button);
    void mouseReleased(int mouseX, int mouseY, int button);
    void mouseDragged(int mouseX, int mouseY);
    
    void addModule(ModuleBase* module);
    
    // Position
    float x;
    float y;
    float width = 100.0f;
    float height = 0.0f;
    
    // Dragging state
    bool isDragging = false;
    float dragOffsetX = 0.0f;
    float dragOffsetY = 0.0f;
    
    // Header
    float headerHeight = 15.0f;
    
    std::string name;
    std::vector<ModuleBase*> modules;
    
    // Module expansion state (per module)
    static std::unordered_map<std::string, bool> moduleExpanded;
    
private:
    int index;
};
