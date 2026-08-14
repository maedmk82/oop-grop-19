#ifndef COMPONENT_H
#define COMPONENT_H

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <cmath>
#include <utility>
#include <sstream>

// -----------------------------------------
// 1. انواع قطعات
// -----------------------------------------
enum class ComponentType {
    // منابع اصلی
    GND, DC_SOURCE, BATTERY, CLOCK,LOGIC_STATE,
    // قطعات غیرفعال
    RESISTOR, CAPACITOR,
    // تعاملی و خروجی
    SWITCH, PUSH_BUTTON, LED, SEVEN_SEGMENT,
    // گیت‌های منطقی و مدارهای ترتیبی
    GATE_AND, GATE_OR,GATE_NOR, GATE_NOT, GATE_XOR, GATE_NAND, FLIP_FLOP_D
};

// -----------------------------------------
// 2. ساختار پین‌ها (بخش ۵.۱ - تشخیص خودکار پایه‌ها)
// -----------------------------------------
struct Pin {
    std::string name;
    float offsetX, offsetY; // موقعیت محلیِ پین نسبت به قطعه (قبل از چرخش/قرینه/جابجایی)
    bool isOutput;
    float voltage = 0.0f;   // مقدار ولتاژ (0 ولت یا 5 ولت منطقی)

    // ---- تشخیص خودکار نزدیک‌شدن موس به پین ----
    bool isHighlighted = false;
    float sensitivityRadius = 6.0f; // شعاع حساسیت (در مختصات جهانیِ بوم)

    // worldX/worldY: موقعیت واقعیِ این پین روی بوم (بعد از اعمال چرخش/
    // قرینه/جابجایی قطعه‌ی مادر) — چون Pin به‌تنهایی از موقعیت قطعه خبر
    // ندارد، این مختصات باید توسط قطعه‌ی مالکش (با LocalToWorld) محاسبه
    // و به این تابع پاس داده شود.
    // mouseX/mouseY: مختصات جهانیِ فعلیِ نشانگر موس.
    bool checkMouseOver(float worldX, float worldY, float mouseX, float mouseY) {
        float dx = mouseX - worldX;
        float dy = mouseY - worldY;
        float dist = std::sqrt(dx * dx + dy * dy);

        isHighlighted = (dist <= sensitivityRadius);
        return isHighlighted;
    }
};

// -----------------------------------------
// 2b. یک فیلد قابل‌ویرایش در پنجره‌ی ویژگی‌ها (Properties Window - بخش ۴.۷)
// -----------------------------------------
struct PropertyField {
    std::string label; // برچسبی که در پنجره نشان داده می‌شود، مثلا "Resistance (Ohm)"
    std::string value;  // مقدار فعلی به‌صورت متن، برای نمایش/ویرایش در کادر متنی
};

// -----------------------------------------
// 3. کلاس پایه (والد) - باید قبل از فرزندان باشد
// -----------------------------------------
class Component {
public:
    int id;
    std::string label;
    float x, y;            // موقعیت روی صفحه ویرایشگر
    float width = 60.0f;
    float height = 40.0f;
    ComponentType type;
    std::vector<Pin> pins;

    // ---------------------------------------------------------------
    // تبدیل یک نقطه‌ی محلی (همان مختصاتی که در DrawLine و در
    // offsetX/offsetY پین‌ها استفاده می‌شود) به مختصات جهانیِ بوم، با در
    // نظر گرفتن موقعیت (x,y)، چرخش (angle) و قرینه‌بودن (isMirrored)
    // قطعه. این تابع پیش‌تر به‌صورت یک لامبدای محلی داخل DrawLine مخفی
    // بود؛ حالا عمومی است تا هم DrawLine و هم محاسبه‌ی موقعیت جهانیِ
    // پین‌ها (برای تشخیص خودکار پایه‌ها - بخش ۵.۱) از همان منطق واحد
    // استفاده کنند.
    // ---------------------------------------------------------------
    std::pair<float, float> LocalToWorld(float lx, float ly) const {
        float centerX = width / 2.0f;
        float centerY = height / 2.0f;

        float rx = lx - centerX;
        float ry = ly - centerY;

        // 1. قرینه
        if (isMirrored) rx = -rx;

        // 2. چرخش (ساده‌سازی شده برای 90 درجه‌ها)
        float fx = rx, fy = ry;
        if (angle == 90) { fx = ry; fy = -rx; }
        else if (angle == 180) { fx = -rx; fy = -ry; }
        else if (angle == 270) { fx = -ry; fy = rx; }

        // 3. بازگشت به مختصات جهانی
        return { x + centerX + fx, y + centerY + fy };
    }

