#include "EditorPage.h"
#include "TextRenderer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>

EditorPage::EditorPage(SDL_Window* window)
    : search(window)
{
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
        {"NOR Gate", ComponentType::GATE_NOR},
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
    std::string query = search.GetText();
    std::transform(query.begin(), query.end(), query.begin(), ::tolower);

    filteredTools.clear();

    for (const auto& tool : allTools) {
        std::string toolName = tool.name;
        std::transform(toolName.begin(), toolName.end(), toolName.begin(), ::tolower);

        if (query.empty() || toolName.find(query) != std::string::npos) {
            filteredTools.push_back(tool);
        }
    }
}

//-----------------------------------------
// Zoom / Pan / Coordinate helpers
//-----------------------------------------
void EditorPage::ScreenToWorld(int sx, int sy, float& wx, float& wy) const
{
    wx = ((float)sx - (canvasBaseX + panOffsetX)) / zoom;
    wy = ((float)sy - (canvasBaseY + panOffsetY)) / zoom;
}

void EditorPage::HandleMouseWheel(float wheelY, int mouseX, int mouseY)
{
    if (wheelY == 0.0f) return;

    float oldZoom = zoom;
    float zoomFactor = 1.0f + (wheelY > 0 ? 0.1f : -0.1f);
    float newZoom = zoom * zoomFactor;

    if (newZoom < minZoom) newZoom = minZoom;
    if (newZoom > maxZoom) newZoom = maxZoom;
    if (newZoom == oldZoom) return;

    // نقطه‌ی زیر نشانگر موس باید بعد از زوم هم ثابت بماند (Zoom to cursor)
    float worldXBefore = ((float)mouseX - (canvasBaseX + panOffsetX)) / oldZoom;
    float worldYBefore = ((float)mouseY - (canvasBaseY + panOffsetY)) / oldZoom;

    zoom = newZoom;

    panOffsetX = (float)mouseX - canvasBaseX - worldXBefore * zoom;
    panOffsetY = (float)mouseY - canvasBaseY - worldYBefore * zoom;

    if (panOffsetX > panLimit) panOffsetX = panLimit;
    if (panOffsetX < -panLimit) panOffsetX = -panLimit;
    if (panOffsetY > panLimit) panOffsetY = panLimit;
    if (panOffsetY < -panLimit) panOffsetY = -panLimit;
}

void EditorPage::StartPan(int x, int y)
{
    isPanning = true;
    panMouseStartX = (float)x;
    panMouseStartY = (float)y;
    panOffsetStartX = panOffsetX;
    panOffsetStartY = panOffsetY;
}

void EditorPage::UpdatePan(int x, int y)
{
    if (!isPanning) return;

    float dx = (float)x - panMouseStartX;
    float dy = (float)y - panMouseStartY;

    panOffsetX = panOffsetStartX + dx;
    panOffsetY = panOffsetStartY + dy;

    if (panOffsetX > panLimit) panOffsetX = panLimit;
    if (panOffsetX < -panLimit) panOffsetX = -panLimit;
    if (panOffsetY > panLimit) panOffsetY = panLimit;
    if (panOffsetY < -panLimit) panOffsetY = -panLimit;
}

void EditorPage::StopPan()
{
    isPanning = false;
}

void EditorPage::ResetView()
{
    zoom = 1.0f;
    panOffsetX = 0.0f;
    panOffsetY = 0.0f;
}

//-----------------------------------------
// Draw Helpers
//-----------------------------------------
void EditorPage::DrawGrid(SDL_Renderer* renderer)
{
    // این تابع نگه‌داشته شده برای سازگاری با کد قبلی؛ رسم اصلی شبکه
    // اکنون داخل Draw() و با پشتیبانی از Zoom/Pan انجام می‌شود.
    SDL_SetRenderDrawColor(renderer, 215, 215, 215, 255);
    int gridSize = gridSpacing;

    for (float x = 100; x < 950; x += gridSize) {
        SDL_RenderLine(renderer, x, 50, x, 600);
    }
    for (float y = 50; y < 600; y += gridSize) {
        SDL_RenderLine(renderer, 100, y, 950, y);
    }
}

