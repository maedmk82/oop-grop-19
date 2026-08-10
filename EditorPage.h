#ifndef EDITORPAGE_H
#define EDITORPAGE_H
#include <vector>
#include <sstream>
#include <SDL3/SDL.h>
#include <vector>
#include <memory>
#include <cmath>
#include <string>
#include <algorithm> // برای توابع جستجو

#include "EditorMenu.h"
#include "SearchBox.h"
#include "Component.h"

// ساختار برای نگهداری اطلاعات هر ابزار/قطعه در نوار کناری
struct ToolItem {
    std::string name;
    ComponentType type;
};

class EditorPage
{
private:
    EditorMenu menu;
    SearchBox search;

    std::vector<std::unique_ptr<Component>> components;

    // --- لیست ابزارها ---
    std::vector<ToolItem> allTools;       // کل قطعات موجود
    std::vector<ToolItem> filteredTools;  // قطعاتی که پس از جستجو نمایش داده می‌شوند

    ComponentType selectedTool = ComponentType::RESISTOR;
    bool isPlacingMode = false;
    int currentMouseX = 0;
    int currentMouseY = 0;

    void DrawGrid(SDL_Renderer* renderer);
    void DrawSidebar(SDL_Renderer* renderer);
    void UpdateSearchFilter(); // تابع جدید برای فیلتر کردن قطعات

public:
    EditorPage(SDL_Window* window);
    std::string pageSize = "A4"; // متغیر جدید برای نگهداری سایز صفحه
    void ClearWorkspace();
    void SaveWorkspace(const std::string& filepath);
    void LoadWorkspace(const std::string& filepath);
    void Draw(SDL_Renderer* renderer);
    bool exportRequested = false;
    void ExportToImage(SDL_Renderer* renderer);
    EditorMenuAction HandleClick(int x, int y);
    void HandleMouseMotion(int x, int y);
    void HandleKeyboard(SDL_Event event);
    std::vector<std::string> undoStack;
    std::vector<std::string> redoStack;

    std::string SaveStateToString();
    void LoadStateFromString(const std::string& state);

    void SaveCurrentStateForUndo();
    void Undo();
    void Redo();
};

#endif
