#include "EditorPage.h"
#include "TextRenderer.h"
// در صورتی که TextRenderer دارید آن را اینکلود کنید تا نام قطعات روی دکمه‌ها نوشته شود
// #include "TextRenderer.h"

EditorPage::EditorPage(SDL_Window* window)
    : search(window)
{
    // لیست کامل قطعات با نام‌های استاندارد (حتی کلمات مخفف برای سرچ بهتر)
    allTools = {
        {"GND (Ground)", ComponentType::GND},
        {"DC Voltage", ComponentType::DC_SOURCE},
        {"Battery", ComponentType::BATTERY},
        {"Logic State (0/1)", ComponentType::LOGIC_STATE},
        {"Pulse Clock", ComponentType::CLOCK},
        {"RES (Resistor)", ComponentType::RESISTOR},
        {"CAP (Capacitor)", ComponentType::CAPACITOR},
        {"Push Button", ComponentType::PUSH_BUTTON},
        {"Switch", ComponentType::SWITCH},
        {"LED", ComponentType::LED},
        {"7Seg Display", ComponentType::SEVEN_SEGMENT},
        {"AND Gate", ComponentType::GATE_AND},
        {"OR Gate", ComponentType::GATE_OR},
        {"NOT Gate", ComponentType::GATE_NOT},
        {"NAND Gate", ComponentType::GATE_NAND},
        {"NOR Gate", ComponentType::GATE_NOR}, // مطمئن شوید GATE_NOR در Enum شما باشد
        {"XOR Gate", ComponentType::GATE_XOR},
        {"FlipFlop D", ComponentType::FLIP_FLOP_D}
    };

    filteredTools = allTools;
}

//-----------------------------------------
// سیستم جستجو
//-----------------------------------------
void EditorPage::UpdateSearchFilter()
{
    std::string query = search.GetText(); // گرفتن متن از SearchBox

    // تبدیل حروف جستجو به کوچک (برای سرچ Case-Insensitive)
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);

    filteredTools.clear();

    for (const auto& tool : allTools) {
        std::string toolName = tool.name;
        std::transform(toolName.begin(), toolName.end(), toolName.begin(), ::tolower);

        // اگر جستجو خالی است یا نام قطعه شامل کلمه جستجو شده است، آن را به لیست اضافه کن
        if (query.empty() || toolName.find(query) != std::string::npos) {
            filteredTools.push_back(tool);
        }
    }
}

//-----------------------------------------
// Draw Helpers
//-----------------------------------------
void EditorPage::DrawGrid(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 215, 215, 215, 255);
    int gridSize = 20;

    for (float x = 100; x < 950; x += gridSize) {
        SDL_RenderLine(renderer, x, 50, x, 600);
    }
    for (float y = 50; y < 600; y += gridSize) {
        SDL_RenderLine(renderer, 100, y, 950, y);
    }
}