    // موقعیت جهانیِ یک پینِ مشخص از این قطعه
    std::pair<float, float> GetPinWorldPos(const Pin& pin) const {
        return LocalToWorld(pin.offsetX, pin.offsetY);
    }

    // تابعی برای رسم خط با در نظر گرفتن چرخش و قرینه
    void DrawLine(SDL_Renderer* renderer, float x1, float y1, float x2, float y2) {
        auto p1 = LocalToWorld(x1, y1);
        auto p2 = LocalToWorld(x2, y2);
        SDL_RenderLine(renderer, p1.first, p1.second, p2.first, p2.second);
    }
    // ===================================================
    // ویژگی‌های جدید که جا افتاده بودند (حتما باید اینجا باشند)
    // ===================================================
    bool isSelected = false;
    int angle = 0;           // زاویه چرخش
    bool isMirrored = false; // قرینه شدن

    // ---------------------------------------------------------------
    // تابع تشخیص کلیک موس روی قطعه (برای انتخاب تک‌کلیکی - بخش ۴.۲.۱)
    //
    // به‌جای یک جعبه‌ی ثابت و تقریبی، از اندازه‌ی واقعی قطعه (width/height)
    // استفاده می‌کنیم تا Bounding Box دقیقاً منطبق بر ابعاد هر قطعه باشد.
    // (توجه: این تست نسبت به چرخش/قرینه، محورهای x/y جهانی را در نظر
    //  می‌گیرد - یعنی جعبه‌ی محوری‌ست، نه چرخیده؛ برای اکثر قطعات این
    //  پروژه که در زوایای ۹۰ درجه چرخش می‌کنند تقریب مناسبی است.)
    // ---------------------------------------------------------------
    virtual bool Contains(float mouseX, float mouseY) {
        float pad = 6.0f; // کمی حاشیه‌ی اضافه تا کلیک نزدیک لبه هم ثبت شود
        return (mouseX >= x - pad && mouseX <= x + width + pad &&
                mouseY >= y - pad && mouseY <= y + height + pad);
    }
    // ===================================================

    // ---------------------------------------------------------------
    // پنجره‌ی ویژگی‌ها (بخش ۴.۷): هر نوع قطعه فیلدهای اختصاصیِ خودش را
    // معرفی می‌کند (مثلا مقاومت -> Resistance، منبع DC -> Voltage).
    // کلاس پایه فقط «برچسب» را قابل ویرایش می‌داند؛ فرزندان می‌توانند
    // این دو تابع را Override کنند تا فیلدهای بیشتری اضافه کنند.
    // SetProperties با همان ترتیبی که GetProperties برگردانده صدا زده
    // می‌شود (values[0] = مقدار جدید فیلد اول و ...).
    // ---------------------------------------------------------------
    virtual std::vector<PropertyField> GetProperties() {
        return { { "Label", label } };
    }

    virtual void SetProperties(const std::vector<std::string>& values) {
        if (!values.empty()) label = values[0];
    }

    Component(float posX, float posY, ComponentType t)
        : x(posX), y(posY), type(t) {}

    virtual ~Component() = default;

    // متدهای اصلی هر قطعه
    virtual void Draw(SDL_Renderer* renderer) = 0;
    virtual void Update() {} // برای به‌روزرسانی وضعیت منطقی یا کلاک
    virtual bool HandleClick(float mouseX, float mouseY) { return false; } // برای کلیدها و دکمه‌ها

