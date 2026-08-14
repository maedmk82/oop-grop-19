#include "EditorPage.h"
#include "TextRenderer.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iomanip>
#include <cstdio>

EditorPage::EditorPage(SDL_Window* window)
    : search(window)
{
    ownerWindow = window;

    libraryCategories = {
        {"Sources", true},
        {"Analog / Passive", true},
        {"Digital Inputs", false},
        {"Outputs", false},
        {"Logic Gates", false},
        {"Sequential", false}
    };

    allTools = {
        {"GND (Ground)", ComponentType::GND, "Sources"},
        {"DC Voltage", ComponentType::DC_SOURCE, "Sources"},
        {"Battery", ComponentType::BATTERY, "Sources"},
        {"Pulse Clock", ComponentType::CLOCK, "Sources"},
        {"RES (Resistor)", ComponentType::RESISTOR, "Analog / Passive"},
        {"CAP (Capacitor)", ComponentType::CAPACITOR, "Analog / Passive"},
        {"Logic State (0/1)", ComponentType::LOGIC_STATE, "Digital Inputs"},
        {"Push Button", ComponentType::PUSH_BUTTON, "Digital Inputs"},
        {"Switch", ComponentType::SWITCH, "Digital Inputs"},
        {"LED", ComponentType::LED, "Outputs"},
        {"7Seg Display", ComponentType::SEVEN_SEGMENT, "Outputs"},
        {"AND Gate", ComponentType::GATE_AND, "Logic Gates"},
        {"OR Gate", ComponentType::GATE_OR, "Logic Gates"},
        {"NOT Gate", ComponentType::GATE_NOT, "Logic Gates"},
        {"NAND Gate", ComponentType::GATE_NAND, "Logic Gates"},
        {"NOR Gate", ComponentType::GATE_NOR, "Logic Gates"},
        {"XOR Gate", ComponentType::GATE_XOR, "Logic Gates"},
        {"FlipFlop D", ComponentType::FLIP_FLOP_D, "Sequential"}
    };

    activeTools = { ComponentType::RESISTOR, ComponentType::LED, ComponentType::PUSH_BUTTON };
    filteredTools = allTools;
}


std::unique_ptr<Component> EditorPage::CreateComponent(ComponentType type, float x, float y)
{
    switch (type) {
        case ComponentType::GND: return std::make_unique<GNDComponent>(x, y);
        case ComponentType::DC_SOURCE: return std::make_unique<DCSourceComponent>(x, y);
        case ComponentType::BATTERY: return std::make_unique<BatteryComponent>(x, y);
        case ComponentType::CLOCK: return std::make_unique<ClockComponent>(x, y);
        case ComponentType::LOGIC_STATE: return std::make_unique<LogicStateComponent>(x, y);
        case ComponentType::RESISTOR: return std::make_unique<ResistorComponent>(x, y);
        case ComponentType::CAPACITOR: return std::make_unique<CapacitorComponent>(x, y);
        case ComponentType::SWITCH: return std::make_unique<SwitchComponent>(x, y);
        case ComponentType::PUSH_BUTTON: return std::make_unique<PushButtonComponent>(x, y);
        case ComponentType::LED: return std::make_unique<LEDComponent>(x, y);
        case ComponentType::SEVEN_SEGMENT: return std::make_unique<SevenSegmentComponent>(x, y);
        case ComponentType::GATE_AND: return std::make_unique<GateANDComponent>(x, y);
        case ComponentType::GATE_OR: return std::make_unique<GateORComponent>(x, y);
        case ComponentType::GATE_NOT: return std::make_unique<GateNOTComponent>(x, y);
        case ComponentType::GATE_NAND: return std::make_unique<GateNANDComponent>(x, y);
        case ComponentType::GATE_NOR: return std::make_unique<GateNORComponent>(x, y);
        case ComponentType::GATE_XOR: return std::make_unique<GateXORComponent>(x, y);
        case ComponentType::FLIP_FLOP_D: return std::make_unique<FlipFlopDComponent>(x, y);
        default: return nullptr;
    }
}

Component* EditorPage::FindComponentById(int id) const
{
    for (const auto& component : components) {
        if (component && component->id == id) return component.get();
    }
    return nullptr;
}

void EditorPage::AssignComponentId(Component* component, int forcedId)
{
    if (!component) return;

    if (forcedId >= 0) {
        component->id = forcedId;
        if (forcedId >= nextComponentId) nextComponentId = forcedId + 1;
        return;
    }

    component->id = nextComponentId++;
}

void EditorPage::SetWireMode(bool enabled)
{
    isWireMode = enabled;
    if (enabled) {
        isPlacingMode = false;
        for (auto& c : components) c->isSelected = false;
        wireSystem.ClearSelection();
    } else {
        wireSystem.Cancel();
    }
}

bool EditorPage::HandleWireClick(float wx, float wy)
{
    Component* pinComponent = nullptr;
    int pinIndex = -1;

    if (!wireSystem.isDrawing) {
        if (wireSystem.FindPinAt(wx, wy, components, pinComponent, pinIndex)) {
            wireSystem.StartFromPin(pinComponent, pinIndex, components);
            return true;
        }

        int wireIndex = -1;
        WirePoint hitPoint;
        if (wireSystem.FindWireHit(wx, wy, wireIndex, hitPoint)) {
            SaveCurrentStateForUndo();
            wireSystem.StartFromPoint(hitPoint.x, hitPoint.y, gridSpacing, wireIndex);
            return true;
        }

        return true;
    }

    if (wireSystem.FindPinAt(wx, wy, components, pinComponent, pinIndex)) {
        if (wireSystem.activeWire.start.componentId == pinComponent->id &&
            wireSystem.activeWire.start.pinIndex == pinIndex) {
            return true;
        }

        SaveCurrentStateForUndo();
        wireSystem.FinishAtPin(pinComponent, pinIndex, components, gridSpacing);
        return true;
    }

    int wireIndex = -1;
    WirePoint hitPoint;
    if (wireSystem.FindWireHit(wx, wy, wireIndex, hitPoint)) {
        SaveCurrentStateForUndo();
        wireSystem.FinishAtPoint(hitPoint.x, hitPoint.y, gridSpacing, components, true);
        return true;
    }

    // Empty click adds an orthogonal corner and keeps wire drawing active.
    WirePoint snapped = WireSystem::SnapPoint(wx, wy, gridSpacing);
    WireSystem::AddOrthogonalPoint(wireSystem.activeWire.points, snapped, gridSpacing);
    wireSystem.previewPoint = snapped;
    return true;
}

void EditorPage::UpdateWirePreview()
{
    if (isWireMode && wireSystem.isDrawing) {
        wireSystem.UpdatePreview(worldMouseX, worldMouseY, gridSpacing);
    }
}

//-----------------------------------------
// سیستم جستجو
//-----------------------------------------
void EditorPage::UpdateSearchFilter()
{
    std::string query = search.GetText();
    std::transform(query.begin(), query.end(), query.begin(), [](unsigned char c) { return (char)std::tolower(c); });

    filteredTools.clear();

    for (const auto& tool : allTools) {
        std::string haystack = tool.name + " " + tool.category;
        std::transform(haystack.begin(), haystack.end(), haystack.begin(), [](unsigned char c) { return (char)std::tolower(c); });

        if (query.empty() || haystack.find(query) != std::string::npos) {
            filteredTools.push_back(tool);
        }
    }
}

bool EditorPage::IsActiveTool(ComponentType type) const
{
    return std::find(activeTools.begin(), activeTools.end(), type) != activeTools.end();
}

void EditorPage::AddActiveTool(ComponentType type)
{
    if (!IsActiveTool(type)) activeTools.push_back(type);
}

void EditorPage::RemoveActiveTool(ComponentType type)
{
    activeTools.erase(std::remove(activeTools.begin(), activeTools.end(), type), activeTools.end());
}

