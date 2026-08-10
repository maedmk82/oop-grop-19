#include "EditorPage.h"
#include "TextRenderer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
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

    // ------------------------------------
    // رسم صفحه شطرنجی (Grid) با ابعاد پویا (A3 یا A4)
    // ------------------------------------
    bool isA3 = (pageSize.find("A3") != std::string::npos);

    float gridMaxX = isA3 ? 1350.0f : 950.0f;
    float gridMaxY = isA3 ? 800.0f : 600.0f;

    // پس‌زمینه کل پنجره بر اساس سایز جدید
    SDL_SetRenderDrawColor(renderer, 245, 245, 245, 255);
    SDL_FRect area = { 0, 0, gridMaxX, gridMaxY };
    SDL_RenderFillRect(renderer, &area);

    // ۱. رسم پس‌زمینه سفید برای ناحیه کاغذ
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_FRect paperRect = { 100, 50, gridMaxX - 100, gridMaxY - 50 };
    SDL_RenderFillRect(renderer, &paperRect);

    // ۲. رسم خطوط شبکه شطرنجی
    SDL_SetRenderDrawColor(renderer, 225, 225, 225, 255);
    for(float x = 100; x <= gridMaxX; x += 20) {
        SDL_RenderLine(renderer, x, 50, x, gridMaxY);
    }
    for(float y = 50; y <= gridMaxY; y += 20) {
        SDL_RenderLine(renderer, 100, y, gridMaxX, y);
    }

    // =========================================================
    // حلقه رسم قطعات
    // =========================================================
    for (auto& comp : components) {
        comp->Update();
        comp->Draw(renderer);

        // تست خطایابی: رسم یک مربع قرمز پشت هر قطعه برای اطمینان
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_FRect debugBox = { comp->x, comp->y, 20, 20 };
        if (comp->isSelected) {
            SDL_SetRenderDrawColor(renderer, 0, 150, 255, 80);
            SDL_FRect selRect = { comp->x - 5, comp->y - 5, 50, 50 };
            SDL_RenderFillRect(renderer, &selRect);
            SDL_SetRenderDrawColor(renderer, 0, 50, 255, 255);
            SDL_RenderRect(renderer, &selRect); // حاشیه آبی تیره
        }

        // نام تابع در SDL3 اصلاح شد
        SDL_RenderRect(renderer, &debugBox);
    }
    // رسم مستطیل انتخاب گروهی
    // رسم مستطیل انتخاب گروهی
    if (isSelectingBox) {
        SDL_SetRenderDrawColor(renderer, 0, 150, 255, 50); // آبی خیلی شفاف

        // اضافه کردن (float) قبل از currentMouseX و currentMouseY
        float rx = std::min(selStartX, (float)currentMouseX);
        float ry = std::min(selStartY, (float)currentMouseY);
        float rw = std::abs(selStartX - (float)currentMouseX);
        float rh = std::abs(selStartY - (float)currentMouseY);

        SDL_FRect selBox = { rx, ry, rw, rh };
        SDL_RenderFillRect(renderer, &selBox);
        SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
        SDL_RenderRect(renderer, &selBox);
    }
    // =========================================================
    // =========================================================

    // رسم سایه (Ghosting) قطعه هنگام قرار دادن
    if (isPlacingMode && currentMouseX >= 100 && currentMouseY >= 50) {
        float snapX = std::round(currentMouseX / 20.0f) * 20.0f;
        float snapY = std::round(currentMouseY / 20.0f) * 20.0f;

        SDL_SetRenderDrawColor(renderer, 0, 200, 0, 120);
        SDL_FRect ghostRect = { snapX, snapY, 60, 40 };
        SDL_RenderFillRect(renderer, &ghostRect);
    }

    DrawSidebar(renderer);

    menu.Draw(renderer);
    search.Draw(renderer);

    // =======================================================
    // رسم دکمه‌های نوار ابزار در بالای صفحه (از چپ به راست)
    // =======================================================
    SDL_Color black = {0, 0, 0, 255};

    // ۱. دکمه Rotate (چرخش) - شروع از مختصات امن 450
    SDL_SetRenderDrawColor(renderer, 200, 220, 255, 255);
    SDL_FRect rotBtn = { 450, 10, 60, 30 };
    SDL_RenderFillRect(renderer, &rotBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &rotBtn);
    SDL_Texture* tRot = TextRenderer::CreateText(renderer, "Rot 90", black);
    if(tRot) { SDL_FRect p = {455, 15, 50, 20}; SDL_RenderTexture(renderer, tRot, NULL, &p); SDL_DestroyTexture(tRot); }

    // ۲. دکمه Mirror (قرینه)
    SDL_SetRenderDrawColor(renderer, 255, 220, 200, 255);
    SDL_FRect mirBtn = { 520, 10, 60, 30 };
    SDL_RenderFillRect(renderer, &mirBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &mirBtn);
    SDL_Texture* tMir = TextRenderer::CreateText(renderer, "Mirror", black);
    if(tMir) { SDL_FRect p = {525, 15, 50, 20}; SDL_RenderTexture(renderer, tMir, NULL, &p); SDL_DestroyTexture(tMir); }

    // ۳. دکمه Delete (حذف)
    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
    SDL_FRect delBtn = { 590, 10, 60, 30 };
    SDL_RenderFillRect(renderer, &delBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &delBtn);
    SDL_Texture* tDel = TextRenderer::CreateText(renderer, "Delete", black);
    if(tDel) { SDL_FRect p = {595, 15, 50, 20}; SDL_RenderTexture(renderer, tDel, NULL, &p); SDL_DestroyTexture(tDel); }

    // ۴. دکمه Undo
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_FRect undoBtn = { 660, 10, 70, 30 };
    SDL_RenderFillRect(renderer, &undoBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &undoBtn);
    SDL_Texture* txtUndo = TextRenderer::CreateText(renderer, "< Undo", black);
    if (txtUndo) { SDL_FRect p = {665, 15, 60, 20}; SDL_RenderTexture(renderer, txtUndo, NULL, &p); SDL_DestroyTexture(txtUndo); }

    // ۵. دکمه Redo
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_FRect redoBtn = { 740, 10, 70, 30 };
    SDL_RenderFillRect(renderer, &redoBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &redoBtn);
    SDL_Texture* txtRedo = TextRenderer::CreateText(renderer, "Redo >", black);
    if (txtRedo) { SDL_FRect p = {745, 15, 60, 20}; SDL_RenderTexture(renderer, txtRedo, NULL, &p); SDL_DestroyTexture(txtRedo); }


    // ۶. دکمه Export Image
    SDL_SetRenderDrawColor(renderer, 200, 255, 200, 255);
    SDL_FRect exportBtn = { 820, 10, 110, 30 };
    SDL_RenderFillRect(renderer, &exportBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &exportBtn);
    SDL_Texture* txtExport = TextRenderer::CreateText(renderer, "Export Image", black);
    if (txtExport) { SDL_FRect p = {825, 15, 100, 20}; SDL_RenderTexture(renderer, txtExport, NULL, &p); SDL_DestroyTexture(txtExport); }

    // =======================================================
    // این بخش پاک شده بود! (باید حتماً در انتهای Draw باشد)
    // =======================================================
    if (exportRequested) {
        ExportToImage(renderer);
        exportRequested = false; // خاموش کردن پرچم تا صدها عکس پشت سر هم نگیرد
    }

} // <--- این آکولاد پایان تابع Draw است


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
    // =======================================================
    // بررسی کلیک روی نوار ابزار (Toolbar)
    // =======================================================

    // ۱. کلیک روی چرخش (Rotate)
    if (x >= 450 && x <= 510 && y >= 10 && y <= 40) {
        SaveCurrentStateForUndo();
        for (auto& c : components) if (c->isSelected) c->angle = (c->angle + 90) % 360;
        return (EditorMenuAction)0;
    }

    // ۲. کلیک روی قرینه (Mirror)
    if (x >= 520 && x <= 580 && y >= 10 && y <= 40) {
        SaveCurrentStateForUndo();
        for (auto& c : components) if (c->isSelected) c->isMirrored = !c->isMirrored;
        return (EditorMenuAction)0;
    }

    // ۳. کلیک روی حذف (Delete)
    if (x >= 590 && x <= 650 && y >= 10 && y <= 40) {
        SaveCurrentStateForUndo();
        components.erase(std::remove_if(components.begin(), components.end(),
            [](const std::unique_ptr<Component>& c) { return c->isSelected; }), components.end());
        return (EditorMenuAction)0;
    }

    // ۴. کلیک روی Undo
    if (x >= 660 && x <= 730 && y >= 10 && y <= 40) {
        Undo();
        return (EditorMenuAction)0;
    }

    // ۵. کلیک روی Redo
    if (x >= 740 && x <= 810 && y >= 10 && y <= 40) {
        Redo();
        return (EditorMenuAction)0;
    }

    // ۶. کلیک روی Export Image
    if (x >= 820 && x <= 930 && y >= 10 && y <= 40) {
        exportRequested = true;
        return (EditorMenuAction)0;
    }
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
    // =================================================================
    // بخش جای‌گذاری قطعات روی صفحه (مجهز به سیستم خطایابی)
    // =================================================================
    if (isPlacingMode && x >= 100 && y >= 50) {

        std::cout << "--- Trying to place component! ---" << std::endl;

        // محاسبه مختصات برای Snap to Grid
        float snapX = std::round(x / 20.0f) * 20.0f;
        float snapY = std::round(y / 20.0f) * 20.0f;

        // ذخیره وضعیت برای Undo
        SaveCurrentStateForUndo();

        bool placed = false; // پرچم بررسی موفقیت

        // --- منابع اصلی ---
        if (selectedTool == ComponentType::GND) {
            components.push_back(std::make_unique<GNDComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::DC_SOURCE) {
            components.push_back(std::make_unique<DCSourceComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::BATTERY) {
            components.push_back(std::make_unique<BatteryComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::CLOCK) {
            components.push_back(std::make_unique<ClockComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::LOGIC_STATE) {
            components.push_back(std::make_unique<LogicStateComponent>(snapX, snapY)); placed = true;
        }
        // --- قطعات غیرفعال ---
        else if (selectedTool == ComponentType::RESISTOR) {
            components.push_back(std::make_unique<ResistorComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::CAPACITOR) {
            components.push_back(std::make_unique<CapacitorComponent>(snapX, snapY)); placed = true;
        }
        // --- تعاملی و خروجی ---
        else if (selectedTool == ComponentType::SWITCH) {
            components.push_back(std::make_unique<SwitchComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::PUSH_BUTTON) {
            components.push_back(std::make_unique<PushButtonComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::LED) {
            components.push_back(std::make_unique<LEDComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::SEVEN_SEGMENT) {
            components.push_back(std::make_unique<SevenSegmentComponent>(snapX, snapY)); placed = true;
        }
        // --- گیت‌های منطقی و فلیپ‌فلاپ ---
        else if (selectedTool == ComponentType::GATE_AND) {
            components.push_back(std::make_unique<GateANDComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::GATE_OR) {
            components.push_back(std::make_unique<GateORComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::GATE_NOT) {
            components.push_back(std::make_unique<GateNOTComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::GATE_XOR) {
            components.push_back(std::make_unique<GateXORComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::GATE_NAND) {
            components.push_back(std::make_unique<GateNANDComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::GATE_NOR) {
            components.push_back(std::make_unique<GateNORComponent>(snapX, snapY)); placed = true;
        }
        else if (selectedTool == ComponentType::FLIP_FLOP_D) {
            components.push_back(std::make_unique<FlipFlopDComponent>(snapX, snapY)); placed = true;
        }

        // گزارش به کنسول که آیا قطعه رسم شد یا خیر
        if (placed) {
            std::cout << "SUCCESS: Component placed at X:" << snapX << " Y:" << snapY << std::endl;
        } else {
            std::cout << "ERROR: selectedTool did not match any component!" << std::endl;
        }

        // خروج از حالت جای‌گذاری پس از قرار دادن قطعه
        isPlacingMode = false;

        // برگرداندن صفر به معنی هیچ دستوری برای منوی اصلی نیست (جلوگیری از ارور کامپایل)
        return (EditorMenuAction)0;
    }

    // ۳. تعامل با کلیدها و قطعاتِ قرار داده شده روی صفحه
    if (!isPlacingMode) {
        for (auto& comp : components) {
            if (comp->HandleClick((float)x, (float)y)) {
                break;
            }
        }
    }
    // اگر در حالت قرار دادن قطعه جدید نیستیم:
    if (!isPlacingMode && x >= 100 && y >= 50) {
        bool clickedOnComponent = false;

        // چک میکنیم آیا روی قطعه‌ای کلیک شده؟ (از آخر به اول تا قطعه رویی انتخاب شود)
        for (auto it = components.rbegin(); it != components.rend(); ++it) {
            if ((*it)->Contains(x, y)) {
                clickedOnComponent = true;
                if (!(*it)->isSelected) { // اگر از قبل انتخاب نشده بود
                    for (auto& c : components) c->isSelected = false; // بقیه را از انتخاب در بیار
                    (*it)->isSelected = true; // این یکی را انتخاب کن
                }
                isDragging = true;
                lastMouseX = x;
                lastMouseY = y;
                break;
            }
        }

        // اگر روی فضای خالی کلیک شده بود، مستطیل انتخاب را شروع کن
        if (!clickedOnComponent) {
            for (auto& c : components) c->isSelected = false;
            isSelectingBox = true;
            selStartX = x;
            selStartY = y;
        }
    }

    return action;
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
//-----------------------------------------
// Save Workspace (ذخیره قطعات در فایل)
//-----------------------------------------
//-----------------------------------------
// Save Workspace (ذخیره قطعات در فایل)
//-----------------------------------------
void EditorPage::SaveWorkspace(const std::string& filepath)
{
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cout << "Error: Could not save file to " << filepath << std::endl;
        return;
    }

    // تغییر مهم: حالا زاویه و وضعیت قرینه هم در فایل متنی ذخیره می‌شوند
    for (const auto& comp : components) {
        file << (int)comp->type << " " << comp->x << " " << comp->y << " "
             << comp->angle << " " << comp->isMirrored << "\n";
    }

    file.close();
    std::cout << "Workspace saved successfully to: " << filepath << std::endl;
}

//-----------------------------------------
// Load Workspace (خواندن قطعات از فایل)
//-----------------------------------------
void EditorPage::LoadWorkspace(const std::string& filepath)
{
    ClearWorkspace(); // ابتدا صفحه را پاک کن

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cout << "Error: Could not open file " << filepath << std::endl;
        return;
    }

    int typeInt, angle;
    float x, y;
    bool isMirrored;

    // تغییر مهم: خواندن زاویه و قرینه علاوه بر مختصات
    while (file >> typeInt >> x >> y >> angle >> isMirrored)
    {
        ComponentType type = (ComponentType)typeInt;

        // بر اساس نوع قطعه خوانده شده، آن را می‌سازیم و در صفحه قرار می‌دهیم
        if (type == ComponentType::GND) components.push_back(std::make_unique<GNDComponent>(x, y));
        else if (type == ComponentType::DC_SOURCE) components.push_back(std::make_unique<DCSourceComponent>(x, y));
        else if (type == ComponentType::BATTERY) components.push_back(std::make_unique<BatteryComponent>(x, y));
        else if (type == ComponentType::LOGIC_STATE) components.push_back(std::make_unique<LogicStateComponent>(x, y));
        else if (type == ComponentType::CLOCK) components.push_back(std::make_unique<ClockComponent>(x, y));
        else if (type == ComponentType::RESISTOR) components.push_back(std::make_unique<ResistorComponent>(x, y));
        else if (type == ComponentType::CAPACITOR) components.push_back(std::make_unique<CapacitorComponent>(x, y));
        else if (type == ComponentType::PUSH_BUTTON) components.push_back(std::make_unique<PushButtonComponent>(x, y));
        else if (type == ComponentType::SWITCH) components.push_back(std::make_unique<SwitchComponent>(x, y));
        else if (type == ComponentType::LED) components.push_back(std::make_unique<LEDComponent>(x, y));
        else if (type == ComponentType::SEVEN_SEGMENT) components.push_back(std::make_unique<SevenSegmentComponent>(x, y));
        else if (type == ComponentType::GATE_AND) components.push_back(std::make_unique<GateANDComponent>(x, y));
        else if (type == ComponentType::GATE_OR) components.push_back(std::make_unique<GateORComponent>(x, y));
        else if (type == ComponentType::GATE_NOT) components.push_back(std::make_unique<GateNOTComponent>(x, y));
        else if (type == ComponentType::GATE_NAND) components.push_back(std::make_unique<GateNANDComponent>(x, y));
        else if (type == ComponentType::GATE_NOR) components.push_back(std::make_unique<GateNORComponent>(x, y));
        else if (type == ComponentType::GATE_XOR) components.push_back(std::make_unique<GateXORComponent>(x, y));
        else if (type == ComponentType::FLIP_FLOP_D) components.push_back(std::make_unique<FlipFlopDComponent>(x, y));

        // اعمال زاویه و قرینه به قطعه‌ای که همین الان ساخته شد
        if (!components.empty()) {
            components.back()->angle = angle;
            components.back()->isMirrored = isMirrored;
        }
    }

    file.close();
    std::cout << "Workspace loaded successfully from: " << filepath << std::endl;
}
// ========================================================
// منطق Undo و Redo
// ========================================================

// ========================================================
// منطق Undo و Redo
// ========================================================

std::string EditorPage::SaveStateToString()
{
    std::stringstream ss;
    // تغییر مهم: زاویه و قرینه به استرینگِ رم (حافظه موقت) هم اضافه شدند
    for (const auto& comp : components) {
        ss << (int)comp->type << " " << comp->x << " " << comp->y << " "
           << comp->angle << " " << comp->isMirrored << "\n";
    }
    return ss.str();
}

void EditorPage::LoadStateFromString(const std::string& state)
{
    ClearWorkspace(); // پاک کردن صفحه فعلی
    std::stringstream ss(state);

    int typeInt, angle;
    float x, y;
    bool isMirrored;

    // خواندن قطعات و زوایا از روی متن موجود در رم
    while (ss >> typeInt >> x >> y >> angle >> isMirrored)
    {
        ComponentType type = (ComponentType)typeInt;

        if (type == ComponentType::GND) components.push_back(std::make_unique<GNDComponent>(x, y));
        else if (type == ComponentType::DC_SOURCE) components.push_back(std::make_unique<DCSourceComponent>(x, y));
        else if (type == ComponentType::BATTERY) components.push_back(std::make_unique<BatteryComponent>(x, y));
        else if (type == ComponentType::LOGIC_STATE) components.push_back(std::make_unique<LogicStateComponent>(x, y));
        else if (type == ComponentType::CLOCK) components.push_back(std::make_unique<ClockComponent>(x, y));
        else if (type == ComponentType::RESISTOR) components.push_back(std::make_unique<ResistorComponent>(x, y));
        else if (type == ComponentType::CAPACITOR) components.push_back(std::make_unique<CapacitorComponent>(x, y));
        else if (type == ComponentType::PUSH_BUTTON) components.push_back(std::make_unique<PushButtonComponent>(x, y));
        else if (type == ComponentType::SWITCH) components.push_back(std::make_unique<SwitchComponent>(x, y));
        else if (type == ComponentType::LED) components.push_back(std::make_unique<LEDComponent>(x, y));
        else if (type == ComponentType::SEVEN_SEGMENT) components.push_back(std::make_unique<SevenSegmentComponent>(x, y));
        else if (type == ComponentType::GATE_AND) components.push_back(std::make_unique<GateANDComponent>(x, y));
        else if (type == ComponentType::GATE_OR) components.push_back(std::make_unique<GateORComponent>(x, y));
        else if (type == ComponentType::GATE_NOT) components.push_back(std::make_unique<GateNOTComponent>(x, y));
        else if (type == ComponentType::GATE_NAND) components.push_back(std::make_unique<GateNANDComponent>(x, y));
        else if (type == ComponentType::GATE_NOR) components.push_back(std::make_unique<GateNORComponent>(x, y));
        else if (type == ComponentType::GATE_XOR) components.push_back(std::make_unique<GateXORComponent>(x, y));
        else if (type == ComponentType::FLIP_FLOP_D) components.push_back(std::make_unique<FlipFlopDComponent>(x, y));

        // اعمال زاویه و قرینه به قطعه
        if (!components.empty()) {
            components.back()->angle = angle;
            components.back()->isMirrored = isMirrored;
        }
    }
}

void EditorPage::SaveCurrentStateForUndo()
{
    // اگر از 10 تا بیشتر شد، قدیمی‌ترین را پاک کن
    if (undoStack.size() >= 10) {
        undoStack.erase(undoStack.begin());
    }

    // وضعیت فعلی را به لیست Undo اضافه کن
    undoStack.push_back(SaveStateToString());

    // وقتی کار جدیدی انجام می‌شود، لیست Redo باید خالی شود
    redoStack.clear();
}

void EditorPage::Undo()
{
    if (undoStack.empty()) return; // اگر چیزی برای برگشت نیست خارج شو

    // وضعیت فعلی را برای Redo ذخیره کن
    redoStack.push_back(SaveStateToString());

    // وضعیت قبلی را بخوان و اعمال کن
    std::string prevState = undoStack.back();
    undoStack.pop_back();
    LoadStateFromString(prevState);

    std::cout << "Undo Performed!" << std::endl;
}

void EditorPage::Redo()
{
    if (redoStack.empty()) return; // اگر چیزی برای جلو رفتن نیست خارج شو

    // وضعیت فعلی را برای Undo ذخیره کن
    undoStack.push_back(SaveStateToString());

    // وضعیت بعدی را بخوان و اعمال کن
    std::string nextState = redoStack.back();
    redoStack.pop_back();
    LoadStateFromString(nextState);

    std::cout << "Redo Performed!" << std::endl;
}
// ========================================================
// خروجی تصویر از مدار (با استفاده از تابع بومی SDL_SavePNG)
// ========================================================
void EditorPage::ExportToImage(SDL_Renderer* renderer)
{
    // خواندن تمام پیکسل‌های رندر شده روی صفحه
    SDL_Surface* surface = SDL_RenderReadPixels(renderer, NULL);

    if (surface) {
        // استفاده از تابع داخلی و جدید SDL3 برای ذخیره PNG (بدون نیاز به DLL اضافه)
        bool result = SDL_SavePNG(surface, "Circuit_Output.png");

        if (result == true) {
            std::cout << "\n============================================\n";
            std::cout << " SUCCESS: Image Exported to 'Circuit_Output.png'" << std::endl;
            std::cout << "============================================\n";
        } else {
            std::cout << "Export PNG Failed: " << SDL_GetError() << std::endl;
        }

        SDL_DestroySurface(surface);
    } else {
        std::cout << "Export Failed (Could not read pixels): " << SDL_GetError() << std::endl;
    }
}
void EditorPage::HandleMouseMove(int x, int y) {
    currentMouseX = x; currentMouseY = y;

    if (isDragging) {
        float dx = x - lastMouseX;
        float dy = y - lastMouseY;
        for (auto& c : components) {
            if (c->isSelected) {
                c->x += dx;
                c->y += dy;
            }
        }
        lastMouseX = x; lastMouseY = y;
    }
}

void EditorPage::HandleMouseRelease(int x, int y) {
    if (isDragging) {
        SaveCurrentStateForUndo();
        for (auto& c : components) {
            if (c->isSelected) {
                c->x = std::round(c->x / 20.0f) * 20.0f;
                c->y = std::round(c->y / 20.0f) * 20.0f;
            }
        }
        isDragging = false;
    }

    if (isSelectingBox) {
        isSelectingBox = false;

        // اینجا هم (float) را قبل از currentMouseX و currentMouseY اضافه کنید:
        float rx = std::min(selStartX, (float)currentMouseX);
        float ry = std::min(selStartY, (float)currentMouseY);
        float rw = std::abs(selStartX - (float)currentMouseX);
        float rh = std::abs(selStartY - (float)currentMouseY);

        for (auto& c : components) {
            if (c->x >= rx && c->x <= rx + rw && c->y >= ry && c->y <= ry + rh) {
                c->isSelected = true;
            }
        }
    }

}
