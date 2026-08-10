#ifndef COMPONENT_H
#define COMPONENT_H

#include <SDL3/SDL.h>
#include <string>
#include <vector>

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
// 2. ساختار پین‌ها
// -----------------------------------------
struct Pin {
    std::string name;
    float offsetX, offsetY; // موقعیت پین نسبت به مرکز قطعه
    bool isOutput;
    float voltage = 0.0f;   // مقدار ولتاژ (0 ولت یا 5 ولت منطقی)
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
    bool isSelected = false;

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
    ResistorComponent(float x, float y) : Component(x, y, ComponentType::RESISTOR) {
        label = "R"; width = 60; height = 40;
        pins.push_back({"1", 0, 20, false});
        pins.push_back({"2", 60, 20, false});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        // پین‌ها
        SDL_RenderLine(renderer, x, y + 20, x + 10, y + 20);
        SDL_RenderLine(renderer, x + 50, y + 20, x + 60, y + 20);
        // زیگ‌زاگ مقاومت
        SDL_RenderLine(renderer, x + 10, y + 20, x + 15, y + 10);
        SDL_RenderLine(renderer, x + 15, y + 10, x + 25, y + 30);
        SDL_RenderLine(renderer, x + 25, y + 30, x + 35, y + 10);
        SDL_RenderLine(renderer, x + 35, y + 10, x + 45, y + 30);
        SDL_RenderLine(renderer, x + 45, y + 30, x + 50, y + 20);
    }
};

// ==========================================
// 2. خازن (دو خط موازی)
// ==========================================
class CapacitorComponent : public Component {
public:
    CapacitorComponent(float x, float y) : Component(x, y, ComponentType::CAPACITOR) {
        label = "C"; width = 60; height = 40;
        pins.push_back({"1", 0, 20, false});
        pins.push_back({"2", 60, 20, false});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        // پین‌های افقی
        SDL_RenderLine(renderer, x, y + 20, x + 25, y + 20);
        SDL_RenderLine(renderer, x + 35, y + 20, x + 60, y + 20);
        // صفحات موازی خازن (عمودی)
        SDL_RenderLine(renderer, x + 25, y + 5, x + 25, y + 35);
        SDL_RenderLine(renderer, x + 35, y + 5, x + 35, y + 35);
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
        SDL_RenderLine(renderer, x + 20, y, x + 20, y + 15);
        // خطوط افقی زمین
        SDL_RenderLine(renderer, x + 5, y + 15, x + 35, y + 15);
        SDL_RenderLine(renderer, x + 12, y + 23, x + 28, y + 23);
        SDL_RenderLine(renderer, x + 18, y + 30, x + 22, y + 30);
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
        SDL_RenderLine(renderer, x, y + 10, x + 15, y + 10);
        SDL_RenderLine(renderer, x, y + 30, x + 15, y + 30);
        SDL_RenderLine(renderer, x + 45, y + 20, x + 60, y + 20);

        // بدنه گیت AND
        SDL_RenderLine(renderer, x + 15, y + 5, x + 15, y + 35); // خط صاف چپ
        SDL_RenderLine(renderer, x + 15, y + 5, x + 30, y + 5);  // بالا
        SDL_RenderLine(renderer, x + 15, y + 35, x + 30, y + 35);// پایین

        // تقریب منحنی سمت راست گیت AND
        SDL_RenderLine(renderer, x + 30, y + 5, x + 40, y + 12);
        SDL_RenderLine(renderer, x + 40, y + 12, x + 45, y + 20);
        SDL_RenderLine(renderer, x + 45, y + 20, x + 40, y + 28);
        SDL_RenderLine(renderer, x + 40, y + 28, x + 30, y + 35);
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

        // منحنی پشتی گیت OR (تقریب با 2 خط)
        SDL_RenderLine(renderer, x + 10, y + 5, x + 20, y + 20);
        SDL_RenderLine(renderer, x + 20, y + 20, x + 10, y + 35);

        // پین‌ها
        SDL_RenderLine(renderer, x, y + 10, x + 13, y + 10);
        SDL_RenderLine(renderer, x, y + 30, x + 13, y + 30);
        SDL_RenderLine(renderer, x + 50, y + 20, x + 60, y + 20);

        // منحنی بالایی و پایینی به سمت نوک (خروجی)
        SDL_RenderLine(renderer, x + 10, y + 5, x + 35, y + 10);
        SDL_RenderLine(renderer, x + 35, y + 10, x + 50, y + 20);

        SDL_RenderLine(renderer, x + 10, y + 35, x + 35, y + 30);
        SDL_RenderLine(renderer, x + 35, y + 30, x + 50, y + 20);
    }
};
// ==========================================
// 6. منبع ولتاژ DC
// ==========================================
class DCSourceComponent : public Component {
public:
    DCSourceComponent(float x, float y) : Component(x, y, ComponentType::DC_SOURCE) {
        label = "DC"; width = 40; height = 40;
        pins.push_back({"V+", 40, 20, true});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        SDL_FRect body = { x + 10, y + 10, 20, 20 };
        SDL_RenderRect(renderer, &body);
        SDL_RenderLine(renderer, x + 30, y + 20, x + 40, y + 20); // پین خروجی
        // علامت مثبت داخل منبع
        SDL_RenderLine(renderer, x + 15, y + 20, x + 25, y + 20);
        SDL_RenderLine(renderer, x + 20, y + 15, x + 20, y + 25);
    }
};