void EditorPage::DrawSidebar(SDL_Renderer* renderer)
{
    // ۱. پس‌زمینه نوار ابزار سمت چپ
    SDL_FRect sidebarArea = { 0, 50, 100, 550 };
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderFillRect(renderer, &sidebarArea);

    // ۲. رسم دکمه‌ها بر اساس لیست فیلتر شده (filteredTools)
    int startY = 70;
    for (size_t i = 0; i < filteredTools.size(); i++) {
        SDL_FRect btnRect = { 10, (float)(startY + i * 40), 80, 30 };

        // تغییر رنگ دکمه در صورت انتخاب شدن قطعه (آبی فیروزه‌ای)
        if (isPlacingMode && selectedTool == filteredTools[i].type) {
            SDL_SetRenderDrawColor(renderer, 140, 240, 240, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        }

        // رسم پس‌زمینه دکمه
        SDL_RenderFillRect(renderer, &btnRect);

        // رسم حاشیه دکمه
        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderRect(renderer, &btnRect);

        // ۳. رسم نام قطعه داخل دکمه
        SDL_Color textColor = {0, 0, 0, 255}; // رنگ مشکی برای متن
        SDL_Texture* txt = TextRenderer::CreateText(renderer, filteredTools[i].name.c_str(), textColor);

        if (txt) {
            float texW = 0, texH = 0;
            SDL_GetTextureSize(txt, &texW, &texH);

            // جلوگیری از بیرون زدن متن از کادر دکمه (اگر اسم قطعه طولانی بود)
            if (texW > 75) texW = 75;
            if (texH > 20) texH = 20;

            // محاسبه موقعیت متن برای قرار گرفتن در وسط نسبی دکمه
            SDL_FRect txtPos = {
                12,                                 // فاصله از چپ
                (float)(startY + i * 40 + 5),       // فاصله از بالا
                texW,
                texH
            };

            SDL_RenderTexture(renderer, txt, NULL, &txtPos);
            SDL_DestroyTexture(txt);
        }
    }
}

//-----------------------------------------
// Draw
//-----------------------------------------
void EditorPage::Draw(SDL_Renderer* renderer)
{
    // همیشه فیلتر را قبل از رسم آپدیت کن تا با تایپ کاربر لیست سریع تغییر کند
    UpdateSearchFilter();

    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    SDL_FRect area = { 0, 0, 950, 600 };
    SDL_RenderFillRect(renderer, &area);

    DrawGrid(renderer);

    for (auto& comp : components) {
        comp->Update();
        comp->Draw(renderer);
    }

    // رسم سایه (Ghosting) قطعه هنگام قرار دادن
    if (isPlacingMode && currentMouseX >= 100 && currentMouseY >= 50) {
        float snapX = std::round(currentMouseX / 20.0f) * 20.0f;
        float snapY = std::round(currentMouseY / 20.0f) * 20.0f;

        SDL_SetRenderDrawColor(renderer, 0, 200, 0, 120);
        SDL_FRect ghostRect = { snapX, snapY, 60, 40 }; // سایز تقریبی قطعه
        SDL_RenderFillRect(renderer, &ghostRect);
    }

    DrawSidebar(renderer);

    menu.Draw(renderer);
    search.Draw(renderer);
}

//-----------------------------------------
// Mouse & Keyboard
//-----------------------------------------
// =====================================
// Mouse Actions (Handle Click)
// =====================================
EditorMenuAction EditorPage::HandleClick(int x, int y)
{
    // بررسی کلیک روی منوی بالا و سرچ باکس
    EditorMenuAction action = menu.HandleClick(x, y);
    search.HandleClick(x, y);

    // ۱. بررسی کلیک روی نوار ابزار (Sidebar) برای انتخاب قطعه از لیست پویا
    if (x < 100 && y > 50) {
        int index = (y - 70) / 40;
        if (index >= 0 && index < (int)filteredTools.size()) {
            int buttonY = 70 + index * 40;
            if (y >= buttonY && y <= buttonY + 30) {
                selectedTool = filteredTools[index].type;
                isPlacingMode = true;
            }
        }
        return action;
    }

    // ۲. قرار دادن قطعه روی شبکه با کلیک روی صفحه (بخش کامل شده)
    if (isPlacingMode && x >= 100 && y >= 50) {
        // محاسبه مختصات برای Snap to Grid (چسبیدن به خطوط شبکه)
        float snapX = std::round(x / 20.0f) * 20.0f;
        float snapY = std::round(y / 20.0f) * 20.0f;

        // --- منابع اصلی ---
        if (selectedTool == ComponentType::GND)
            components.push_back(std::make_unique<GNDComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::DC_SOURCE)
            components.push_back(std::make_unique<DCSourceComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::BATTERY)
            components.push_back(std::make_unique<BatteryComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::CLOCK)
            components.push_back(std::make_unique<ClockComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::LOGIC_STATE) // <--- اضافه کردن این خط
            components.push_back(std::make_unique<LogicStateComponent>(snapX, snapY));
        // --- قطعات غیرفعال ---
        else if (selectedTool == ComponentType::RESISTOR)
            components.push_back(std::make_unique<ResistorComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::CAPACITOR)
            components.push_back(std::make_unique<CapacitorComponent>(snapX, snapY));

        // --- تعاملی و خروجی ---
        else if (selectedTool == ComponentType::SWITCH)
            components.push_back(std::make_unique<SwitchComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::PUSH_BUTTON)
            components.push_back(std::make_unique<PushButtonComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::LED)
            components.push_back(std::make_unique<LEDComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::SEVEN_SEGMENT)
            components.push_back(std::make_unique<SevenSegmentComponent>(snapX, snapY));

        // --- گیت‌های منطقی و فلیپ‌فلاپ ---
        else if (selectedTool == ComponentType::GATE_AND)
            components.push_back(std::make_unique<GateANDComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::GATE_OR)
            components.push_back(std::make_unique<GateORComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::GATE_NOT)
            components.push_back(std::make_unique<GateNOTComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::GATE_XOR)
            components.push_back(std::make_unique<GateXORComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::GATE_NAND)
            components.push_back(std::make_unique<GateNANDComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::GATE_NOR)
            components.push_back(std::make_unique<GateNORComponent>(snapX, snapY));
        else if (selectedTool == ComponentType::FLIP_FLOP_D)
            components.push_back(std::make_unique<FlipFlopDComponent>(snapX, snapY));

        // خروج از حالت جای‌گذاری پس از قرار دادن قطعه
        isPlacingMode = false;
        return action;
    }

    // ۳. تعامل با کلیدها و قطعاتِ قرار داده شده روی صفحه
    if (!isPlacingMode) {
        for (auto& comp : components) {
            if (comp->HandleClick((float)x, (float)y)) {
                break;
            }
        }
    }

    return action;
}

void EditorPage::HandleMouseMotion(int x, int y)
{
    currentMouseX = x;
    currentMouseY = y;
}

void EditorPage::HandleKeyboard(SDL_Event event)
{
    search.HandleKeyboard(event);

    // انصراف از جای‌گذاری قطعه با دکمه ESC
    if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE) {
        isPlacingMode = false;
    }
}
void EditorPage::ClearWorkspace()
{
    // پاک کردن تمام قطعات از روی صفحه
    components.clear();

    // خروج از حالت جای‌گذاری قطعه (اگر دست کاربر ابزاری بوده لغو شود)
    isPlacingMode = false;
}