    // بررسی کلیک روی قطعه
    bool ContainsPoint(float px, float py) const {
        return (px >= x && px <= x + width && py >= y && py <= y + height);
    }
};

// -----------------------------------------
// 4. کلاس‌های فرزند (قطعات)
// -----------------------------------------

// --- مقاومت ---
// ==========================================
// 1. مقاومت (شکل زیگ‌زاگ استاندارد)
// ==========================================
class ResistorComponent : public Component {
public:
    float resistanceOhms = 1000.0f; // مقدار مقاومت به اهم (پیش‌فرض 1k)

    ResistorComponent(float x, float y) : Component(x, y, ComponentType::RESISTOR) {
        label = "R"; width = 60; height = 40;
        pins.push_back({"1", 0, 20, false});
        pins.push_back({"2", 60, 20, false});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        // پین‌ها
        DrawLine(renderer, 0, 20, 10, 20);
        DrawLine(renderer, 50, 20, 60, 20);

        DrawLine(renderer, 10, 20, 15, 10);
        DrawLine(renderer, 15, 10, 25, 30);
        DrawLine(renderer, 25, 30, 35, 10);
        DrawLine(renderer, 35, 10, 45, 30);
        DrawLine(renderer, 45, 30, 50, 20);
    }

    std::vector<PropertyField> GetProperties() override {
        return {
            { "Label", label },
            { "Resistance (Ohm)", std::to_string((long long)resistanceOhms) }
        };
    }
    void SetProperties(const std::vector<std::string>& values) override {
        if (values.size() >= 1) label = values[0];
        if (values.size() >= 2) {
            try { resistanceOhms = std::stof(values[1]); } catch (...) {}
        }
    }
};

// ==========================================
// 2. خازن (دو خط موازی)
// ==========================================
class CapacitorComponent : public Component {
public:
    float capacitanceMicroFarad = 100.0f; // مقدار خازن به میکروفاراد

    CapacitorComponent(float x, float y) : Component(x, y, ComponentType::CAPACITOR) {
        label = "C"; width = 60; height = 40;
        pins.push_back({"1", 0, 20, false});
        pins.push_back({"2", 60, 20, false});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        // پین‌های افقی
        DrawLine(renderer, 0, 20, 25, 20);
        DrawLine(renderer, 35, 20, 60, 20);
        // صفحات موازی خازن (عمودی)
        DrawLine(renderer, 25, 5, 25, 35);
        DrawLine(renderer, 35, 5, 35, 35);
    }

    std::vector<PropertyField> GetProperties() override {
        return {
            { "Label", label },
            { "Capacitance (uF)", std::to_string((long long)capacitanceMicroFarad) }
        };
    }
    void SetProperties(const std::vector<std::string>& values) override {
        if (values.size() >= 1) label = values[0];
        if (values.size() >= 2) {
            try { capacitanceMicroFarad = std::stof(values[1]); } catch (...) {}
        }
    }
};

// ==========================================
// 3. زمین (GND - شکل مثلثی)
// ==========================================
class GNDComponent : public Component {
public:
    GNDComponent(float x, float y) : Component(x, y, ComponentType::GND) {
        label = "GND"; width = 40; height = 40;
        pins.push_back({"GND", 20, 0, false});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        // خط عمودی ورودی
        DrawLine(renderer, 20, 0, 20, 15);
        // خطوط افقی زمین
        DrawLine(renderer, 5, 15, 35, 15);
        DrawLine(renderer, 12, 23, 28, 23);
        DrawLine(renderer, 18, 30, 22, 30);
    }
};