void EditorPage::DrawSidebar(SDL_Renderer* renderer)
{
    SDL_FRect sidebarArea = { 0, 50, 100, 550 };
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderFillRect(renderer, &sidebarArea);

    int startY = 70;
    for (size_t i = 0; i < filteredTools.size(); i++) {
        SDL_FRect btnRect = { 10, (float)(startY + i * 40), 80, 30 };

        if (isPlacingMode && selectedTool == filteredTools[i].type) {
            SDL_SetRenderDrawColor(renderer, 140, 240, 240, 255);
        } else {
            SDL_SetRenderDrawColor(renderer, 230, 230, 230, 255);
        }

        SDL_RenderFillRect(renderer, &btnRect);

        SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
        SDL_RenderRect(renderer, &btnRect);

        SDL_Color textColor = {0, 0, 0, 255};
        SDL_Texture* txt = TextRenderer::CreateText(renderer, filteredTools[i].name.c_str(), textColor);

        if (txt) {
            float texW = 0, texH = 0;
            SDL_GetTextureSize(txt, &texW, &texH);

            if (texW > 75) texW = 75;
            if (texH > 20) texH = 20;

            SDL_FRect txtPos = {
                12,
                (float)(startY + i * 40 + 5),
                texW,
                texH
            };

            SDL_RenderTexture(renderer, txt, NULL, &txtPos);
            SDL_DestroyTexture(txt);
        }
    }
}

// علامت مبدأ مختصات (0,0) بوم — این تابع باید هنگامی صدا زده شود که
// Viewport/Scale بوم فعال است (یعنی داخل بلاک رسم World-space)
void EditorPage::DrawOriginMarker(SDL_Renderer* renderer)
{
    SDL_SetRenderDrawColor(renderer, 200, 30, 30, 210);

    // اندازه‌ی ثابت روی صفحه، مستقل از سطح زوم
    float s = 9.0f / zoom;

    SDL_RenderLine(renderer, -s, 0, s, 0);
    SDL_RenderLine(renderer, 0, -s, 0, s);

    float boxSize = 6.0f / zoom;
    SDL_FRect box = { -boxSize / 2.0f, -boxSize / 2.0f, boxSize, boxSize };
    SDL_RenderRect(renderer, &box);
}

// نوار وضعیت پایین صفحه: مختصات لحظه‌ای موس + درصد زوم + دکمه بازگشت به ۱۰۰٪
void EditorPage::DrawStatusBar(SDL_Renderer* renderer, int windowW, int windowH)
{
    const int barHeight = 26;

    SDL_FRect bar = { 0, (float)(windowH - barHeight), (float)windowW, (float)barHeight };
    SDL_SetRenderDrawColor(renderer, 235, 235, 235, 255);
    SDL_RenderFillRect(renderer, &bar);

    SDL_SetRenderDrawColor(renderer, 180, 180, 180, 255);
    SDL_RenderLine(renderer, 0, (float)(windowH - barHeight), (float)windowW, (float)(windowH - barHeight));

    SDL_Color black = {30, 30, 30, 255};

    // --- مختصات لحظه‌ای نشانگر موس (نسبت به مبدأ بوم) ---
    std::ostringstream coordStream;
    if (currentMouseX >= (int)canvasBaseX && currentMouseY >= (int)canvasBaseY) {
        coordStream << "X: " << (int)std::round(worldMouseX) << "   Y: " << (int)std::round(worldMouseY);
    } else {
        coordStream << "X: --   Y: --";
    }
    std::string coordText = coordStream.str();

    SDL_Texture* tCoord = TextRenderer::CreateText(renderer, coordText.c_str(), black);
    if (tCoord) {
        float tw = 0, th = 0;
        SDL_GetTextureSize(tCoord, &tw, &th);
        SDL_FRect p = { 15, (float)(windowH - barHeight + 4), tw, th };
        SDL_RenderTexture(renderer, tCoord, NULL, &p);
        SDL_DestroyTexture(tCoord);
    }

    // --- وضعیت پن (اگر در حال جابه‌جایی بوم هستیم) ---
    if (isPanning) {
        SDL_Texture* tPan = TextRenderer::CreateText(renderer, "Panning...", black);
        if (tPan) {
            float tw = 0, th = 0;
            SDL_GetTextureSize(tPan, &tw, &th);
            SDL_FRect p = { 220, (float)(windowH - barHeight + 4), tw, th };
            SDL_RenderTexture(renderer, tPan, NULL, &p);
            SDL_DestroyTexture(tPan);
        }
    }

    // --- درصد زوم ---
    std::ostringstream zoomStream;
    zoomStream << "Zoom: " << (int)std::round(zoom * 100) << "%";
    std::string zoomText = zoomStream.str();

    SDL_Texture* tZoom = TextRenderer::CreateText(renderer, zoomText.c_str(), black);
    if (tZoom) {
        float tw = 0, th = 0;
        SDL_GetTextureSize(tZoom, &tw, &th);
        SDL_FRect p = { (float)(windowW - tw - 100), (float)(windowH - barHeight + 4), tw, th };
        SDL_RenderTexture(renderer, tZoom, NULL, &p);
        SDL_DestroyTexture(tZoom);
    }

    // --- دکمه‌ی بازگشت به ۱۰۰٪ (مطابق با محدوده‌ی چک‌شده در HandleClick) ---
    SDL_FRect resetBtn = { (float)(windowW - 80), (float)(windowH - barHeight + 2), 70, (float)(barHeight - 4) };
    SDL_SetRenderDrawColor(renderer, 210, 225, 250, 255);
    SDL_RenderFillRect(renderer, &resetBtn);
    SDL_SetRenderDrawColor(renderer, 100, 100, 100, 255);
    SDL_RenderRect(renderer, &resetBtn);

    SDL_Texture* tReset = TextRenderer::CreateText(renderer, "Reset 100%", black);
    if (tReset) {
        float tw = 0, th = 0;
        SDL_GetTextureSize(tReset, &tw, &th);
        if (tw > resetBtn.w - 6) tw = resetBtn.w - 6;
        if (th > resetBtn.h - 4) th = resetBtn.h - 4;
        SDL_FRect p = { resetBtn.x + 3, resetBtn.y + 2, tw, th };
        SDL_RenderTexture(renderer, tReset, NULL, &p);
        SDL_DestroyTexture(tReset);
    }
}

