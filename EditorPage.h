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
    void CancelPlacing() { isPlacingMode = false; }

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
};
#endif