std::string EditorPage::ComponentTypeName(ComponentType type) const
{
    for (const auto& tool : allTools) {
        if (tool.type == type) return tool.name;
    }
    return "Unknown";
}

void EditorPage::BuildLibraryHitBoxes()
{
    libraryHitBoxes.clear();

    const float panelTop = TOOLBAR_HEIGHT;
    const float panelH = std::max(120.0f, (float)lastWindowH - TOOLBAR_HEIGHT - STATUSBAR_HEIGHT);
    const float x = 0.0f;
    const float w = LEFT_LIBRARY_WIDTH;
    const float rowH = 27.0f;
    float y = panelTop + 30.0f;

    const float previewH = 132.0f;
    const float activeH = 150.0f;
    const float previewY = std::max(y + 150.0f, panelTop + panelH - previewH - activeH - 14.0f);
    const float activeY = previewY + previewH + 7.0f;
    libraryPreviewRect = {8.0f, previewY, w - 16.0f, previewH};

    std::string query = search.GetText();
    const bool searching = !query.empty();

    for (size_t ci = 0; ci < libraryCategories.size(); ++ci) {
        const auto& cat = libraryCategories[ci];
        bool hasMatch = false;
        for (size_t ti = 0; ti < filteredTools.size(); ++ti) {
            if (filteredTools[ti].category == cat.name) { hasMatch = true; break; }
        }
        if (!searching && !hasMatch) {
            // Empty categories are still shown as tree nodes when there is no search.
        }
        if (searching && !hasMatch) continue;

        SDL_FRect catRect = {6.0f, y, w - 12.0f, rowH};
        libraryHitBoxes.push_back({LibraryHitBox::Kind::Category, (int)ci, catRect});
        y += rowH + 2.0f;

        const bool expanded = searching || cat.expanded;
        if (!expanded) continue;

        for (size_t ti = 0; ti < filteredTools.size(); ++ti) {
            if (filteredTools[ti].category != cat.name) continue;
            if (y + rowH >= previewY - 4.0f) break;

            SDL_FRect toolRect = {12.0f, y, w - 38.0f, rowH - 2.0f};
            SDL_FRect addRect = {w - 31.0f, y, 22.0f, rowH - 2.0f};
            libraryHitBoxes.push_back({LibraryHitBox::Kind::Tool, (int)ti, toolRect});
            libraryHitBoxes.push_back({LibraryHitBox::Kind::ToolAdd, (int)ti, addRect});
            y += rowH;
        }
    }

    // Active components list hit boxes
    float ay = activeY + 28.0f;
    for (size_t i = 0; i < activeTools.size(); ++i) {
        if (ay + 24.0f > panelTop + panelH - 6.0f) break;
        SDL_FRect item = {10.0f, ay, w - 40.0f, 22.0f};
        SDL_FRect remove = {w - 29.0f, ay, 20.0f, 22.0f};
        libraryHitBoxes.push_back({LibraryHitBox::Kind::ActiveTool, (int)i, item});
        libraryHitBoxes.push_back({LibraryHitBox::Kind::ActiveRemove, (int)i, remove});
        ay += 25.0f;
    }
}

void EditorPage::DrawComponentPreview(SDL_Renderer* renderer, const SDL_FRect& rect)
{
    SDL_SetRenderDrawColor(renderer, 248, 248, 248, 255);
    SDL_RenderFillRect(renderer, &rect);
    SDL_SetRenderDrawColor(renderer, 160, 160, 160, 255);
    SDL_RenderRect(renderer, &rect);

    std::string selectedName = ComponentTypeName(selectedTool);
    std::string titleText = "Preview: " + selectedName;
    SDL_Texture* title = TextRenderer::CreateText(renderer, titleText.c_str(), {40,40,40,255});
    if (title) {
        float tw=0, th=0; SDL_GetTextureSize(title,&tw,&th);
        SDL_FRect tp = {rect.x + 7.0f, rect.y + 6.0f, std::min(tw, rect.w-14.0f), th};
        SDL_RenderTexture(renderer, title, nullptr, &tp);
        SDL_DestroyTexture(title);
    }

    auto previewComp = CreateComponent(selectedTool, 0.0f, 0.0f);
    if (!previewComp) return;

    const float availW = rect.w - 20.0f;
    const float availH = rect.h - 42.0f;
    const float sx = availW / std::max(1.0f, previewComp->width);
    const float sy = availH / std::max(1.0f, previewComp->height);
    const float scale = std::min(1.35f, std::min(sx, sy));
    const float scaledW = previewComp->width * scale;
    const float scaledH = previewComp->height * scale;
    const int vx = (int)(rect.x + (rect.w - scaledW) * 0.5f);
    const int vy = (int)(rect.y + 32.0f + (availH - scaledH) * 0.5f);

    SDL_Rect oldViewport{};
    SDL_GetRenderViewport(renderer, &oldViewport);
    float oldScaleX=1.0f, oldScaleY=1.0f;
    SDL_GetRenderScale(renderer, &oldScaleX, &oldScaleY);

    SDL_Rect previewViewport = {
        vx, vy,
        std::max(1, (int)std::ceil(scaledW) + 2),
        std::max(1, (int)std::ceil(scaledH) + 2)
    };
    SDL_SetRenderViewport(renderer, &previewViewport);
    SDL_SetRenderScale(renderer, scale, scale);
    previewComp->Draw(renderer);

    SDL_SetRenderScale(renderer, oldScaleX, oldScaleY);
    SDL_SetRenderViewport(renderer, &oldViewport);
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
// پنجره‌ی ویژگی‌ها (Properties Window - بخش ۴.۷)
//-----------------------------------------
void EditorPage::OpenPropertiesFor(Component* comp)
{
    if (!comp) return;

    propertiesTarget = comp;
    propertiesFields = comp->GetProperties();

    propertiesEditText.clear();
    for (auto& f : propertiesFields) {
        propertiesEditText.push_back(f.value);
    }

    propertiesActiveField = propertiesFields.empty() ? -1 : 0;
    propertiesOpen = true;

    // شروع دریافت ورودی متنی از سیستم‌عامل (بدون این، رویداد
    // SDL_EVENT_TEXT_INPUT اصلاً تولید نمی‌شود)
    if (ownerWindow) {
        SDL_StartTextInput(ownerWindow);
    }
}

void EditorPage::CloseProperties(bool applyChanges)
{
    // تغییرات فقط وقتی روی خودِ قطعه اعمال می‌شوند که کاربر «تایید»
    // کرده باشد؛ در حالت انصراف/Esc هیچ تغییری در مدار اعمال نمی‌شود.
    if (applyChanges && propertiesTarget) {
        propertiesTarget->SetProperties(propertiesEditText);
    }

    propertiesOpen = false;
    propertiesTarget = nullptr;
    propertiesFields.clear();
    propertiesEditText.clear();
    propertiesActiveField = -1;

    if (ownerWindow) {
        SDL_StopTextInput(ownerWindow);
    }
}

void EditorPage::ComputePropertiesLayout(int windowW, int windowH,
                                          SDL_FRect& panel,
                                          std::vector<SDL_FRect>& fieldBoxes,
                                          SDL_FRect& applyBtn,
                                          SDL_FRect& cancelBtn) const
{
    int fieldCount = (int)propertiesFields.size();
    float panelW = 340.0f;
    float fieldH = 34.0f;
    float rowH = fieldH + 32.0f; // فاصله‌ی هر ردیف (برچسب + کادر متن)
    float panelH = 70.0f + fieldCount * rowH + 50.0f;

    float panelX = ((float)windowW - panelW) / 2.0f;
    float panelY = ((float)windowH - panelH) / 2.0f;

    panel = { panelX, panelY, panelW, panelH };

    fieldBoxes.clear();
    float curY = panelY + 46.0f;
    for (int i = 0; i < fieldCount; i++) {
        SDL_FRect box = { panelX + 14.0f, curY + 20.0f, panelW - 28.0f, fieldH };
        fieldBoxes.push_back(box);
        curY += rowH;
    }

    applyBtn  = { panelX + 14.0f, panelY + panelH - 42.0f, 140.0f, 30.0f };
    cancelBtn = { panelX + panelW - 154.0f, panelY + panelH - 42.0f, 140.0f, 30.0f };
}

void EditorPage::DrawPropertiesPanel(SDL_Renderer* renderer, int windowW, int windowH)
{
    if (!propertiesOpen || !propertiesTarget) return;

    // پرده‌ی نیمه‌شفاف پشت پنجره (حالت Modal - بقیه‌ی صفحه غیرقابل کلیک است)
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 120);
    SDL_FRect overlay = { 0, 0, (float)windowW, (float)windowH };
    SDL_RenderFillRect(renderer, &overlay);

    SDL_FRect panel, applyBtn, cancelBtn;
    std::vector<SDL_FRect> fieldBoxes;
    ComputePropertiesLayout(windowW, windowH, panel, fieldBoxes, applyBtn, cancelBtn);

    SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
    SDL_RenderRect(renderer, &panel);

    SDL_Color black = {20, 20, 20, 255};

    // عنوان: نوع/برچسب قطعه‌ای که در حال ویرایش آن هستیم
    std::string title = "Properties: " + propertiesTarget->label;
    SDL_Texture* tTitle = TextRenderer::CreateText(renderer, title.c_str(), black);
    if (tTitle) {
        float tw = 0, th = 0;
        SDL_GetTextureSize(tTitle, &tw, &th);
        SDL_FRect p = { panel.x + 14, panel.y + 12, tw, th };
        SDL_RenderTexture(renderer, tTitle, NULL, &p);
        SDL_DestroyTexture(tTitle);
    }

    // هر فیلد: برچسب بالای کادر + کادر متنیِ قابل‌ویرایش
    for (size_t i = 0; i < propertiesFields.size(); i++) {
        SDL_Texture* tLabel = TextRenderer::CreateText(renderer, propertiesFields[i].label.c_str(), black);
        if (tLabel) {
            float tw = 0, th = 0;
            SDL_GetTextureSize(tLabel, &tw, &th);
            SDL_FRect p = { fieldBoxes[i].x, fieldBoxes[i].y - 22, tw, th };
            SDL_RenderTexture(renderer, tLabel, NULL, &p);
            SDL_DestroyTexture(tLabel);
        }

        bool active = ((int)i == propertiesActiveField);
        SDL_SetRenderDrawColor(renderer, active ? 235 : 255, active ? 245 : 255, 255, 255);
        SDL_RenderFillRect(renderer, &fieldBoxes[i]);
        SDL_SetRenderDrawColor(renderer, active ? 30 : 150, active ? 120 : 150, active ? 220 : 150, 255);
        SDL_RenderRect(renderer, &fieldBoxes[i]);

        std::string shown = propertiesEditText[i];
        if (active) shown += "|"; // نشانگر تایپ ساده (کرسور)
        SDL_Texture* tVal = TextRenderer::CreateText(renderer, shown.c_str(), black);
        if (tVal) {
            float tw = 0, th = 0;
            SDL_GetTextureSize(tVal, &tw, &th);
            SDL_FRect p = { fieldBoxes[i].x + 6, fieldBoxes[i].y + (fieldBoxes[i].h - th) / 2.0f, tw, th };
            SDL_RenderTexture(renderer, tVal, NULL, &p);
            SDL_DestroyTexture(tVal);
        }
    }

    // دکمه‌ی تایید
    SDL_SetRenderDrawColor(renderer, 190, 230, 190, 255);
    SDL_RenderFillRect(renderer, &applyBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &applyBtn);
    SDL_Texture* tApply = TextRenderer::CreateText(renderer, "Apply", black);
    if (tApply) {
        float tw = 0, th = 0;
        SDL_GetTextureSize(tApply, &tw, &th);
        SDL_FRect p = { applyBtn.x + 10, applyBtn.y + 5, tw, th };
        SDL_RenderTexture(renderer, tApply, NULL, &p);
        SDL_DestroyTexture(tApply);
    }

    // دکمه‌ی انصراف
    SDL_SetRenderDrawColor(renderer, 230, 190, 190, 255);
    SDL_RenderFillRect(renderer, &cancelBtn);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderRect(renderer, &cancelBtn);
    SDL_Texture* tCancel = TextRenderer::CreateText(renderer, "Cancel", black);
    if (tCancel) {
        float tw = 0, th = 0;
        SDL_GetTextureSize(tCancel, &tw, &th);
        SDL_FRect p = { cancelBtn.x + 10, cancelBtn.y + 5, tw, th };
        SDL_RenderTexture(renderer, tCancel, NULL, &p);
        SDL_DestroyTexture(tCancel);
    }
}