// ==========================================
// 7. باتری (Battery)
// ==========================================
class BatteryComponent : public Component {
public:
    BatteryComponent(float x, float y) : Component(x, y, ComponentType::BATTERY) {
        label = "BAT"; width = 40; height = 60;
        pins.push_back({"+", 20, 0, true});
        pins.push_back({"-", 20, 60, false});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        SDL_RenderLine(renderer, x + 20, y, x + 20, y + 20); // پین بالا
        SDL_RenderLine(renderer, x + 5, y + 20, x + 35, y + 20); // خط بلند (مثبت)
        SDL_RenderLine(renderer, x + 12, y + 30, x + 28, y + 30); // خط کوتاه (منفی)
        SDL_RenderLine(renderer, x + 5, y + 40, x + 35, y + 40); // خط بلند
        SDL_RenderLine(renderer, x + 12, y + 50, x + 28, y + 50); // خط کوتاه
        SDL_RenderLine(renderer, x + 20, y + 50, x + 20, y + 60); // پین پایین
    }
};

// ==========================================
// 8. کلاک پالس (Pulse Clock)
// ==========================================
class ClockComponent : public Component {
public:
    ClockComponent(float x, float y) : Component(x, y, ComponentType::CLOCK) {
        label = "CLK"; width = 40; height = 40;
        pins.push_back({"OUT", 40, 20, true});
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        SDL_FRect body = { x + 10, y + 10, 20, 20 };
        SDL_RenderRect(renderer, &body);
        SDL_RenderLine(renderer, x + 30, y + 20, x + 40, y + 20);
        // شکل موج مربعی
        SDL_RenderLine(renderer, x + 14, y + 25, x + 14, y + 15);
        SDL_RenderLine(renderer, x + 14, y + 15, x + 22, y + 15);
        SDL_RenderLine(renderer, x + 22, y + 15, x + 22, y + 25);
        SDL_RenderLine(renderer, x + 22, y + 25, x + 28, y + 25);
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
            isPressed = !isPressed; // در شبیه‌ساز واقعی این فقط تا زمانی که موس پایین است کار می‌کند
            return true;
        }
        return false;
    }
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        SDL_RenderLine(renderer, x, y + 20, x + 15, y + 20);
        SDL_RenderLine(renderer, x + 35, y + 20, x + 50, y + 20);

        // قسمت دکمه
        float pushOffset = isPressed ? 20 : 10;
        SDL_RenderLine(renderer, x + 15, y + pushOffset, x + 35, y + pushOffset); // تیغه
        SDL_RenderLine(renderer, x + 25, y + 5, x + 25, y + pushOffset); // میله دکمه
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
        SDL_FRect body = { x, y, 40, 50 };
        SDL_RenderRect(renderer, &body); // بدنه سون سگمنت
        // رسم پین‌های پایین
        for(int i=0; i<7; i++) {
            SDL_RenderLine(renderer, x + 5 + i*5, y + 50, x + 5 + i*5, y + 60);
        }
        // رسم شکل 8 کم‌رنگ
        SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
        SDL_FRect topLoop = { x + 10, y + 10, 20, 15 };
        SDL_FRect botLoop = { x + 10, y + 25, 20, 15 };
        SDL_RenderRect(renderer, &topLoop);
        SDL_RenderRect(renderer, &botLoop);
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
    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        SDL_RenderLine(renderer, x, y + 20, x + 15, y + 20); // ورودی
        SDL_RenderLine(renderer, x + 45, y + 20, x + 60, y + 20); // خروجی
        // مثلث
        SDL_RenderLine(renderer, x + 15, y + 5, x + 15, y + 35);
        SDL_RenderLine(renderer, x + 15, y + 5, x + 40, y + 20);
        SDL_RenderLine(renderer, x + 15, y + 35, x + 40, y + 20);
        // دایره (حباب) خروجی - تقریب با مربع کوچک
        SDL_FRect bubble = { x + 40, y + 18, 4, 4 };
        SDL_RenderRect(renderer, &bubble);
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
        SDL_FRect bubble = { x + 45, y + 18, 4, 4 }; // اضافه کردن حباب خروجی
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        SDL_RenderRect(renderer, &bubble);
    }
};

class GateNORComponent : public GateORComponent {
public:
    GateNORComponent(float x, float y) : GateORComponent(x, y) { type = ComponentType::GATE_NOR; label = "NOR"; }
    void Draw(SDL_Renderer* renderer) override {
        GateORComponent::Draw(renderer); // رسم OR
        SDL_FRect bubble = { x + 50, y + 18, 4, 4 };
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        SDL_RenderRect(renderer, &bubble);
    }
};