// ==========================================
// 4. گیت AND (شکل حرف D)
// ==========================================
class GateANDComponent : public Component {
public:
    GateANDComponent(float x, float y) : Component(x, y, ComponentType::GATE_AND) {
        label = "AND"; width = 60; height = 40;
        pins.push_back({"A", 0, 10, false});
        pins.push_back({"B", 0, 30, false});
        pins.push_back({"Y", 60, 20, true});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        // پین‌ها
        DrawLine(renderer, 0, 10, 15, 10);
        DrawLine(renderer, 0, 30, 15, 30);
        DrawLine(renderer, 45, 20, 60, 20);

        // بدنه گیت AND
        DrawLine(renderer, 15, 5, 15, 35); // خط صاف چپ
        DrawLine(renderer, 15, 5, 30, 5);  // بالا
        DrawLine(renderer, 15, 35, 30, 35);// پایین

        // تقریب منحنی سمت راست گیت AND
        DrawLine(renderer, 30, 5, 40, 12);
        DrawLine(renderer, 40, 12, 45, 20);
        DrawLine(renderer, 45, 20, 40, 28);
        DrawLine(renderer, 40, 28, 30, 35);
    }
};

// ==========================================
// 5. گیت OR (شکل منحنی)
// ==========================================
class GateORComponent : public Component {
public:
    GateORComponent(float x, float y) : Component(x, y, ComponentType::GATE_OR) {
        label = "OR"; width = 60; height = 40;
        pins.push_back({"A", 0, 10, false});
        pins.push_back({"B", 0, 30, false});
        pins.push_back({"Y", 60, 20, true});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);

        // منحنی پشتی گیت OR
        DrawLine(renderer, 10, 5, 20, 20);
        DrawLine(renderer, 20, 20, 10, 35);

        // پین‌ها
        DrawLine(renderer, 0, 10, 13, 10);
        DrawLine(renderer, 0, 30, 13, 30);
        DrawLine(renderer, 50, 20, 60, 20);

        // منحنی بالایی و پایینی به سمت نوک (خروجی)
        DrawLine(renderer, 10, 5, 35, 10);
        DrawLine(renderer, 35, 10, 50, 20);
        DrawLine(renderer, 10, 35, 35, 30);
        DrawLine(renderer, 35, 30, 50, 20);
    }
};

// ==========================================
// 6. منبع ولتاژ DC
// ==========================================
class DCSourceComponent : public Component {
public:
    float voltage = 5.0f; // ولتاژ منبع (پیش‌فرض ۵ ولت)

    DCSourceComponent(float x, float y) : Component(x, y, ComponentType::DC_SOURCE) {
        label = "DC"; width = 40; height = 40;
        pins.push_back({"V+", 40, 20, true});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);

        // رسم کادر منبع با 4 خط (برای پشتیبانی از چرخش)
        DrawLine(renderer, 10, 10, 30, 10);
        DrawLine(renderer, 30, 10, 30, 30);
        DrawLine(renderer, 30, 30, 10, 30);
        DrawLine(renderer, 10, 30, 10, 10);

        // پین خروجی و علامت مثبت داخل منبع
        DrawLine(renderer, 30, 20, 40, 20);
        DrawLine(renderer, 15, 20, 25, 20);
        DrawLine(renderer, 20, 15, 20, 25);
    }

    std::vector<PropertyField> GetProperties() override {
        return {
            { "Label", label },
            { "Voltage (V)", std::to_string((long long)voltage) }
        };
    }
    void SetProperties(const std::vector<std::string>& values) override {
        if (values.size() >= 1) label = values[0];
        if (values.size() >= 2) {
            try {
                voltage = std::stof(values[1]);
                if (!pins.empty()) pins[0].voltage = voltage;
            } catch (...) {}
        }
    }
};

// ==========================================
// 7. باتری (Battery)
// ==========================================
class BatteryComponent : public Component {
public:
    float voltage = 9.0f; // ولتاژ باتری (پیش‌فرض ۹ ولت)