bool EditorPage::HandlePropertiesClick(int x, int y)
{
    if (!propertiesOpen) return false;

    SDL_FRect panel, applyBtn, cancelBtn;
    std::vector<SDL_FRect> fieldBoxes;
    ComputePropertiesLayout(lastWindowW, lastWindowH, panel, fieldBoxes, applyBtn, cancelBtn);

    // کلیک روی دکمه‌ی تایید: تغییرات را روی قطعه اعمال کن
    if (x >= applyBtn.x && x <= applyBtn.x + applyBtn.w && y >= applyBtn.y && y <= applyBtn.y + applyBtn.h) {
        SaveCurrentStateForUndo();
        CloseProperties(true);
        return true;
    }

    // کلیک روی دکمه‌ی انصراف: بدون اعمال تغییرات ببند
    if (x >= cancelBtn.x && x <= cancelBtn.x + cancelBtn.w && y >= cancelBtn.y && y <= cancelBtn.y + cancelBtn.h) {
        CloseProperties(false);
        return true;
    }

    // کلیک روی یکی از فیلدها: آن فیلد را برای تایپ فعال کن
    for (size_t i = 0; i < fieldBoxes.size(); i++) {
        SDL_FRect& b = fieldBoxes[i];
        if (x >= b.x && x <= b.x + b.w && y >= b.y && y <= b.y + b.h) {
            propertiesActiveField = (int)i;
            return true;
        }
    }

    // کلیک بیرون از پنل (روی پرده‌ی تیره): مثل انصراف عمل کن
    if (!(x >= panel.x && x <= panel.x + panel.w && y >= panel.y && y <= panel.y + panel.h)) {
        CloseProperties(false);
        return true;
    }

    // کلیک روی جای خالیِ داخل پنل: فقط مصرفش کن، کاری نکن
    return true;
}

//-----------------------------------------
// سیستم سیم‌کشی: تشخیص خودکار پایه‌ها (بخش ۵.۱)
//-----------------------------------------
void EditorPage::UpdatePinHighlights()
{
    // در حلقه‌ی اصلی (اینجا: هر رویداد حرکت موس)، فاصله‌ی موس تا موقعیت
    // جهانیِ هر پینِ هر قطعه سنجیده می‌شود تا کاربر ببیند از کجا می‌تواند
    // سیم‌کشی را شروع/تمام کند.
    for (auto& comp : components) {
        for (auto& pin : comp->pins) {
            auto worldPos = comp->GetPinWorldPos(pin);
            pin.checkMouseOver(worldPos.first, worldPos.second, worldMouseX, worldMouseY);
        }
    }
}

void EditorPage::DrawFilledCircle(SDL_Renderer* renderer, float cx, float cy, float radius)
{
    // SDL تابع آماده‌ای برای دایره‌ی توپر ندارد؛ با رسم خطوط افقیِ
    // پیاپی (اسکن‌لاین) یک دایره‌ی توپر تقریبی می‌سازیم.
    int r = (int)std::ceil(radius);
    for (int dy = -r; dy <= r; dy++) {
        float remain = (float)(r * r) - (float)(dy * dy);
        if (remain < 0) continue;
        int dx = (int)std::sqrt(remain);
        SDL_RenderLine(renderer, cx - dx, cy + dy, cx + dx, cy + dy);
    }
}

