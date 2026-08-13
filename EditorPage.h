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
#include "WireSystem.h"

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
    bool isWireMode = false;
    int nextComponentId = 1;

    WireSystem wireSystem;

    // مختصات خام موس روی صفحه (Screen Space) - برای تعامل با UI ثابت (سایدبار، دکمه‌ها)
    int currentMouseX = 0;
    int currentMouseY = 0;

    // مختصات موس در فضای جهانیِ بوم (World Space) - تحت تاثیر زوم و پن
    float worldMouseX = 0.0f;
    float worldMouseY = 0.0f;

    // آخرین اندازه‌ی پنجره که در Draw() ثبت می‌شود (برای تشخیص کلیک روی دکمه‌های نوار وضعیت)
    int lastWindowW = 950;
    int lastWindowH = 600;

    void DrawGrid(SDL_Renderer* renderer);
    void DrawSidebar(SDL_Renderer* renderer);
    void DrawStatusBar(SDL_Renderer* renderer, int windowW, int windowH);
    void DrawOriginMarker(SDL_Renderer* renderer);
    void UpdateSearchFilter(); // تابع جدید برای فیلتر کردن قطعات
    std::unique_ptr<Component> CreateComponent(ComponentType type, float x, float y);
    Component* FindComponentById(int id) const;
    void AssignComponentId(Component* component, int forcedId = -1);
    void DeleteSelectedItems();
    bool HandleWireClick(float wx, float wy);
    void UpdateWirePreview();

    // ================================================================
    // ---------- سیستم سیم‌کشی: تشخیص خودکار پایه‌ها (بخش ۵.۱) ----------
    // ================================================================
    // در هر حرکت موس فراخوانی می‌شود: فاصله‌ی موس تا موقعیت جهانیِ هر
    // پینِ هر قطعه را می‌سنجد و isHighlighted آن پین را به‌روز می‌کند.
    void UpdatePinHighlights();

    // رسم بصری همه‌ی پین‌ها (نقطه‌ی کوچک خاکستری، یا دایره‌ی زرد بزرگ‌تر
    // وقتی هایلایت شده‌اند) — باید داخل بلاکِ تبدیل‌شده‌ی بومِ طراحی
    // (Viewport/Scale فعال) فراخوانی شود.
    void DrawPins(SDL_Renderer* renderer);

    // رسم یک دایره‌ی توپر ساده با اسکن‌لاین افقی (SDL هیچ تابع آماده‌ای
    // برای دایره ندارد)
    void DrawFilledCircle(SDL_Renderer* renderer, float cx, float cy, float radius);

    // ================================================================
    // ---------------------- Zoom & Pan (جدید) ----------------------
    // ================================================================
    float zoom = 1.0f;
    const float minZoom = 0.25f;
    const float maxZoom = 3.0f;

    float panOffsetX = 0.0f;
    float panOffsetY = 0.0f;
    const float panLimit = 2500.0f; // محدوده‌ی مجاز جابه‌جایی بوم (بر حسب پیکسل صفحه)

    bool isPanning = false;
    float panMouseStartX = 0.0f, panMouseStartY = 0.0f;
    float panOffsetStartX = 0.0f, panOffsetStartY = 0.0f;

    // مبدأ بوم روی صفحه؛ همان فاصله‌ی سایدبار چپ (100) و نوار ابزار بالا (50)
    const float canvasBaseX = 100.0f;
    const float canvasBaseY = 50.0f;

    // فاصله‌ی خطوط شبکه در مختصات جهانی. با تغییر همین مقدار، فاصله شبکه
    // در کل برنامه (رسم + Snap) قابل تنظیم است.
    const int gridSpacing = 20;

    // ================================================================
    // ------------------- پنجره‌ی ویژگی‌ها (بخش ۴.۷) -------------------
    // ================================================================
    SDL_Window* ownerWindow = nullptr; // برای SDL_StartTextInput/SDL_StopTextInput

    bool propertiesOpen = false;
    Component* propertiesTarget = nullptr;
    std::vector<PropertyField> propertiesFields; // برای برچسب هر فیلد (از GetProperties)
    std::vector<std::string> propertiesEditText; // بافر متنیِ در حال ویرایش هر فیلد
    int propertiesActiveField = -1;              // فیلدی که الان تایپ در آن وارد می‌شود

    void OpenPropertiesFor(Component* comp);
    void CloseProperties(bool applyChanges);
    void DrawPropertiesPanel(SDL_Renderer* renderer, int windowW, int windowH);
    bool HandlePropertiesClick(int x, int y);

    // چیدمانِ پنجره را یک‌جا محاسبه می‌کند تا رسم و کلیک دقیقاً هم‌جهت باشند
    void ComputePropertiesLayout(int windowW, int windowH,
                                  SDL_FRect& panel,
                                  std::vector<SDL_FRect>& fieldBoxes,
                                  SDL_FRect& applyBtn,
                                  SDL_FRect& cancelBtn) const;