    BatteryComponent(float x, float y) : Component(x, y, ComponentType::BATTERY) {
        label = "BAT"; width = 40; height = 60;
        pins.push_back({"+", 20, 0, true});
        pins.push_back({"-", 20, 60, false});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        DrawLine(renderer, 20, 0, 20, 20);   // پین بالا
        DrawLine(renderer, 5, 20, 35, 20);   // خط بلند (مثبت)
        DrawLine(renderer, 12, 30, 28, 30);  // خط کوتاه (منفی)
        DrawLine(renderer, 5, 40, 35, 40);   // خط بلند
        DrawLine(renderer, 12, 50, 28, 50);  // خط کوتاه
        DrawLine(renderer, 20, 50, 20, 60);  // پین پایین
    }

    std::vector<PropertyField> GetProperties() override {
        return {
            { "Label", label },
            { "Voltage (V)", std::to_string((long long)voltage) }
        };
    }
    void SetProperties(const std::vector<std::string>& values) override {
        if (values.size() >= 1) label = values[0];
        if (values.size() >= 2) {
            try {
                voltage = std::stof(values[1]);
                if (!pins.empty()) pins[0].voltage = voltage;
            } catch (...) {}
        }
    }
};

// ==========================================
// 8. کلاک پالس (Pulse Clock)
// ==========================================
class ClockComponent : public Component {
public:
    float frequencyHz = 1.0f; // فرکانس پالس کلاک (پیش‌فرض ۱ هرتز)

    ClockComponent(float x, float y) : Component(x, y, ComponentType::CLOCK) {
        label = "CLK"; width = 40; height = 40;
        pins.push_back({"OUT", 40, 20, true});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);

        // رسم کادر با 4 خط
        DrawLine(renderer, 10, 10, 30, 10);
        DrawLine(renderer, 30, 10, 30, 30);
        DrawLine(renderer, 30, 30, 10, 30);
        DrawLine(renderer, 10, 30, 10, 10);

        DrawLine(renderer, 30, 20, 40, 20); // پین
        // شکل موج مربعی
        DrawLine(renderer, 14, 25, 14, 15);
        DrawLine(renderer, 14, 15, 22, 15);
        DrawLine(renderer, 22, 15, 22, 25);
        DrawLine(renderer, 22, 25, 28, 25);
    }

    std::vector<PropertyField> GetProperties() override {
        return {
            { "Label", label },
            { "Frequency (Hz)", std::to_string((long long)frequencyHz) }
        };
    }
    void SetProperties(const std::vector<std::string>& values) override {
        if (values.size() >= 1) label = values[0];
        if (values.size() >= 2) {
            try { frequencyHz = std::stof(values[1]); } catch (...) {}
        }
    }
};

// ==========================================
// 9. دکمه فشاری (Push Button)
// ==========================================
class PushButtonComponent : public Component {
public:
    bool isPressed = false;
    PushButtonComponent(float x, float y) : Component(x, y, ComponentType::PUSH_BUTTON) {
        label = "BTN"; width = 50; height = 30;
        pins.push_back({"1", 0, 20, false});
        pins.push_back({"2", 50, 20, true});
    }
    bool HandleClick(float mouseX, float mouseY) override {
        if (ContainsPoint(mouseX, mouseY)) {
            isPressed = !isPressed;
            return true;
        }
        return false;
    }
    std::vector<PropertyField> GetProperties() override {
        return { { "Label", label }, { "Pressed (0/1)", isPressed ? "1" : "0" } };
    }

    void SetProperties(const std::vector<std::string>& values) override {
        if (!values.empty()) label = values[0];
        if (values.size() >= 2) isPressed = (values[1] == "1" || values[1] == "true" || values[1] == "TRUE");
    }

    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        DrawLine(renderer, 0, 20, 15, 20);
        DrawLine(renderer, 35, 20, 50, 20);

        // قسمت دکمه
        float pushOffset = isPressed ? 20 : 10;
        DrawLine(renderer, 15, pushOffset, 35, pushOffset); // تیغه
        DrawLine(renderer, 25, 5, 25, pushOffset); // میله دکمه
    }
};