void EditorPage::DrawPins(SDL_Renderer* renderer)
{
    // توجه: این تابع باید وقتی صدا زده شود که Viewport/Scale بومِ طراحی
    // فعال است (یعنی مختصات پین‌ها، مختصات جهانی/World هستند).
    for (auto& comp : components) {
        for (auto& pin : comp->pins) {
            auto worldPos = comp->GetPinWorldPos(pin);

            if (pin.isHighlighted) {
                // پین هایلایت‌شده: دایره‌ی زرد بزرگ‌تر و پررنگ - یعنی
                // کاربر می‌تواند از این‌جا سیم‌کشی را شروع/تمام کند
                SDL_SetRenderDrawColor(renderer, 255, 200, 0, 255);
                DrawFilledCircle(renderer, worldPos.first, worldPos.second, pin.sensitivityRadius);
                SDL_SetRenderDrawColor(renderer, 160, 110, 0, 255);
                DrawFilledCircle(renderer, worldPos.first, worldPos.second, 1.5f);
            } else {
                // پین عادی: نقطه‌ی کوچک خاکستری
                SDL_SetRenderDrawColor(renderer, 90, 90, 90, 255);
                DrawFilledCircle(renderer, worldPos.first, worldPos.second, 3.0f);
            }
        }
    }
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
    SDL_FRect sidebarArea = {0, TOOLBAR_HEIGHT, LEFT_LIBRARY_WIDTH, (float)lastWindowH - TOOLBAR_HEIGHT - STATUSBAR_HEIGHT};
    SDL_SetRenderDrawColor(renderer, 214, 214, 214, 255);
    SDL_RenderFillRect(renderer, &sidebarArea);

    BuildLibraryHitBoxes();
    SDL_Color dark = {25,25,25,255};

    // Header
    SDL_Texture* head = TextRenderer::CreateText(renderer, "Component Library", dark);
    if (head) { float tw=0,th=0; SDL_GetTextureSize(head,&tw,&th); SDL_FRect p={8,TOOLBAR_HEIGHT+7,tw,th}; SDL_RenderTexture(renderer,head,nullptr,&p); SDL_DestroyTexture(head); }

    for (const auto& hit : libraryHitBoxes) {
        if (hit.kind == LibraryHitBox::Kind::Category) {
            const auto& cat = libraryCategories[hit.index];
            SDL_SetRenderDrawColor(renderer, 195, 195, 195, 255);
            SDL_RenderFillRect(renderer, &hit.rect);
            SDL_SetRenderDrawColor(renderer, 125, 125, 125, 255);
            SDL_RenderRect(renderer, &hit.rect);
            const bool expanded = !search.GetText().empty() || cat.expanded;
            const std::string marker = expanded ? "-" : "+";
            SDL_Texture* mark = TextRenderer::CreateText(renderer, marker.c_str(), dark);
            if (mark) { float tw=0,th=0; SDL_GetTextureSize(mark,&tw,&th); SDL_FRect p={hit.rect.x+6,hit.rect.y+2,tw,th}; SDL_RenderTexture(renderer,mark,nullptr,&p); SDL_DestroyTexture(mark); }
            SDL_Texture* txt = TextRenderer::CreateText(renderer, cat.name.c_str(), dark);
            if (txt) { float tw=0,th=0; SDL_GetTextureSize(txt,&tw,&th); SDL_FRect p={hit.rect.x+22,hit.rect.y+3,std::min(tw,hit.rect.w-30.0f),th}; SDL_RenderTexture(renderer,txt,nullptr,&p); SDL_DestroyTexture(txt); }
        }
        else if (hit.kind == LibraryHitBox::Kind::Tool) {
            const auto& tool = filteredTools[hit.index];
            const bool selected = isPlacingMode && selectedTool == tool.type;
            SDL_SetRenderDrawColor(renderer, selected ? 140 : 235, selected ? 240 : 235, selected ? 240 : 235, 255);
            SDL_RenderFillRect(renderer, &hit.rect);
            SDL_SetRenderDrawColor(renderer, 145,145,145,255); SDL_RenderRect(renderer,&hit.rect);
            SDL_Texture* txt=TextRenderer::CreateText(renderer, tool.name.c_str(), dark);
            if(txt){float tw=0,th=0;SDL_GetTextureSize(txt,&tw,&th);SDL_FRect p={hit.rect.x+6,hit.rect.y+2,std::min(tw,hit.rect.w-10.0f),th};SDL_RenderTexture(renderer,txt,nullptr,&p);SDL_DestroyTexture(txt);}
        }
        else if (hit.kind == LibraryHitBox::Kind::ToolAdd) {
            SDL_SetRenderDrawColor(renderer, IsActiveTool(filteredTools[hit.index].type) ? 170 : 215, IsActiveTool(filteredTools[hit.index].type) ? 235 : 215, 190, 255);
            SDL_RenderFillRect(renderer,&hit.rect); SDL_SetRenderDrawColor(renderer,120,120,120,255); SDL_RenderRect(renderer,&hit.rect);
            SDL_Texture* txt=TextRenderer::CreateText(renderer, IsActiveTool(filteredTools[hit.index].type) ? "*" : "+", dark);
            if(txt){float tw=0,th=0;SDL_GetTextureSize(txt,&tw,&th);SDL_FRect p={hit.rect.x+(hit.rect.w-tw)/2,hit.rect.y+2,tw,th};SDL_RenderTexture(renderer,txt,nullptr,&p);SDL_DestroyTexture(txt);}
        }
        else if (hit.kind == LibraryHitBox::Kind::ActiveTool) {
            const ComponentType type = activeTools[hit.index];
            const bool selected = isPlacingMode && selectedTool == type;
            SDL_SetRenderDrawColor(renderer, selected ? 170 : 235, selected ? 230 : 235, selected ? 190 : 235, 255);
            SDL_RenderFillRect(renderer,&hit.rect); SDL_SetRenderDrawColor(renderer,150,150,150,255); SDL_RenderRect(renderer,&hit.rect);
            SDL_Texture* txt=TextRenderer::CreateText(renderer,ComponentTypeName(type).c_str(),dark);
            if(txt){float tw=0,th=0;SDL_GetTextureSize(txt,&tw,&th);SDL_FRect p={hit.rect.x+6,hit.rect.y+2,std::min(tw,hit.rect.w-8.0f),th};SDL_RenderTexture(renderer,txt,nullptr,&p);SDL_DestroyTexture(txt);}
        }
        else if (hit.kind == LibraryHitBox::Kind::ActiveRemove) {
            SDL_SetRenderDrawColor(renderer,245,180,180,255); SDL_RenderFillRect(renderer,&hit.rect); SDL_SetRenderDrawColor(renderer,150,110,110,255); SDL_RenderRect(renderer,&hit.rect);
            SDL_Texture* txt=TextRenderer::CreateText(renderer,"x",dark);
            if(txt){float tw=0,th=0;SDL_GetTextureSize(txt,&tw,&th);SDL_FRect p={hit.rect.x+(hit.rect.w-tw)/2,hit.rect.y+2,tw,th};SDL_RenderTexture(renderer,txt,nullptr,&p);SDL_DestroyTexture(txt);}
        }
    }

    DrawComponentPreview(renderer, libraryPreviewRect);

    // Active list header and divider
    const float activeY = libraryPreviewRect.y + libraryPreviewRect.h + 7.0f;
    SDL_SetRenderDrawColor(renderer,180,180,180,255);
    SDL_RenderLine(renderer,8,activeY,LEFT_LIBRARY_WIDTH-8,activeY);
    SDL_Texture* ah=TextRenderer::CreateText(renderer,"Active Components",dark);
    if(ah){float tw=0,th=0;SDL_GetTextureSize(ah,&tw,&th);SDL_FRect p={8,activeY+6,tw,th};SDL_RenderTexture(renderer,ah,nullptr,&p);SDL_DestroyTexture(ah);}
}

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
    const int barHeight = (int)STATUSBAR_HEIGHT;

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
    SDL_FRect resetBtn = { (float)(windowW - 92), (float)(windowH - barHeight + 2), 82, (float)(barHeight - 4) };
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
//-----------------------------------------
// Docked Inspector / Properties
//-----------------------------------------
void EditorPage::DrawInspectorPanel(SDL_Renderer* renderer, int windowW, int windowH)
{
    const float x = (float)windowW - RIGHT_INSPECTOR_WIDTH;
    const float y = TOOLBAR_HEIGHT;
    const float h = (float)windowH - TOOLBAR_HEIGHT - STATUSBAR_HEIGHT;

    SDL_SetRenderDrawColor(renderer, 238, 238, 238, 255);
    SDL_FRect panel = { x, y, RIGHT_INSPECTOR_WIDTH, h };
    SDL_RenderFillRect(renderer, &panel);
    SDL_SetRenderDrawColor(renderer, 170, 170, 170, 255);
    SDL_RenderRect(renderer, &panel);

    SDL_Color black = {25,25,25,255};
    SDL_Texture* title = TextRenderer::CreateText(renderer, "Properties", black);
    if (title) {
        float tw=0, th=0; SDL_GetTextureSize(title,&tw,&th);
        SDL_FRect p = {x+12, y+10, tw, th};
        SDL_RenderTexture(renderer,title,NULL,&p);
        SDL_DestroyTexture(title);
    }

    if (!inspectorTarget) {
        SDL_Texture* hint = TextRenderer::CreateText(renderer, "Select a component", {120,120,120,255});
        if (hint) {
            float tw=0,th=0; SDL_GetTextureSize(hint,&tw,&th);
            SDL_FRect p={x+12,y+48,tw,th};
            SDL_RenderTexture(renderer,hint,NULL,&p);
            SDL_DestroyTexture(hint);
        }
        return;
    }

    auto fields = inspectorTarget->GetProperties();
    float cy = y + 48;
    SDL_Texture* typeText = TextRenderer::CreateText(renderer, inspectorTarget->label.c_str(), {0,90,170,255});
    if (typeText) {
        float tw=0,th=0; SDL_GetTextureSize(typeText,&tw,&th);
        SDL_FRect p={x+12,cy,tw,th}; SDL_RenderTexture(renderer,typeText,NULL,&p); SDL_DestroyTexture(typeText);
        cy += 28;
    }

    for (const auto& f : fields) {
        SDL_Texture* l = TextRenderer::CreateText(renderer, f.label.c_str(), black);
        if (l) { float tw=0,th=0; SDL_GetTextureSize(l,&tw,&th); SDL_FRect p={x+12,cy,tw,th}; SDL_RenderTexture(renderer,l,NULL,&p); SDL_DestroyTexture(l); }
        SDL_SetRenderDrawColor(renderer,255,255,255,255);
        SDL_FRect box={x+12,cy+20,RIGHT_INSPECTOR_WIDTH-24,28}; SDL_RenderFillRect(renderer,&box);
        SDL_SetRenderDrawColor(renderer,185,185,185,255); SDL_RenderRect(renderer,&box);
        SDL_Texture* v=TextRenderer::CreateText(renderer,f.value.c_str(),black);
        if(v){float tw=0,th=0;SDL_GetTextureSize(v,&tw,&th); SDL_FRect p={box.x+6,box.y+4,box.w-12,th}; SDL_RenderTexture(renderer,v,NULL,&p); SDL_DestroyTexture(v);}        
        cy += 56;
        if (cy > y+h-80) break;
    }

    SDL_SetRenderDrawColor(renderer,220,235,250,255);
    SDL_FRect editBtn={x+12,y+h-42,RIGHT_INSPECTOR_WIDTH-24,30}; SDL_RenderFillRect(renderer,&editBtn);
    SDL_SetRenderDrawColor(renderer,110,140,170,255); SDL_RenderRect(renderer,&editBtn);
    SDL_Texture* e=TextRenderer::CreateText(renderer,"Edit Properties",black);
    if(e){float tw=0,th=0;SDL_GetTextureSize(e,&tw,&th); SDL_FRect p={editBtn.x+(editBtn.w-tw)/2,editBtn.y+5,tw,th}; SDL_RenderTexture(renderer,e,NULL,&p); SDL_DestroyTexture(e);}
}