public:
    EditorPage(SDL_Window* window);
    std::string pageSize = "A4"; // متغیر جدید برای نگهداری سایز صفحه

    void ClearWorkspace();
    void SaveWorkspace(const std::string& filepath);
    void LoadWorkspace(const std::string& filepath);
    void Draw(SDL_Renderer* renderer);
    bool exportRequested = false;
    void ExportToImage(SDL_Renderer* renderer);
    // clickCount از event.button.clicks در SDL3 می‌آید (۱=تک‌کلیک، ۲=دابل‌کلیک)
    // و برای باز کردن پنجره‌ی ویژگی‌ها با دابل‌کلیک روی قطعه استفاده می‌شود.
    EditorMenuAction HandleClick(int x, int y, int clickCount = 1);
    void HandleMouseMotion(int x, int y);
    // خروجی این تابع برای میانبرهایی مثل Ctrl+S استفاده می‌شود که باید
    // به main.cpp اطلاع دهند (مثلاً برای ذخیره‌ی واقعی فایل روی دیسک)
    EditorMenuAction HandleKeyboard(SDL_Event event);

    std::vector<std::string> undoStack;
    std::vector<std::string> redoStack;
    std::string SaveStateToString();
    void LoadStateFromString(const std::string& state);
    void SaveCurrentStateForUndo();
    void Undo();
    void Redo();

    // --- متغیرهای انتخاب گروهی و جابه‌جایی (این مختصات اکنون در فضای جهانی هستند) ---
    bool isDragging = false;
    bool isSelectingBox = false;
    float selStartX = 0, selStartY = 0;
    float lastMouseX = 0, lastMouseY = 0;

    // ---------------------------------------------------------------
    // Snap زنده هنگام درگ: موقعیت اصلیِ (بدون Snap) هر قطعه‌ی انتخاب‌شده
    // را در لحظه‌ی شروع درگ ذخیره می‌کنیم تا بتوانیم افستِ خامِ موس را
    // نسبت به آن حساب و در هر فریم روی شبکه Snap کنیم — بدون انحراف.
    // ---------------------------------------------------------------
    struct DragOrigin {
        Component* comp;
        float origX;
        float origY;
    };
    std::vector<DragOrigin> dragOrigins;
    float dragStartMouseX = 0.0f;
    float dragStartMouseY = 0.0f;

    // توابع موس
    void HandleMouseMove(int x, int y);
    void HandleMouseRelease(int x, int y);

    // لغو حالت جای‌گذاری قطعه (مثلاً با کلیک راست موس) — کاربر می‌تواند
    // بعد از قرار دادن هر تعداد نمونه که خواست، با این تابع از حالت
    // جای‌گذاری خارج شود.
    void CancelPlacing() { isPlacingMode = false; wireSystem.Cancel(); }

    // ================================================================
    // -------------------- توابع جدید Zoom / Pan ---------------------
    // ================================================================

    // فراخوانی شود وقتی event.type == SDL_EVENT_MOUSE_WHEEL
    void HandleMouseWheel(float wheelY, int mouseX, int mouseY);

    // شروع/ادامه/پایان جابه‌جایی بوم (مثلا با دکمه وسط موس)
    void StartPan(int x, int y);
    void UpdatePan(int x, int y);
    void StopPan();
    bool IsPanningActive() const { return isPanning; }

    // بازگشت زوم و پن به حالت پیش‌فرض (۱۰۰٪ و بدون جابه‌جایی)
    void ResetView();

    // تبدیل مختصات صفحه (پیکسل پنجره) به مختصات جهانی بوم
    void ScreenToWorld(int sx, int sy, float& wx, float& wy) const;
    bool IsWireMode() const { return isWireMode; }
    void SetWireMode(bool enabled);
};
#endif