// ==========================================
// 10. سون سگمنت (7-Segment)
// ==========================================
class SevenSegmentComponent : public Component {
public:
    SevenSegmentComponent(float x, float y) : Component(x, y, ComponentType::SEVEN_SEGMENT) {
        label = "7SEG"; width = 40; height = 60;
        for(int i=0; i<7; i++) {
            pins.push_back({"P"+std::to_string(i), (float)(i*5 + 5), 60, false});
        }
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        // کادر اصلی سون سگمنت (بدنه)
        DrawLine(renderer, 0, 0, 40, 0);
        DrawLine(renderer, 40, 0, 40, 50);
        DrawLine(renderer, 40, 50, 0, 50);
        DrawLine(renderer, 0, 50, 0, 0);

        // رسم پین‌های پایین
        for(int i=0; i<7; i++) {
            DrawLine(renderer, 5 + i*5, 50, 5 + i*5, 60);
        }

        // رسم شکل 8 کم‌رنگ
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        // مربع بالایی
        DrawLine(renderer, 10, 10, 30, 10);
        DrawLine(renderer, 30, 10, 30, 25);
        DrawLine(renderer, 30, 25, 10, 25);
        DrawLine(renderer, 10, 25, 10, 10);
        // مربع پایینی
        DrawLine(renderer, 10, 25, 30, 25);
        DrawLine(renderer, 30, 25, 30, 40);
        DrawLine(renderer, 30, 40, 10, 40);
        DrawLine(renderer, 10, 40, 10, 25);
    }
};

// ==========================================
// 11. گیت NOT
// ==========================================
class GateNOTComponent : public Component {
public:
    GateNOTComponent(float x, float y) : Component(x, y, ComponentType::GATE_NOT) {
        label = "NOT"; width = 60; height = 40;
        pins.push_back({"A", 0, 20, false});
        pins.push_back({"Y", 60, 20, true});
    }
    std::vector<PropertyField> GetProperties() override {
        return { { "Label", label } };
    }

    void SetProperties(const std::vector<std::string>& values) override {
        if (!values.empty()) label = values[0];
    }

    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        DrawLine(renderer, 0, 20, 15, 20); // ورودی
        DrawLine(renderer, 45, 20, 60, 20); // خروجی

        // مثلث
        DrawLine(renderer, 15, 5, 15, 35);
        DrawLine(renderer, 15, 5, 40, 20);
        DrawLine(renderer, 15, 35, 40, 20);

        // دایره (حباب) خروجی با 4 خط
        DrawLine(renderer, 40, 18, 44, 18);
        DrawLine(renderer, 44, 18, 44, 22);
        DrawLine(renderer, 44, 22, 40, 22);
        DrawLine(renderer, 40, 22, 40, 18);
    }
};

// ==========================================
// 12. گیت NAND و NOR و XOR
// ==========================================
class GateNANDComponent : public GateANDComponent {
public:
    GateNANDComponent(float x, float y) : GateANDComponent(x, y) { type = ComponentType::GATE_NAND; label = "NAND"; }
    void Draw(SDL_Renderer* renderer) override {
        GateANDComponent::Draw(renderer); // رسم AND
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        // حباب خروجی
        DrawLine(renderer, 45, 18, 49, 18);
        DrawLine(renderer, 49, 18, 49, 22);
        DrawLine(renderer, 49, 22, 45, 22);
        DrawLine(renderer, 45, 22, 45, 18);
    }
};

class GateNORComponent : public GateORComponent {
public:
    GateNORComponent(float x, float y) : GateORComponent(x, y) { type = ComponentType::GATE_NOR; label = "NOR"; }
    void Draw(SDL_Renderer* renderer) override {
        GateORComponent::Draw(renderer); // رسم OR
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        // حباب خروجی
        DrawLine(renderer, 50, 18, 54, 18);
        DrawLine(renderer, 54, 18, 54, 22);
        DrawLine(renderer, 54, 22, 50, 22);
        DrawLine(renderer, 50, 22, 50, 18);
    }
};