class GateXORComponent : public GateORComponent {
public:
    GateXORComponent(float x, float y) : GateORComponent(x, y) { type = ComponentType::GATE_XOR; label = "XOR"; }
    void Draw(SDL_Renderer* renderer) override {
        GateORComponent::Draw(renderer); // رسم OR
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        // اضافه کردن خط منحنی دوم پشت گیت
        SDL_RenderLine(renderer, x + 5, y + 5, x + 15, y + 20);
        SDL_RenderLine(renderer, x + 15, y + 20, x + 5, y + 35);
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
        SDL_FRect body = { x + 10, y, 40, 60 };
        SDL_RenderRect(renderer, &body);

        // پین‌ها
        SDL_RenderLine(renderer, x, y + 15, x + 10, y + 15); // D
        SDL_RenderLine(renderer, x, y + 45, x + 10, y + 45); // CLK
        SDL_RenderLine(renderer, x + 50, y + 15, x + 60, y + 15); // Q
        SDL_RenderLine(renderer, x + 50, y + 45, x + 60, y + 45); // Q'

        // علامت کلاک (مثلث کوچک روی پین CLK)
        SDL_RenderLine(renderer, x + 10, y + 40, x + 18, y + 45);
        SDL_RenderLine(renderer, x + 18, y + 45, x + 10, y + 50);
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
        // با کلیک روی کلید، وضعیت آن تغییر می‌کند
        if (ContainsPoint(mouseX, mouseY)) {
            isOpen = !isOpen;
            pins[1].voltage = isOpen ? 0.0f : pins[0].voltage;
            return true;
        }
        return false;
    }

    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);

        // پین‌های ورودی و خروجی
        SDL_RenderLine(renderer, x, y + 15, x + 15, y + 15);
        SDL_RenderLine(renderer, x + 35, y + 15, x + 50, y + 15);

        // تیغه کلید
        SDL_SetRenderDrawColor(renderer, 200, 0, 0, 255); // رنگ قرمز برای تیغه متحرک
        if (isOpen) {
            SDL_RenderLine(renderer, x + 15, y + 15, x + 32, y + 2); // حالت باز
        } else {
            SDL_RenderLine(renderer, x + 15, y + 15, x + 35, y + 15); // حالت بسته
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

    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);

        // پین‌های دو طرف
        SDL_RenderLine(renderer, x, y + 20, x + 15, y + 20);
        SDL_RenderLine(renderer, x + 35, y + 20, x + 50, y + 20);

        // مثلث نماد دیود
        SDL_RenderLine(renderer, x + 15, y + 10, x + 15, y + 30);
        SDL_RenderLine(renderer, x + 15, y + 10, x + 35, y + 20);
        SDL_RenderLine(renderer, x + 15, y + 30, x + 35, y + 20);

        // خط صاف کاتد
        SDL_RenderLine(renderer, x + 35, y + 10, x + 35, y + 30);

        // فلش‌های نور (نماد LED)
        SDL_RenderLine(renderer, x + 25, y + 8, x + 30, y + 2);
        SDL_RenderLine(renderer, x + 30, y + 8, x + 35, y + 2);

        // اگر روشن بود، داخل آن رنگی شود
        if (isOn) {
            SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, 255);
            SDL_FRect light = { x + 18, y + 16, 8, 8 };
            SDL_RenderFillRect(renderer, &light);
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
            state = !state; // تغییر وضعیت بین صفر و یک با کلیک
            pins[0].voltage = state ? 5.0f : 0.0f; // تغییر ولتاژ پین
            return true;
        }
        return false;
    }

    void Draw(SDL_Renderer* renderer) override {
        // رسم پس‌زمینه قطعه
        SDL_SetRenderDrawColor(renderer, isSelected ? 150 : 220, isSelected ? 200 : 220, isSelected ? 255 : 220, 255);
        SDL_FRect body = { x, y, 30, 30 };
        SDL_RenderFillRect(renderer, &body);

        // رسم حاشیه
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderRect(renderer, &body);

        // رسم پین خروجی
        SDL_RenderLine(renderer, x + 30, y + 15, x + 40, y + 15);

        // رسم عدد 0 یا 1 داخل کادر با خطوط (بدون نیاز به فونت برای سادگی رندر)
        SDL_SetRenderDrawColor(renderer, 0, 0, 200, 255); // رنگ آبی برای عدد
        if (state) {
            // رسم عدد '1'
            SDL_RenderLine(renderer, x + 15, y + 5, x + 15, y + 25);
            SDL_RenderLine(renderer, x + 10, y + 10, x + 15, y + 5);
        } else {
            // رسم عدد '0'
            SDL_FRect zeroRect = { x + 10, y + 5, 10, 20 };
            SDL_RenderRect(renderer, &zeroRect);
        }
    }
};
#endif