//-----------------------------------------
// Draw
//-----------------------------------------
void EditorPage::Draw(SDL_Renderer* renderer)
{
    UpdateSearchFilter();

    bool isA3 = (pageSize.find("A3") != std::string::npos);

    float gridMaxX = isA3 ? 1350.0f : 950.0f;
    float gridMaxY = isA3 ? 800.0f : 600.0f;

    // اندازه‌ی کاغذ در مختصات جهانی (نسبت به مبدأ بوم، یعنی گوشه‌ی بالا-چپ ناحیه طراحی)
    float worldPaperW = gridMaxX - canvasBaseX;
    float worldPaperH = gridMaxY - canvasBaseY;

    int winW = 0, winH = 0;
    SDL_GetRenderOutputSize(renderer, &winW, &winH);
    lastWindowW = winW;
    lastWindowH = winH;

    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    // -----------------------------------------------------------
    // ۱. پس‌زمینه‌ی کلی پنجره (بیرون از بوم)
    // -----------------------------------------------------------
    SDL_SetRenderDrawColor(renderer, 225, 225, 225, 255);
    SDL_FRect fullArea = { 0, 0, (float)winW, (float)winH };
    SDL_RenderFillRect(renderer, &fullArea);

    // -----------------------------------------------------------
    // ۲. فعال کردن Viewport + Scale برای بوم طراحی (اینجا Zoom/Pan اعمال می‌شود)
    //    هرچه داخل این بلاک رسم شود، در مختصات «جهانی» (World) است.
    // -----------------------------------------------------------
    SDL_Rect canvasViewport;
    canvasViewport.x = (int)(canvasBaseX + panOffsetX);
    canvasViewport.y = (int)(canvasBaseY + panOffsetY);
    canvasViewport.w = 8000;
    canvasViewport.h = 8000;
    SDL_SetRenderViewport(renderer, &canvasViewport);
    SDL_SetRenderScale(renderer, zoom, zoom);

    // پس‌زمینه‌ی سفید کاغذ
    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_FRect paperRect = { 0, 0, worldPaperW, worldPaperH };
    SDL_RenderFillRect(renderer, &paperRect);

    // خطوط شبکه (Grid) - رنگ کم‌رنگ و نیمه‌شفاف تا کمک به تراز کند ولی مزاحم دید نباشد
    SDL_SetRenderDrawColor(renderer, 190, 190, 205, 100);
    for (float gx = 0; gx <= worldPaperW; gx += gridSpacing) {
        SDL_RenderLine(renderer, gx, 0, gx, worldPaperH);
    }
    for (float gy = 0; gy <= worldPaperH; gy += gridSpacing) {
        SDL_RenderLine(renderer, 0, gy, worldPaperW, gy);
    }

    // نقاط تاکیدی هر ۵ خط، برای راهنمایی بهتر چشم بدون شلوغ‌کردن صفحه
    SDL_SetRenderDrawColor(renderer, 140, 140, 160, 150);
    for (float gx = 0; gx <= worldPaperW; gx += gridSpacing * 5) {
        for (float gy = 0; gy <= worldPaperH; gy += gridSpacing * 5) {
            SDL_FRect dot = { gx - 1, gy - 1, 2, 2 };
            SDL_RenderFillRect(renderer, &dot);
        }
    }

    // مرز کاغذ
    SDL_SetRenderDrawColor(renderer, 120, 120, 120, 255);
    SDL_RenderRect(renderer, &paperRect);

    // علامت مبدأ مختصات (0,0)
    DrawOriginMarker(renderer);

    // =========================================================
    // رسم قطعات (بدون تغییر نسبت به قبل، چون در فضای جهانی هستند)
    // =========================================================
    for (auto& comp : components) {
        comp->Update();
        comp->Draw(renderer);

        // هایلایت انتخاب (بخش ۴.۲): متناسب با اندازه‌ی واقعی هر قطعه،
        // نه یک جعبه‌ی ثابت — تا هم برای قطعات کوچک (مثلاً LOGIC 30x30)
        // و هم بزرگ (مثلاً D-FF 60x60) دقیق و خوانا باشد.
        if (comp->isSelected) {
            float pad = 6.0f;
            SDL_FRect selRect = {
                comp->x - pad,
                comp->y - pad,
                comp->width  + pad * 2.0f,
                comp->height + pad * 2.0f
            };
            SDL_SetRenderDrawColor(renderer, 0, 150, 255, 60);
            SDL_RenderFillRect(renderer, &selRect);
            SDL_SetRenderDrawColor(renderer, 0, 90, 255, 255);
            SDL_RenderRect(renderer, &selRect);
        }
    }

    // مستطیل انتخاب گروهی (در فضای جهانی)
    if (isSelectingBox) {
        SDL_SetRenderDrawColor(renderer, 0, 150, 255, 50);

        float rx = std::min(selStartX, worldMouseX);
        float ry = std::min(selStartY, worldMouseY);
        float rw = std::abs(selStartX - worldMouseX);
        float rh = std::abs(selStartY - worldMouseY);

        SDL_FRect selBox = { rx, ry, rw, rh };
        SDL_RenderFillRect(renderer, &selBox);
        SDL_SetRenderDrawColor(renderer, 0, 150, 255, 255);
        SDL_RenderRect(renderer, &selBox);
    }

    // سایه (Ghosting) قطعه هنگام جای‌گذاری - با Snap to Grid در فضای جهانی
    if (isPlacingMode && worldMouseX >= 0 && worldMouseY >= 0) {
        float snapX = std::round(worldMouseX / (float)gridSpacing) * gridSpacing;
        float snapY = std::round(worldMouseY / (float)gridSpacing) * gridSpacing;

        SDL_SetRenderDrawColor(renderer, 0, 200, 0, 120);
        SDL_FRect ghostRect = { snapX, snapY, 60, 40 };
        SDL_RenderFillRect(renderer, &ghostRect);
    }

    // -----------------------------------------------------------
    // ۳. غیرفعال کردن Viewport/Scale؛ از این‌جا به بعد رسم در فضای صفحه (UI ثابت)
    // -----------------------------------------------------------
    SDL_SetRenderScale(renderer, 1.0f, 1.0f);
    SDL_SetRenderViewport(renderer, NULL);

    DrawSidebar(renderer);

    menu.Draw(renderer);
    search.Draw(renderer);

    SDL_Color black = {0, 0, 0, 255};

    SDL_FRect rightSidebarArea = { gridMaxX - 100, 50, 100, gridMaxY - 50 };
    SDL_SetRenderDrawColor(renderer, 220, 220, 220, 255);
    SDL_RenderFillRect(renderer, &rightSidebarArea);

    SDL_SetRenderDrawColor(renderer, 150, 255, 150, 255);
    SDL_FRect runBtn = { gridMaxX - 90, 70, 80, 30 };
    SDL_RenderFillRect(renderer, &runBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &runBtn);

    SDL_Texture* tRun = TextRenderer::CreateText(renderer, "Run", black);
    if (tRun) {
        SDL_FRect p = {gridMaxX - 65, 75, 30, 20};
        SDL_RenderTexture(renderer, tRun, NULL, &p);
        SDL_DestroyTexture(tRun);
    }

    SDL_SetRenderDrawColor(renderer, 255, 230, 120, 255);
    SDL_FRect pauseBtn = { gridMaxX - 90, 120, 80, 30 };
    SDL_RenderFillRect(renderer, &pauseBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &pauseBtn);

    SDL_Texture* tPause = TextRenderer::CreateText(renderer, "Pause", black);
    if (tPause) {
        SDL_FRect p = {gridMaxX - 75, 125, 50, 20};
        SDL_RenderTexture(renderer, tPause, NULL, &p);
        SDL_DestroyTexture(tPause);
    }

    SDL_SetRenderDrawColor(renderer, 255, 150, 150, 255);
    SDL_FRect stopBtn = { gridMaxX - 90, 170, 80, 30 };
    SDL_RenderFillRect(renderer, &stopBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &stopBtn);

    SDL_Texture* tStop = TextRenderer::CreateText(renderer, "Stop", black);
    if (tStop) {
        SDL_FRect p = {gridMaxX - 70, 175, 40, 20};
        SDL_RenderTexture(renderer, tStop, NULL, &p);
        SDL_DestroyTexture(tStop);
    }

    SDL_SetRenderDrawColor(renderer, 200, 220, 255, 255);
    SDL_FRect rotBtn = { 450, 10, 60, 30 };
    SDL_RenderFillRect(renderer, &rotBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &rotBtn);
    SDL_Texture* tRot = TextRenderer::CreateText(renderer, "Rot 90", black);
    if (tRot) { SDL_FRect p = {455, 15, 50, 20}; SDL_RenderTexture(renderer, tRot, NULL, &p); SDL_DestroyTexture(tRot); }

    SDL_SetRenderDrawColor(renderer, 255, 220, 200, 255);
    SDL_FRect mirBtn = { 520, 10, 60, 30 };
    SDL_RenderFillRect(renderer, &mirBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &mirBtn);
    SDL_Texture* tMir = TextRenderer::CreateText(renderer, "Mirror", black);
    if (tMir) { SDL_FRect p = {525, 15, 50, 20}; SDL_RenderTexture(renderer, tMir, NULL, &p); SDL_DestroyTexture(tMir); }

    SDL_SetRenderDrawColor(renderer, 255, 100, 100, 255);
    SDL_FRect delBtn = { 590, 10, 60, 30 };
    SDL_RenderFillRect(renderer, &delBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &delBtn);
    SDL_Texture* tDel = TextRenderer::CreateText(renderer, "Delete", black);
    if (tDel) { SDL_FRect p = {595, 15, 50, 20}; SDL_RenderTexture(renderer, tDel, NULL, &p); SDL_DestroyTexture(tDel); }

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_FRect undoBtn = { 660, 10, 70, 30 };
    SDL_RenderFillRect(renderer, &undoBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &undoBtn);
    SDL_Texture* txtUndo = TextRenderer::CreateText(renderer, "< Undo", black);
    if (txtUndo) { SDL_FRect p = {665, 15, 60, 20}; SDL_RenderTexture(renderer, txtUndo, NULL, &p); SDL_DestroyTexture(txtUndo); }

    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_FRect redoBtn = { 740, 10, 70, 30 };
    SDL_RenderFillRect(renderer, &redoBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &redoBtn);
    SDL_Texture* txtRedo = TextRenderer::CreateText(renderer, "Redo >", black);
    if (txtRedo) { SDL_FRect p = {745, 15, 60, 20}; SDL_RenderTexture(renderer, txtRedo, NULL, &p); SDL_DestroyTexture(txtRedo); }

    SDL_SetRenderDrawColor(renderer, 200, 255, 200, 255);
    SDL_FRect exportBtn = { 820, 10, 110, 30 };
    SDL_RenderFillRect(renderer, &exportBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &exportBtn);
    SDL_Texture* txtExport = TextRenderer::CreateText(renderer, "Export Image", black);
    if (txtExport) { SDL_FRect p = {825, 15, 100, 20}; SDL_RenderTexture(renderer, txtExport, NULL, &p); SDL_DestroyTexture(txtExport); }

    // نوار وضعیت پایین صفحه (مختصات + زوم + دکمه Reset)
    DrawStatusBar(renderer, winW, winH);

    if (exportRequested) {
        ExportToImage(renderer);
        exportRequested = false;
    }
}