bool EditorPage::HandleInspectorClick(int x, int y)
{
    const float panelX = (float)lastWindowW - RIGHT_INSPECTOR_WIDTH;
    const float panelY = TOOLBAR_HEIGHT;
    const float panelH = (float)lastWindowH - TOOLBAR_HEIGHT - STATUSBAR_HEIGHT;
    if (x < panelX || y < panelY || y > panelY + panelH) return false;

    if (inspectorTarget && y >= panelY + panelH - 48) {
        OpenPropertiesFor(inspectorTarget);
        return true;
    }
    return true;
}

// Draw
//-----------------------------------------
void EditorPage::Draw(SDL_Renderer* renderer)
{
    UpdateSearchFilter();

    bool isA3 = (pageSize.find("A3") != std::string::npos);

    // ابعاد منطقی کاغذ را از ابعاد پنجره جدا نگه می‌داریم تا
    // با بزرگ شدن پنجره، Canvas واقعاً فضای بیشتری برای کار داشته باشد.
    int winW = 0, winH = 0;
    SDL_GetRenderOutputSize(renderer, &winW, &winH);
    lastWindowW = winW;
    lastWindowH = winH;

    // A3 کاغذ بزرگ‌تری دارد؛ A4 کمی کوچک‌تر است اما همچنان
    // برای پروژه‌های معمول فضای مناسبی در اختیار می‌گذارد.
    const float paperW = isA3 ? 1600.0f : 1200.0f;
    const float paperH = isA3 ? 1050.0f : 820.0f;

    const float worldPaperW = paperW;
    const float worldPaperH = paperH;

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
    // رسم سیم‌ها قبل از قطعات تا سیم‌ها زیر بدنه‌ی قطعات قرار بگیرند.
    // =========================================================
    wireSystem.Draw(renderer, components);

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

    // رسم پین‌های همه‌ی قطعات (بخش ۵.۱): نقطه‌ی خاکستری معمولی، یا
    // دایره‌ی زرد بزرگ‌تر وقتی موس در محدوده‌ی حساسیتِ آن پین است
    DrawPins(renderer);

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

    // --- نوار ابزار Responsive ---
    // با توجه به عرض پنجره، دکمه‌ها بین Search و Inspector جا می‌گیرند.
    const float inspectorLeft = (float)winW - RIGHT_INSPECTOR_WIDTH;
    const float toolbarRight = inspectorLeft - 8.0f;
    float toolbarX = 392.0f;
    float gap = 4.0f;

    struct ToolbarButtonSpec {
        const char* text;
        float baseWidth;
        SDL_Color fill;
    };

    const ToolbarButtonSpec specs[] = {
        {"Wire", 50.0f, {190,235,200,255}},
        {"Rot", 48.0f, {210,220,245,255}},
        {"Mirror", 58.0f, {245,225,205,255}},
        {"Delete", 56.0f, {250,190,190,255}},
        {"Undo", 48.0f, {225,225,225,255}},
        {"Redo", 48.0f, {225,225,225,255}},
        {"Save", 52.0f, {205,235,215,255}},
        {"Export", 60.0f, {205,235,215,255}},
        {"Run", 48.0f, {180,245,180,255}},
        {"Pause", 56.0f, {255,230,120,255}},
        {"Stop", 50.0f, {245,190,190,255}}
    };

    float totalBase = 0.0f;
    for (const auto& b : specs) totalBase += b.baseWidth;
    totalBase += gap * 10.0f;
    const float available = std::max(0.0f, toolbarRight - toolbarX);
    const float scale = totalBase > available && available > 0.0f ? available / totalBase : 1.0f;

    auto drawTopButton = [&](float x, float w, const char* text, SDL_Color fill) {
        SDL_SetRenderDrawColor(renderer, fill.r, fill.g, fill.b, fill.a);
        SDL_FRect r={x,7,w,30}; SDL_RenderFillRect(renderer,&r);
        SDL_SetRenderDrawColor(renderer,120,120,120,255); SDL_RenderRect(renderer,&r);
        SDL_Texture* t=TextRenderer::CreateText(renderer,text,{20,20,20,255});
        if(t){
            float tw=0,th=0; SDL_GetTextureSize(t,&tw,&th);
            const float maxTextW = std::max(8.0f, w - 6.0f);
            if (tw > maxTextW) tw = maxTextW;
            SDL_FRect p={x+(w-tw)/2,12,tw,th};
            SDL_RenderTexture(renderer,t,NULL,&p);
            SDL_DestroyTexture(t);
        }
    };

    float buttonX[11] = {};
    float buttonW[11] = {};
    for (int i = 0; i < 11; ++i) {
        buttonX[i] = toolbarX;
        buttonW[i] = specs[i].baseWidth * scale;
        drawTopButton(buttonX[i], buttonW[i], specs[i].text, specs[i].fill);
        toolbarX += buttonW[i] + gap * scale;
    }

    SDL_Color black = {0, 0, 0, 255};

    // Right-side run controls are now drawn in the toolbar/status area.
    // نوار وضعیت پایین صفحه (مختصات + زوم + دکمه Reset)
    DrawStatusBar(renderer, winW, winH);

    // پنل Properties ثابت در سمت راست
    DrawInspectorPanel(renderer, winW, winH);

    // پنجره‌ی ویژگی‌ها (اگر باز باشد) باید روی همه‌چیز رسم شود (بخش ۴.۷)
    DrawPropertiesPanel(renderer, winW, winH);

    if (exportRequested) {
        ExportToImage(renderer);
        exportRequested = false;
    }
}