class GateXORComponent : public GateORComponent {
public:
    GateXORComponent(float x, float y) : GateORComponent(x, y) { type = ComponentType::GATE_XOR; label = "XOR"; }
    void Draw(SDL_Renderer* renderer) override {
        GateORComponent::Draw(renderer); // رسم OR
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        // خط منحنی دوم پشت گیت
        DrawLine(renderer, 5, 5, 15, 20);
        DrawLine(renderer, 15, 20, 5, 35);
    }
};
// ==========================================
// 13. فلیپ‌فلاپ (D Flip-Flop)
// ==========================================
class FlipFlopDComponent : public Component {
public:
    FlipFlopDComponent(float x, float y) : Component(x, y, ComponentType::FLIP_FLOP_D) {
        label = "D-FF"; width = 60; height = 60;
        pins.push_back({"D", 0, 15, false});
        pins.push_back({"CLK", 0, 45, false});
        pins.push_back({"Q", 60, 15, true});
        pins.push_back({"Q'", 60, 45, true});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);

        // کادر اصلی فلیپ‌فلاپ
        DrawLine(renderer, 10, 0, 50, 0);
        DrawLine(renderer, 50, 0, 50, 60);
        DrawLine(renderer, 50, 60, 10, 60);
        DrawLine(renderer, 10, 60, 10, 0);

        // پین‌ها
        DrawLine(renderer, 0, 15, 10, 15); // D
        DrawLine(renderer, 0, 45, 10, 45); // CLK
        DrawLine(renderer, 50, 15, 60, 15); // Q
        DrawLine(renderer, 50, 45, 60, 45); // Q'

        // علامت کلاک (مثلث کوچک روی پین CLK)
        DrawLine(renderer, 10, 40, 18, 45);
        DrawLine(renderer, 18, 45, 10, 50);
    }
};
// ==========================================
// 14. کلید قطع و وصل (Switch)
// ==========================================
class SwitchComponent : public Component {
public:
    bool isOpen = true;

    SwitchComponent(float x, float y) : Component(x, y, ComponentType::SWITCH) {
        label = "SW"; width = 50; height = 30;
        pins.push_back({"IN", 0, 15, false});
        pins.push_back({"OUT", 50, 15, true});
    }

    bool HandleClick(float mouseX, float mouseY) override {
        if (ContainsPoint(mouseX, mouseY)) {
            isOpen = !isOpen;
            pins[1].voltage = isOpen ? 0.0f : pins[0].voltage;
            return true;
        }
        return false;
    }

    std::vector<PropertyField> GetProperties() override {
        return { { "Label", label }, { "Open (0/1)", isOpen ? "1" : "0" } };
    }

    void SetProperties(const std::vector<std::string>& values) override {
        if (!values.empty()) label = values[0];
        if (values.size() >= 2) {
            isOpen = (values[1] == "1" || values[1] == "true" || values[1] == "TRUE");
            if (pins.size() > 1) pins[1].voltage = isOpen ? 0.0f : pins[0].voltage;
        }
    }

    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);

        // پین‌های ورودی و خروجی
        DrawLine(renderer, 0, 15, 15, 15);
        DrawLine(renderer, 35, 15, 50, 15);

        // تیغه کلید
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255);
        if (isOpen) {
            DrawLine(renderer, 15, 15, 32, 2); // حالت باز
        } else {
            DrawLine(renderer, 15, 15, 35, 15); // حالت بسته
        }
    }
};

// ==========================================
// 15. ال‌ئی‌دی (LED)
// ==========================================
class LEDComponent : public Component {
public:
    bool isOn = false;
    SDL_Color color = { 255, 0, 0, 255 }; // پیش‌فرض قرمز

    LEDComponent(float x, float y) : Component(x, y, ComponentType::LED) {
        label = "LED"; width = 50; height = 40;
        pins.push_back({"Anode", 0, 20, false});
        pins.push_back({"Cathode", 50, 20, false});
    }

    std::vector<PropertyField> GetProperties() override {
        return { { "Label", label }, { "On (0/1)", isOn ? "1" : "0" }, { "Color RGB", std::to_string(color.r) + "," + std::to_string(color.g) + "," + std::to_string(color.b) } };
    }