//-----------------------------------------
// Mouse & Keyboard
//-----------------------------------------
EditorMenuAction EditorPage::HandleClick(int x, int y)
{
    // دکمه‌ی «بازگشت به ۱۰۰٪» در نوار وضعیت پایین صفحه (فضای صفحه)
    SDL_FRect resetBtn = { (float)(lastWindowW - 80), (float)(lastWindowH - 24), 70, 22 };
    if (x >= resetBtn.x && x <= resetBtn.x + resetBtn.w && y >= resetBtn.y && y <= resetBtn.y + resetBtn.h) {
        ResetView();
        return (EditorMenuAction)0;
    }

    EditorMenuAction action = menu.HandleClick(x, y);
    search.HandleClick(x, y);

    // ---------------- نوار ابزار بالا (فضای صفحه، بدون تغییر) ----------------
    if (x >= 450 && x <= 510 && y >= 10 && y <= 40) {
        SaveCurrentStateForUndo();
        for (auto& c : components) if (c->isSelected) c->angle = (c->angle + 90) % 360;
        return (EditorMenuAction)0;
    }
    if (x >= 520 && x <= 580 && y >= 10 && y <= 40) {
        SaveCurrentStateForUndo();
        for (auto& c : components) if (c->isSelected) c->isMirrored = !c->isMirrored;
        return (EditorMenuAction)0;
    }
    if (x >= 590 && x <= 650 && y >= 10 && y <= 40) {
        SaveCurrentStateForUndo();
        components.erase(std::remove_if(components.begin(), components.end(),
            [](const std::unique_ptr<Component>& c) { return c->isSelected; }), components.end());
        return (EditorMenuAction)0;
    }
    if (x >= 660 && x <= 730 && y >= 10 && y <= 40) {
        Undo();
        return (EditorMenuAction)0;
    }
    if (x >= 740 && x <= 810 && y >= 10 && y <= 40) {
        Redo();
        return (EditorMenuAction)0;
    }
    if (x >= 820 && x <= 930 && y >= 10 && y <= 40) {
        exportRequested = true;
        return (EditorMenuAction)0;
    }

    // ---------------- سایدبار چپ (فضای صفحه، بدون تغییر) ----------------
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

    // از این‌جا به بعد تعاملات مربوط به داخل بوم طراحی است؛
    // مختصات صفحه را به مختصات جهانی تبدیل می‌کنیم تا زوم/پن را در نظر بگیرند
    float wx, wy;
    ScreenToWorld(x, y, wx, wy);

    // ---------------- جای‌گذاری قطعه با Snap to Grid ----------------
    if (isPlacingMode && x >= (int)canvasBaseX && y >= (int)canvasBaseY) {

        float snapX = std::round(wx / (float)gridSpacing) * gridSpacing;
        float snapY = std::round(wy / (float)gridSpacing) * gridSpacing;

        SaveCurrentStateForUndo();

        bool placed = false;

        if (selectedTool == ComponentType::GND) { components.push_back(std::make_unique<GNDComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::DC_SOURCE) { components.push_back(std::make_unique<DCSourceComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::BATTERY) { components.push_back(std::make_unique<BatteryComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::CLOCK) { components.push_back(std::make_unique<ClockComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::LOGIC_STATE) { components.push_back(std::make_unique<LogicStateComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::RESISTOR) { components.push_back(std::make_unique<ResistorComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::CAPACITOR) { components.push_back(std::make_unique<CapacitorComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::SWITCH) { components.push_back(std::make_unique<SwitchComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::PUSH_BUTTON) { components.push_back(std::make_unique<PushButtonComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::LED) { components.push_back(std::make_unique<LEDComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::SEVEN_SEGMENT) { components.push_back(std::make_unique<SevenSegmentComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::GATE_AND) { components.push_back(std::make_unique<GateANDComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::GATE_OR) { components.push_back(std::make_unique<GateORComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::GATE_NOT) { components.push_back(std::make_unique<GateNOTComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::GATE_XOR) { components.push_back(std::make_unique<GateXORComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::GATE_NAND) { components.push_back(std::make_unique<GateNANDComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::GATE_NOR) { components.push_back(std::make_unique<GateNORComponent>(snapX, snapY)); placed = true; }
        else if (selectedTool == ComponentType::FLIP_FLOP_D) { components.push_back(std::make_unique<FlipFlopDComponent>(snapX, snapY)); placed = true; }

        if (placed) {
            std::cout << "SUCCESS: Component placed at X:" << snapX << " Y:" << snapY << std::endl;
        } else {
            std::cout << "ERROR: selectedTool did not match any component!" << std::endl;
        }

        // ---------------------------------------------------------------
        // توجه: عمداً isPlacingMode را false نمی‌کنیم.
        // طبق مشخصات، کاربر با یک‌بار انتخاب قطعه از سایدبار، باید بتواند
        // با کلیک‌های پیاپی، هر تعداد که خواست نمونه‌ی جدید از همان قطعه
        // را روی بوم قرار دهد. خروج از حالت جای‌گذاری فقط با یکی از این‌ها
        // انجام می‌شود: کلید Esc، کلیک راست موس، یا انتخاب یک ابزار دیگر
        // از سایدبار (که selectedTool را عوض می‌کند).
        // ---------------------------------------------------------------
        return (EditorMenuAction)0;
    }

    // ---------------- تعامل با قطعات موجود (کلیک روی کلید/سوییچ و ...) ----------------
    if (!isPlacingMode) {
        for (auto& comp : components) {
            if (comp->HandleClick(wx, wy)) {
                break;
            }
        }
    }

    // ---------------- انتخاب قطعه / شروع درگ / شروع مستطیل انتخاب گروهی ----------------
    if (!isPlacingMode && x >= (int)canvasBaseX && y >= (int)canvasBaseY) {
        bool clickedOnComponent = false;

        for (auto it = components.rbegin(); it != components.rend(); ++it) {
            if ((*it)->Contains(wx, wy)) {
                clickedOnComponent = true;
                if (!(*it)->isSelected) {
                    for (auto& c : components) c->isSelected = false;
                    (*it)->isSelected = true;
                }
                isDragging = true;
                lastMouseX = wx;
                lastMouseY = wy;
                break;
            }
        }

        if (!clickedOnComponent) {
            for (auto& c : components) c->isSelected = false;
            isSelectingBox = true;
            selStartX = wx;
            selStartY = wy;
        }
    }

    return action;
}

EditorMenuAction EditorPage::HandleKeyboard(SDL_Event event)
{
    search.HandleKeyboard(event);

    if (event.type == SDL_EVENT_KEY_DOWN) {

        bool ctrlHeld  = (event.key.mod & SDL_KMOD_CTRL)  != 0;
        bool shiftHeld = (event.key.mod & SDL_KMOD_SHIFT) != 0;

        // ---------------- Ctrl+S : ذخیره‌ی پروژه ----------------
        // منطق واقعیِ ذخیره‌سازی (نوشتن روی دیسک) در main.cpp انجام می‌شود؛
        // اینجا فقط همان اکشنی را برمی‌گردانیم که دکمه‌ی Save هم برمی‌گرداند.
        if (ctrlHeld && event.key.key == SDLK_S) {
            return EDITOR_SAVE_PROJECT;
        }

        // ---------------- Ctrl+Z : Undo ----------------
        if (ctrlHeld && !shiftHeld && event.key.key == SDLK_Z) {
            Undo();
            return (EditorMenuAction)0;
        }

        // ---------------- Ctrl+Y  یا  Ctrl+Shift+Z : Redo ----------------
        if ((ctrlHeld && event.key.key == SDLK_Y) ||
            (ctrlHeld && shiftHeld && event.key.key == SDLK_Z)) {
            Redo();
            return (EditorMenuAction)0;
        }

        // ---------------- Delete : حذف قطعات انتخاب‌شده ----------------
        if (event.key.key == SDLK_DELETE) {
            bool anySelected = false;
            for (auto& c : components) {
                if (c->isSelected) { anySelected = true; break; }
            }
            if (anySelected) {
                SaveCurrentStateForUndo();
                components.erase(std::remove_if(components.begin(), components.end(),
                    [](const std::unique_ptr<Component>& c) { return c->isSelected; }), components.end());
            }
            return (EditorMenuAction)0;
        }

        // ---------------- Esc : لغو حالت جای‌گذاری قطعه ----------------
        if (event.key.key == SDLK_ESCAPE) {
            isPlacingMode = false;
            return (EditorMenuAction)0;
        }

        // ---------------- کلیدهای میان‌بر زوم: + / - و بازگشت به ۱۰۰٪ با 0 ----------------
        if (event.key.key == SDLK_EQUALS || event.key.key == SDLK_KP_PLUS) {
            HandleMouseWheel(1.0f, currentMouseX, currentMouseY);
            return (EditorMenuAction)0;
        }
        if (event.key.key == SDLK_MINUS || event.key.key == SDLK_KP_MINUS) {
            HandleMouseWheel(-1.0f, currentMouseX, currentMouseY);
            return (EditorMenuAction)0;
        }
        if (event.key.key == SDLK_0 || event.key.key == SDLK_KP_0) {
            ResetView();
            return (EditorMenuAction)0;
        }
    }

    return (EditorMenuAction)0;
}

void EditorPage::ClearWorkspace()
{
    components.clear();
    isPlacingMode = false;
}

//-----------------------------------------
// Save / Load Workspace
//-----------------------------------------
void EditorPage::SaveWorkspace(const std::string& filepath)
{
    std::ofstream file(filepath);
    if (!file.is_open()) {
        std::cout << "Error: Could not save file to " << filepath << std::endl;
        return;
    }

    for (const auto& comp : components) {
        file << (int)comp->type << " " << comp->x << " " << comp->y << " "
             << comp->angle << " " << comp->isMirrored << "\n";
    }

    file.close();
    std::cout << "Workspace saved successfully to: " << filepath << std::endl;
}

void EditorPage::LoadWorkspace(const std::string& filepath)
{
    ClearWorkspace();

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cout << "Error: Could not open file " << filepath << std::endl;
        return;
    }

    int typeInt, angle;
    float x, y;
    bool isMirrored;

    while (file >> typeInt >> x >> y >> angle >> isMirrored)
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

        if (!components.empty()) {
            components.back()->angle = angle;
            components.back()->isMirrored = isMirrored;
        }
    }

    file.close();
    std::cout << "Workspace loaded successfully from: " << filepath << std::endl;
}

//-----------------------------------------
// Undo / Redo
//-----------------------------------------
std::string EditorPage::SaveStateToString()
{
    std::stringstream ss;
    for (const auto& comp : components) {
        ss << (int)comp->type << " " << comp->x << " " << comp->y << " "
           << comp->angle << " " << comp->isMirrored << "\n";
    }
    return ss.str();
}

void EditorPage::LoadStateFromString(const std::string& state)
{
    ClearWorkspace();
    std::stringstream ss(state);

    int typeInt, angle;
    float x, y;
    bool isMirrored;

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

        if (!components.empty()) {
            components.back()->angle = angle;
            components.back()->isMirrored = isMirrored;
        }
    }
}

void EditorPage::SaveCurrentStateForUndo()
{
    if (undoStack.size() >= 10) {
        undoStack.erase(undoStack.begin());
    }

    undoStack.push_back(SaveStateToString());
    redoStack.clear();
}

void EditorPage::Undo()
{
    if (undoStack.empty()) return;

    redoStack.push_back(SaveStateToString());

    std::string prevState = undoStack.back();
    undoStack.pop_back();
    LoadStateFromString(prevState);

    std::cout << "Undo Performed!" << std::endl;
}

void EditorPage::Redo()
{
    if (redoStack.empty()) return;

    undoStack.push_back(SaveStateToString());

    std::string nextState = redoStack.back();
    redoStack.pop_back();
    LoadStateFromString(nextState);

    std::cout << "Redo Performed!" << std::endl;
}

//-----------------------------------------
// Export
//-----------------------------------------
void EditorPage::ExportToImage(SDL_Renderer* renderer)
{
    SDL_Surface* surface = SDL_RenderReadPixels(renderer, NULL);

    if (surface) {
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

//-----------------------------------------
// Mouse Move / Release
//-----------------------------------------
void EditorPage::HandleMouseMove(int x, int y) {
    currentMouseX = x;
    currentMouseY = y;

    float wx, wy;
    ScreenToWorld(x, y, wx, wy);
    worldMouseX = wx;
    worldMouseY = wy;

    if (isPanning) {
        UpdatePan(x, y);
        return;
    }

    if (isDragging) {
        float dx = wx - lastMouseX;
        float dy = wy - lastMouseY;
        for (auto& c : components) {
            if (c->isSelected) {
                c->x += dx;
                c->y += dy;
            }
        }
        lastMouseX = wx;
        lastMouseY = wy;
    }
}

void EditorPage::HandleMouseRelease(int x, int y) {
    if (isPanning) {
        StopPan();
    }

    if (isDragging) {
        SaveCurrentStateForUndo();
        for (auto& c : components) {
            if (c->isSelected) {
                c->x = std::round(c->x / (float)gridSpacing) * gridSpacing;
                c->y = std::round(c->y / (float)gridSpacing) * gridSpacing;
            }
        }
        isDragging = false;
    }

    if (isSelectingBox) {
        isSelectingBox = false;

        float rx = std::min(selStartX, worldMouseX);
        float ry = std::min(selStartY, worldMouseY);
        float rw = std::abs(selStartX - worldMouseX);
        float rh = std::abs(selStartY - worldMouseY);

        // بخش ۴.۲.۲ (Multi-Select): قطعه انتخاب می‌شود اگر «داخل» مستطیل
        // انتخاب باشد یا با آن «تقاطع» داشته باشد — یعنی تست هم‌پوشانی
        // واقعی دو مستطیل (AABB overlap)، نه فقط بودن نقطه‌ی گوشه‌ی قطعه
        // داخل مستطیل انتخاب.
        for (auto& c : components) {
            bool intersects = !(c->x > rx + rw ||
                                 c->x + c->width < rx ||
                                 c->y > ry + rh ||
                                 c->y + c->height < ry);
            if (intersects) {
                c->isSelected = true;
            }
        }
    }
}