//-----------------------------------------
// Mouse & Keyboard
//-----------------------------------------
EditorMenuAction EditorPage::HandleClick(int x, int y, int clickCount)
{
    // اگر پنجره‌ی ویژگی‌ها باز است، تمام کلیک‌ها را خودش مصرف می‌کند
    // (حالت Modal) — هیچ تعامل دیگری با بوم/سایدبار/نوار ابزار مجاز نیست.
    if (propertiesOpen) {
        HandlePropertiesClick(x, y);
        return (EditorMenuAction)0;
    }

    // دکمه‌ی «بازگشت به ۱۰۰٪» در نوار وضعیت پایین صفحه (فضای صفحه)
    SDL_FRect resetBtn = { (float)(lastWindowW - 92), (float)(lastWindowH - STATUSBAR_HEIGHT + 2), 82, STATUSBAR_HEIGHT - 4 };
    if (x >= resetBtn.x && x <= resetBtn.x + resetBtn.w && y >= resetBtn.y && y <= resetBtn.y + resetBtn.h) {
        ResetView();
        return (EditorMenuAction)0;
    }

    EditorMenuAction action = menu.HandleClick(x, y);
    search.HandleClick(x, y);

    // ---------------- Top toolbar (Responsive) ----------------
    const float inspectorLeft = (float)lastWindowW - RIGHT_INSPECTOR_WIDTH;
    const float toolbarRight = inspectorLeft - 8.0f;
    float toolbarX = 392.0f;
    const float gap = 4.0f;

    struct ToolbarHit { const char* text; float baseWidth; };
    const ToolbarHit hits[] = {
        {"Wire",50.0f},{"Rot",48.0f},{"Mirror",58.0f},{"Delete",56.0f},{"Undo",48.0f},{"Redo",48.0f},
        {"Save",52.0f},{"Export",60.0f},{"Run",48.0f},{"Pause",56.0f},{"Stop",50.0f}
    };
    float totalBase = 0.0f;
    for (const auto& b : hits) totalBase += b.baseWidth;
    totalBase += gap * 10.0f;
    const float available = std::max(0.0f, toolbarRight - toolbarX);
    const float scale = totalBase > available && available > 0.0f ? available / totalBase : 1.0f;

    float hitX[11] = {};
    float hitW[11] = {};
    for (int i = 0; i < 11; ++i) {
        hitX[i] = toolbarX;
        hitW[i] = hits[i].baseWidth * scale;
        if (x >= hitX[i] && x <= hitX[i] + hitW[i] && y >= 7 && y <= 37) {
            switch (i) {
                case 0: SetWireMode(!isWireMode); return (EditorMenuAction)0;
                case 1:
                    SaveCurrentStateForUndo();
                    for (auto& c : components) if (c->isSelected) c->angle = (c->angle + 90) % 360;
                    return (EditorMenuAction)0;
                case 2:
                    SaveCurrentStateForUndo();
                    for (auto& c : components) if (c->isSelected) c->isMirrored = !c->isMirrored;
                    return (EditorMenuAction)0;
                case 3: {
                    bool hasSelection = false;
                    for (const auto& c : components) if (c->isSelected) { hasSelection = true; break; }
                    for (const auto& w : wireSystem.wires) if (w.isSelected) { hasSelection = true; break; }
                    if (hasSelection) { SaveCurrentStateForUndo(); DeleteSelectedItems(); }
                    return (EditorMenuAction)0;
                }
                case 4: Undo(); return (EditorMenuAction)0;
                case 5: Redo(); return (EditorMenuAction)0;
                case 6: return EDITOR_SAVE_PROJECT;
                case 7: exportRequested = true; return (EditorMenuAction)0;
                case 8: return (EditorMenuAction)0;
                case 9: return (EditorMenuAction)0;
                case 10:return (EditorMenuAction)0;
            }
        }
        toolbarX += hitW[i] + gap * scale;
    }

    // ---------------- Component Library / Tree / Active list ----------------
    if (x < (int)LEFT_LIBRARY_WIDTH && y > (int)TOOLBAR_HEIGHT) {
        BuildLibraryHitBoxes();
        for (const auto& hit : libraryHitBoxes) {
            if (x < hit.rect.x || x > hit.rect.x + hit.rect.w || y < hit.rect.y || y > hit.rect.y + hit.rect.h) continue;

            if (hit.kind == LibraryHitBox::Kind::Category) {
                if (search.GetText().empty()) libraryCategories[hit.index].expanded = !libraryCategories[hit.index].expanded;
                return (EditorMenuAction)0;
            }
            if (hit.kind == LibraryHitBox::Kind::Tool) {
                selectedTool = filteredTools[hit.index].type;
                SetWireMode(false);
                isPlacingMode = true;
                return (EditorMenuAction)0;
            }
            if (hit.kind == LibraryHitBox::Kind::ToolAdd) {
                AddActiveTool(filteredTools[hit.index].type);
                selectedTool = filteredTools[hit.index].type;
                return (EditorMenuAction)0;
            }
            if (hit.kind == LibraryHitBox::Kind::ActiveTool) {
                selectedTool = activeTools[hit.index];
                SetWireMode(false);
                isPlacingMode = true;
                return (EditorMenuAction)0;
            }
            if (hit.kind == LibraryHitBox::Kind::ActiveRemove) {
                RemoveActiveTool(activeTools[hit.index]);
                return (EditorMenuAction)0;
            }
        }
        return action;
    }

    // از این‌جا به بعد تعاملات مربوط به داخل بوم طراحی است؛
    // مختصات صفحه را به مختصات جهانی تبدیل می‌کنیم تا زوم/پن را در نظر بگیرند
    float wx, wy;
    ScreenToWorld(x, y, wx, wy);

    // Docked right inspector consumes clicks in its own area.
    if (HandleInspectorClick(x, y)) return (EditorMenuAction)0;

    // ---------------- سیستم سیم‌کشی ----------------
    if (isWireMode && x >= (int)canvasBaseX && x < (int)(lastWindowW - RIGHT_INSPECTOR_WIDTH) && y >= (int)canvasBaseY && y < (int)(lastWindowH - STATUSBAR_HEIGHT)) {
        return HandleWireClick(wx, wy) ? (EditorMenuAction)0 : action;
    }

    // ---------------- جای‌گذاری قطعه با Snap to Grid ----------------
    if (isPlacingMode && x >= (int)canvasBaseX && x < (int)(lastWindowW - RIGHT_INSPECTOR_WIDTH) && y >= (int)canvasBaseY && y < (int)(lastWindowH - STATUSBAR_HEIGHT)) {

        float snapX = std::round(wx / (float)gridSpacing) * gridSpacing;
        float snapY = std::round(wy / (float)gridSpacing) * gridSpacing;

        SaveCurrentStateForUndo();

        std::unique_ptr<Component> newComponent = CreateComponent(selectedTool, snapX, snapY);
        if (newComponent) {
            AssignComponentId(newComponent.get());
            components.push_back(std::move(newComponent));
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

    // ---------------- دابل‌کلیک روی قطعه: باز کردن پنجره‌ی ویژگی‌ها (بخش ۴.۷) ----------------
    // SDL3 خودش تعداد کلیک‌های پیاپی را در event.button.clicks می‌شمارد،
    // پس نیازی به تایمر دستی برای تشخیص دابل‌کلیک نیست.
    if (!isPlacingMode && clickCount >= 2 && x >= (int)canvasBaseX && x < (int)(lastWindowW - RIGHT_INSPECTOR_WIDTH) && y >= (int)canvasBaseY && y < (int)(lastWindowH - STATUSBAR_HEIGHT)) {
        for (auto it = components.rbegin(); it != components.rend(); ++it) {
            if ((*it)->Contains(wx, wy)) {
                OpenPropertiesFor(it->get());
                return (EditorMenuAction)0;
            }
        }
    }

    // ---------------- تعامل با قطعات موجود (کلیک روی کلید/سوییچ و ...) ----------------
    if (!isPlacingMode) {
        for (auto& comp : components) {
            if (comp->HandleClick(wx, wy)) {
                inspectorTarget = comp.get();
                break;
            }
        }
    }

    // ---------------- انتخاب سیم ----------------
    if (!isPlacingMode && x >= (int)canvasBaseX && x < (int)(lastWindowW - RIGHT_INSPECTOR_WIDTH) && y >= (int)canvasBaseY && y < (int)(lastWindowH - STATUSBAR_HEIGHT)) {
        int wireIndex = -1;
        WirePoint hitPoint;
        if (wireSystem.FindWireHit(wx, wy, wireIndex, hitPoint)) {
            for (auto& c : components) c->isSelected = false;
            wireSystem.ClearSelection();
            if (wireIndex >= 0 && wireIndex < (int)wireSystem.wires.size()) {
                wireSystem.wires[wireIndex].isSelected = true;
            }
            isDragging = false;
            isSelectingBox = false;
            return (EditorMenuAction)0;
        }
    }

    // ---------------- انتخاب قطعه / شروع درگ / شروع مستطیل انتخاب گروهی ----------------
    if (!isPlacingMode && x >= (int)canvasBaseX && x < (int)(lastWindowW - RIGHT_INSPECTOR_WIDTH) && y >= (int)canvasBaseY && y < (int)(lastWindowH - STATUSBAR_HEIGHT)) {
        bool clickedOnComponent = false;

        for (auto it = components.rbegin(); it != components.rend(); ++it) {
            if ((*it)->Contains(wx, wy)) {
                clickedOnComponent = true;
                if (!(*it)->isSelected) {
                    for (auto& c : components) c->isSelected = false;
                    wireSystem.ClearSelection();
                    (*it)->isSelected = true;
                }
                isDragging = true;
                lastMouseX = wx;
                lastMouseY = wy;

                // نکته‌ی مهم: چون از این‌جا به بعد مختصات قطعه در هر فریمِ
                // حرکت موس تغییر می‌کند، باید وضعیتِ «قبل از جابجایی» را
                // همین الان (قبل از هر تغییری) برای Undo ذخیره کنیم؛
                // در غیر این صورت Undo بعد از یک درگ کامل، به موقعیت
                // درست قبل از جابجایی برنمی‌گردد.
                SaveCurrentStateForUndo();

                // ثبت موقعیت اولیه‌ی (بدون Snap) همه‌ی قطعات انتخاب‌شده و
                // مختصات شروع موس، تا بتوانیم در هر فریمِ جابجایی، افستِ
                // خام موس را نسبت به موقعیت اصلی محاسبه و Snap کنیم
                // (به‌جای جمع‌کردن دلتاهای کوچک که باعث انحراف می‌شود).
                dragOrigins.clear();
                for (auto& c : components) {
                    if (c->isSelected) {
                        dragOrigins.push_back({ c.get(), c->x, c->y });
                    }
                }
                dragStartMouseX = wx;
                dragStartMouseY = wy;
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

    inspectorTarget = nullptr;
    for (auto it = components.rbegin(); it != components.rend(); ++it) {
        if ((*it)->isSelected) { inspectorTarget = it->get(); break; }
    }

    return action;
}

EditorMenuAction EditorPage::HandleKeyboard(SDL_Event event)
{
    // ---------------------------------------------------------------
    // اگر پنجره‌ی ویژگی‌ها باز است، کیبورد فقط برای ویرایش فیلد فعال
    // است — نه سرچ‌باکس و نه هیچ‌کدام از میانبرهای دیگر (حالت Modal).
    // ---------------------------------------------------------------
    if (propertiesOpen) {
        if (event.type == SDL_EVENT_TEXT_INPUT) {
            if (propertiesActiveField >= 0 && propertiesActiveField < (int)propertiesEditText.size()) {
                propertiesEditText[propertiesActiveField] += event.text.text;
            }
            return (EditorMenuAction)0;
        }

        if (event.type == SDL_EVENT_KEY_DOWN) {
            if (event.key.key == SDLK_BACKSPACE) {
                if (propertiesActiveField >= 0 && propertiesActiveField < (int)propertiesEditText.size()) {
                    std::string& s = propertiesEditText[propertiesActiveField];
                    if (!s.empty()) s.pop_back();
                }
                return (EditorMenuAction)0;
            }
            if (event.key.key == SDLK_TAB) {
                // جابه‌جایی بین فیلدها با Tab
                if (!propertiesEditText.empty()) {
                    propertiesActiveField = (propertiesActiveField + 1) % (int)propertiesEditText.size();
                }
                return (EditorMenuAction)0;
            }
            if (event.key.key == SDLK_RETURN || event.key.key == SDLK_KP_ENTER) {
                // Enter مثل زدن دکمه‌ی Apply عمل می‌کند
                SaveCurrentStateForUndo();
                CloseProperties(true);
                return (EditorMenuAction)0;
            }
            if (event.key.key == SDLK_ESCAPE) {
                // Esc مثل زدن دکمه‌ی Cancel عمل می‌کند (بدون اعمال تغییرات)
                CloseProperties(false);
                return (EditorMenuAction)0;
            }
        }

        return (EditorMenuAction)0;
    }

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
            if (!anySelected) {
                for (const auto& w : wireSystem.wires) {
                    if (w.isSelected) { anySelected = true; break; }
                }
            }
            if (anySelected) {
                SaveCurrentStateForUndo();
                DeleteSelectedItems();
            }
            return (EditorMenuAction)0;
        }

        // ---------------- Esc : لغو حالت جای‌گذاری قطعه ----------------
        if (event.key.key == SDLK_ESCAPE) {
            isPlacingMode = false;
            if (isWireMode) wireSystem.Cancel();
            return (EditorMenuAction)0;
        }

        // ---------------- کلید W: ابزار سیم‌کشی ----------------
        if (!ctrlHeld && event.key.key == SDLK_W) {
            SetWireMode(!isWireMode);
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

void EditorPage::DeleteSelectedItems()
{
    std::vector<int> deletedComponentIds;
    for (const auto& c : components) {
        if (c && c->isSelected) deletedComponentIds.push_back(c->id);
    }

    components.erase(
        std::remove_if(components.begin(), components.end(),
            [](const std::unique_ptr<Component>& c) { return c && c->isSelected; }),
        components.end()
    );

    for (int id : deletedComponentIds) {
        wireSystem.DeleteWiresConnectedToComponent(id, components);
    }

    wireSystem.DeleteSelected(components);
    if (inspectorTarget) {
        bool stillAlive = false;
        for (const auto& c : components) if (c.get() == inspectorTarget) { stillAlive = true; break; }
        if (!stillAlive) inspectorTarget = nullptr;
    }
}

void EditorPage::ClearWorkspace()
{
    components.clear();
    wireSystem.Clear();
    isPlacingMode = false;
    isWireMode = false;
    nextComponentId = 1;
    dragOrigins.clear();
    isDragging = false;
    isSelectingBox = false;
    inspectorTarget = nullptr;
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

    file << SaveStateToString();
    file.close();
    std::cout << "Workspace saved successfully to: " << filepath << std::endl;
}

void EditorPage::LoadWorkspace(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cout << "Error: Could not open file " << filepath << std::endl;
        return;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    file.close();

    LoadStateFromString(buffer.str());
    std::cout << "Workspace loaded successfully from: " << filepath << std::endl;
}

//-----------------------------------------
// Undo / Redo
//-----------------------------------------
std::string EditorPage::SaveStateToString()
{
    std::stringstream ss;
    ss << "EDITOR_STATE_V2\n";
    ss << "PAGE " << pageSize << "\n";
    ss << "NEXT_COMPONENT_ID " << nextComponentId << "\n";
    ss << "COMPONENTS " << components.size() << "\n";

    for (const auto& comp : components) {
        ss << "C " << comp->id << " " << (int)comp->type << " "
           << comp->x << " " << comp->y << " "
           << comp->angle << " " << comp->isMirrored << "\n";

        const auto props = comp->GetProperties();
        ss << "P " << props.size() << "\n";
        for (const auto& prop : props) {
            ss << std::quoted(prop.value) << "\n";
        }
    }

    ss << "ACTIVE_TOOLS " << activeTools.size() << "\n";
    for (ComponentType type : activeTools) {
        ss << static_cast<int>(type) << "\n";
    }

    ss << wireSystem.Serialize();
    return ss.str();
}

void EditorPage::LoadStateFromString(const std::string& state)
{
    std::stringstream ss(state);
    std::string firstToken;

    if (!(ss >> firstToken)) return;

    // New format.
    if (firstToken == "EDITOR_STATE_V2") {
        ClearWorkspace();

        std::string token;
        size_t componentCount = 0;

        while (ss >> token) {
            if (token == "PAGE") {
                ss >> pageSize;
            } else if (token == "NEXT_COMPONENT_ID") {
                ss >> nextComponentId;
            } else if (token == "COMPONENTS") {
                ss >> componentCount;

                for (size_t i = 0; i < componentCount; ++i) {
                    int id = -1;
                    int typeInt = -1;
                    float x = 0.0f, y = 0.0f;
                    int angle = 0;
                    bool mirrored = false;

                    ss >> token;
                    if (token != "C") return;

                    ss >> id >> typeInt >> x >> y >> angle >> mirrored;

                    auto component = CreateComponent((ComponentType)typeInt, x, y);
                    if (!component) continue;

                    component->angle = angle;
                    component->isMirrored = mirrored;

                    // Optional persistent component properties. Older V2 files
                    // without a P block remain loadable.
                    const std::streampos afterComponent = ss.tellg();
                    std::string propertyToken;
                    if (ss >> propertyToken) {
                        if (propertyToken == "P") {
                            size_t propertyCount = 0;
                            ss >> propertyCount;
                            std::vector<std::string> values;
                            values.reserve(propertyCount);
                            for (size_t pi = 0; pi < propertyCount; ++pi) {
                                std::string value;
                                ss >> std::quoted(value);
                                values.push_back(value);
                            }
                            component->SetProperties(values);
                        } else {
                            ss.clear();
                            ss.seekg(afterComponent);
                        }
                    }

                    AssignComponentId(component.get(), id);
                    components.push_back(std::move(component));
                }
            } else if (token == "ACTIVE_TOOLS") {
                size_t activeCount = 0;
                ss >> activeCount;
                activeTools.clear();
                for (size_t ai = 0; ai < activeCount; ++ai) {
                    int typeInt = -1;
                    ss >> typeInt;
                    if (typeInt >= 0) activeTools.push_back(static_cast<ComponentType>(typeInt));
                }
            } else if (token == "WIRE_SYSTEM_V1") {
                // WireSystem::Deserialize expects the version token itself.
                // We already consumed it, so parse the remainder manually by
                // reconstructing a tiny stream with the token included.
                std::stringstream wireStream;
                wireStream << "WIRE_SYSTEM_V1\n";
                wireStream << ss.rdbuf();
                wireSystem.Deserialize(wireStream);
                break;
            }
        }

        if (nextComponentId <= 0) nextComponentId = 1;
        for (const auto& c : components) {
            if (c->id >= nextComponentId) nextComponentId = c->id + 1;
        }
        wireSystem.RebuildJunctions(components);
        return;
    }

    // Backward compatibility with the old five-column format.
    ClearWorkspace();
    ss.clear();
    ss.str(state);

    int typeInt, angle;
    float x, y;
    bool isMirrored;

    while (ss >> typeInt >> x >> y >> angle >> isMirrored) {
        auto component = CreateComponent((ComponentType)typeInt, x, y);
        if (!component) continue;

        component->angle = angle;
        component->isMirrored = isMirrored;
        AssignComponentId(component.get());
        components.push_back(std::move(component));
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
void EditorPage::HandleMouseMotion(int x, int y)
{
    HandleMouseMove(x, y);
}

void EditorPage::HandleMouseMove(int x, int y) {
    currentMouseX = x;
    currentMouseY = y;

    float wx, wy;
    ScreenToWorld(x, y, wx, wy);
    worldMouseX = wx;
    worldMouseY = wy;

    // بخش ۵.۱ - تشخیص خودکار پایه‌ها: در هر حرکت موس (حلقه‌ی اصلی رویداد)
    // فاصله‌ی موس تا همه‌ی پین‌های همه‌ی قطعات سنجیده و هایلایت می‌شود.
    UpdatePinHighlights();
    UpdateWirePreview();

    if (isPanning) {
        UpdatePan(x, y);
        return;
    }

    if (isDragging) {
        // افستِ خامِ کل حرکت موس نسبت به نقطه‌ی شروع درگ (بدون Snap)
        float totalDx = wx - dragStartMouseX;
        float totalDy = wy - dragStartMouseY;

        // برای هر قطعه‌ی انتخاب‌شده: موقعیت اصلی + افست خام = موقعیت هدف،
        // سپس همان‌جا (زنده، حین کشیدن) به نزدیک‌ترین نقطه‌ی شبکه Snap می‌شود.
        for (auto& origin : dragOrigins) {
            float targetX = origin.origX + totalDx;
            float targetY = origin.origY + totalDy;

            origin.comp->x = std::round(targetX / (float)gridSpacing) * gridSpacing;
            origin.comp->y = std::round(targetY / (float)gridSpacing) * gridSpacing;
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
        // موقعیت نهایی قطعات از قبل (در HandleMouseMove) روی شبکه Snap
        // شده؛ و وضعیت Undo هم از ابتدای درگ ذخیره شده — اینجا فقط
        // پاک‌سازی می‌کنیم.
        dragOrigins.clear();
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