    void SetProperties(const std::vector<std::string>& values) override {
        if (!values.empty()) label = values[0];
        if (values.size() >= 2) isOn = (values[1] == "1" || values[1] == "true" || values[1] == "TRUE");
        if (values.size() >= 3) {
            unsigned int r=255,g=0,b=0;
            if (std::sscanf(values[2].c_str(), "%u,%u,%u", &r,&g,&b) == 3) {
                color = SDL_Color{(Uint8)std::min(r,255u),(Uint8)std::min(g,255u),(Uint8)std::min(b,255u),255};
            }
        }
    }

    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);

        // پین‌های دو طرف
        DrawLine(renderer, 0, 20, 15, 20);
        DrawLine(renderer, 35, 20, 50, 20);

        // مثلث نماد دیود
        DrawLine(renderer, 15, 10, 15, 30);
        DrawLine(renderer, 15, 10, 35, 20);
        DrawLine(renderer, 15, 30, 35, 20);

        // خط صاف کاتد
        DrawLine(renderer, 35, 10, 35, 30);

        // فلش‌های نور
        DrawLine(renderer, 25, 8, 30, 2);
        DrawLine(renderer, 30, 8, 35, 2);

        // اگر روشن بود، داخل آن با کشیدن 8 خط موازی رنگی می‌شود (تا با چرخش هماهنگ باشد)
        if (isOn) {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
            for(int i = 0; i < 8; i++) {
                DrawLine(renderer, 18, 16 + i, 26, 16 + i);
            }
        }
    }
};

// ==========================================
// 16. وضعیت منطقی (Logic State 0/1)
// ==========================================
class LogicStateComponent : public Component {
public:
    bool state = false; // false = 0, true = 1

    LogicStateComponent(float x, float y) : Component(x, y, ComponentType::LOGIC_STATE) {
        label = "LOGIC"; width = 30; height = 30;
        pins.push_back({"OUT", 30, 15, true}); // پین خروجی
    }

    bool HandleClick(float mouseX, float mouseY) override {
        if (ContainsPoint(mouseX, mouseY)) {
            state = !state;
            pins[0].voltage = state ? 5.0f : 0.0f;
            return true;
        }
        return false;
    }

    std::vector<PropertyField> GetProperties() override {
        return { { "Label", label }, { "State (0/1)", state ? "1" : "0" } };
    }

    void SetProperties(const std::vector<std::string>& values) override {
        if (!values.empty()) label = values[0];
        if (values.size() >= 2) {
            state = (values[1] == "1" || values[1] == "true" || values[1] == "TRUE");
            if (!pins.empty()) pins[0].voltage = state ? 5.0f : 0.0f;
        }
    }

    void Draw(SDL_Renderer* renderer) override {
        // رسم پس‌زمینه قطعه (با 30 خط موازی برای پشتیبانی کامل از چرخش!)
        SDL_SetRenderDrawColor(renderer, isSelected ? 150 : 220, isSelected ? 200 : 220, isSelected ? 255 : 220, 255);
        for(int i = 0; i <= 30; i++) {
            DrawLine(renderer, 0, i, 30, i);
        }

        // رسم حاشیه
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        DrawLine(renderer, 0, 0, 30, 0);
        DrawLine(renderer, 30, 0, 30, 30);
        DrawLine(renderer, 30, 30, 0, 30);
        DrawLine(renderer, 0, 30, 0, 0);

        // رسم پین خروجی
        DrawLine(renderer, 30, 15, 40, 15);

        // رسم عدد
        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255);
        if (state) {
            // رسم عدد '1'
            DrawLine(renderer, 15, 5, 15, 25);
            DrawLine(renderer, 10, 10, 15, 5);
        } else {
            // رسم عدد '0' (کادر دور توخالی)
            DrawLine(renderer, 10, 5, 20, 5);
            DrawLine(renderer, 20, 5, 20, 25);
            DrawLine(renderer, 20, 25, 10, 25);
            DrawLine(renderer, 10, 25, 10, 5);
        }
    }
};

#endif
