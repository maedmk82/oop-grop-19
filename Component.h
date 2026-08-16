#ifndef COMPONENT_H
#define COMPONENT_H

#include <SDL3/SDL.h>
#include <string>
#include <vector>
#include <cmath>
#include <utility>
#include <sstream>
#include <algorithm>
#include <fstream>   // برای خواندن فایل .hex از دیسک (بخش ۳.۷)
#include <cstdint>   // uint8_t / uint16_t برای بایت‌های حافظه‌ی فلش
#include <cctype>    // std::isxdigit برای اعتبارسنجی خطوط hex
#include "MCUCore.h" // بخش ۷.۴/۷.۵ - PC، ثبات‌ها، RAM داخلی، Port، رمزگشای دستورات

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
    GATE_AND, GATE_OR,GATE_NOR, GATE_NOT, GATE_XOR, GATE_NAND, FLIP_FLOP_D,
    ADC, DAC,
    // میکروکنترلر (بخش ۳.۷ - رابط بارگذاری فریمور)
    MICROCONTROLLER,
    // قطعات جانبی و حافظه‌ی خارجی (بخش ۷.۷ تا ۷.۹)
    EXTERNAL_MEMORY, LCD_1602, KEYPAD_4X4
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

// ================================================================
// بخش ۳.۷ - رابط بارگذاری فریمور (Frameware) میکروکنترلر
// ================================================================
// کلاس اختصاصیِ مدیریت فایل، طبق سه وظیفه‌ی خواسته‌شده:
//   ۱. باز کردن و خواندن فایل با فرمت استاندارد hex (Intel HEX)
//   ۲. اعتبارسنجی اولیه‌ی فایل (بررسی ساختار صحیح خطوط hex + چک‌سام)
//   ۳. استخراج کدهای باینری (Opcode ها) و داده‌ها از داخل فایل متنی و
//      بارگذاری مستقیم آن‌ها در آرایه‌ای که نقش حافظه‌ی Flash میکرو را
//      ایفا می‌کند.
// این کلاس عمداً از Component/MicrocontrollerComponent جدا نگه داشته
// شده تا مسئولیت «خواندن و تفسیر فایل» از مسئولیت «نگهداری وضعیت قطعه»
// تفکیک باشد (طراحی زیرسیستم مستقل طبق متن خواسته‌شده).
// ================================================================
class IntelHexLoader {
public:
    // نتیجه‌ی عملیات بارگذاری - برای گزارش موفقیت/خطا (همراه شماره خط) به کاربر
    struct Result {
        bool success = false;
        std::string message;
        int bytesLoaded = 0;
        int highestAddress = -1;
    };

    // باز کردن و خواندن فایل .hex از روی دیسک و نوشتن مستقیمِ Opcode/داده‌ها
    // در flashOut. flashOut باید از قبل با اندازه‌ی حافظه‌ی فلش ساخته شده باشد.
    static Result LoadFile(const std::string& filepath, std::vector<uint8_t>& flashOut) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            Result r;
            r.success = false;
            r.message = "خطا: فایل باز نشد -> " + filepath;
            return r;
        }
        std::stringstream buffer;
        buffer << file.rdbuf();
        return LoadFromString(buffer.str(), flashOut);
    }

    // همان منطق بارگذاری، مستقیم روی یک رشته (تفکیک‌شده تا بدون نیاز به
    // فایل واقعی روی دیسک هم قابل تست باشد).
    static Result LoadFromString(const std::string& content, std::vector<uint8_t>& flashOut) {
        Result result;
        std::istringstream stream(content);
        std::string line;
        int lineNumber = 0;
        unsigned int extendedAddress = 0; // برای رکوردهای نوع ۰۲/۰۴ (آدرس‌دهی بیش از ۶۴کیلوبایت)
        bool eofFound = false;
        int bytesLoaded = 0;
        int highestAddress = -1;

        while (std::getline(stream, line)) {
            lineNumber++;
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n' ||
                                      line.back() == ' ' || line.back() == '\t')) {
                line.pop_back();
            }
            if (line.empty()) continue; // خطوط خالی مجاز است

            // ---- اعتبارسنجی اولیه‌ی ساختار خط ----
            if (line[0] != ':') {
                result.message = "خطای ساختار در خط " + std::to_string(lineNumber) +
                                  ": هر خط باید با ':' شروع شود";
                return result;
            }
            const std::string body = line.substr(1);
            if (body.size() < 10 || (body.size() % 2) != 0 || !IsAllHex(body)) {
                result.message = "خطای فرمت در خط " + std::to_string(lineNumber) +
                                  ": کاراکتر غیرمجاز یا طول نامعتبر";
                return result;
            }

            std::vector<uint8_t> bytes;
            bytes.reserve(body.size() / 2);
            for (size_t i = 0; i + 1 < body.size(); i += 2) {
                bytes.push_back(static_cast<uint8_t>(std::stoi(body.substr(i, 2), nullptr, 16)));
            }

            const uint8_t byteCount = bytes[0];
            const uint16_t address = static_cast<uint16_t>((bytes[1] << 8) | bytes[2]);
            const uint8_t recordType = bytes[3];

            if (bytes.size() != static_cast<size_t>(byteCount) + 5) {
                result.message = "خطای طول داده در خط " + std::to_string(lineNumber) +
                                  ": تعداد بایت اعلام‌شده با محتوای خط همخوانی ندارد";
                return result;
            }

            // ---- اعتبارسنجی چک‌سام: جمع همه‌ی بایت‌ها باید mod 256 == 0 باشد ----
            uint8_t sum = 0;
            for (uint8_t b : bytes) sum = static_cast<uint8_t>(sum + b);
            if (sum != 0) {
                result.message = "خطای چک‌سام در خط " + std::to_string(lineNumber) +
                                  ": فایل خراب یا دستکاری‌شده است";
                return result;
            }

            switch (recordType) {
                case 0x00: { // Data record: همان Opcode/داده‌ای که باید در فلش نوشته شود
                    for (int i = 0; i < byteCount; ++i) {
                        const unsigned int absoluteAddr = extendedAddress + address + i;
                        if (absoluteAddr >= flashOut.size()) {
                            result.message = "خطای سرریز حافظه در خط " + std::to_string(lineNumber) +
                                              ": آدرس بزرگ‌تر از ظرفیت فلش میکرو است";
                            return result;
                        }
                        flashOut[absoluteAddr] = bytes[4 + i];
                        bytesLoaded++;
                        if (static_cast<int>(absoluteAddr) > highestAddress) highestAddress = static_cast<int>(absoluteAddr);
                    }
                    break;
                }
                case 0x01: // End Of File
                    eofFound = true;
                    break;
                case 0x02: // Extended Segment Address
                    extendedAddress = (static_cast<unsigned int>((bytes[4] << 8) | bytes[5])) << 4;
                    break;
                case 0x04: // Extended Linear Address
                    extendedAddress = (static_cast<unsigned int>((bytes[4] << 8) | bytes[5])) << 16;
                    break;
                case 0x03: // Start Segment Address - در شبیه‌سازی استفاده نمی‌شود
                case 0x05: // Start Linear Address - در شبیه‌سازی استفاده نمی‌شود
                    break;
                default:
                    result.message = "نوع رکورد ناشناخته در خط " + std::to_string(lineNumber);
                    return result;
            }

            if (eofFound) break;
        }

        if (!eofFound) {
            result.message = "فایل ناقص است: رکورد پایانی (End Of File) پیدا نشد";
            return result;
        }

        result.success = true;
        result.bytesLoaded = bytesLoaded;
        result.highestAddress = highestAddress;
        result.message = "بارگذاری موفق: " + std::to_string(bytesLoaded) + " بایت نوشته شد";
        return result;
    }

private:
    static bool IsAllHex(const std::string& s) {
        for (char c : s) if (!std::isxdigit(static_cast<unsigned char>(c))) return false;
        return true;
    }
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
    std::vector<PropertyField> GetProperties() override { return { { "Label", label } }; }

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


// ==========================================
// 14. مبدل آنالوگ به دیجیتال (ADC)
// ==========================================
class ADCComponent : public Component {
public:
    float vRefMinus = 0.0f;
    float vRefPlus = 5.0f;
    float analogInputVoltage = 0.0f;
    int resolutionBits = 8;
    float conversionDelayMs = 0.0f;

private:
    int currentCode = 0;
    int pendingCode = 0;
    Uint64 conversionStartMs = 0;
    bool conversionPending = false;

    void RebuildPins() {
        pins.clear();
        pins.push_back({"VIN", 0.0f, height / 2.0f, false});
        const float top = 10.0f;
        const float spacing = (resolutionBits <= 1) ? 0.0f : (height - 20.0f) / float(resolutionBits - 1);
        for (int i = 0; i < resolutionBits; ++i) {
            const float py = (resolutionBits == 1) ? height / 2.0f : top + i * spacing;
            pins.push_back({"D" + std::to_string(i), width, py, true});
        }
        ApplyCodeToPins(currentCode);
    }

    int MaxCode() const {
        return (1 << resolutionBits) - 1;
    }

    int CalculateCode(float vin) const {
        if (resolutionBits < 1 || vRefPlus <= vRefMinus) return 0;
        const int maxCode = MaxCode();
        if (vin <= vRefMinus) return 0;
        if (vin >= vRefPlus) return maxCode;
        const float normalized = (vin - vRefMinus) / (vRefPlus - vRefMinus);
        const int code = static_cast<int>(std::llround(normalized * float(maxCode)));
        return std::max(0, std::min(maxCode, code));
    }

    void ApplyCodeToPins(int code) {
        for (int i = 0; i < resolutionBits && i + 1 < static_cast<int>(pins.size()); ++i) {
            pins[i + 1].voltage = ((code >> i) & 1) ? 5.0f : 0.0f;
        }
    }

public:
    ADCComponent(float x, float y) : Component(x, y, ComponentType::ADC) {
        label = "ADC";
        width = 90.0f;
        height = 110.0f;
        RebuildPins();
    }

    void Update() override {
        if (!pins.empty()) analogInputVoltage = pins[0].voltage;
        const int desired = CalculateCode(analogInputVoltage);
        const Uint64 now = SDL_GetTicks();
        if (desired == currentCode) {
            conversionPending = false;
            pendingCode = desired;
            return;
        }
        if (conversionDelayMs <= 0.0f) {
            currentCode = desired;
            ApplyCodeToPins(currentCode);
            conversionPending = false;
            return;
        }
        if (!conversionPending || pendingCode != desired) {
            pendingCode = desired;
            conversionStartMs = now;
            conversionPending = true;
            return;
        }
        if (now - conversionStartMs >= static_cast<Uint64>(conversionDelayMs)) {
            currentCode = pendingCode;
            ApplyCodeToPins(currentCode);
            conversionPending = false;
        }
    }

    std::vector<PropertyField> GetProperties() override {
        return {
            {"Label", label},
            {"Vref- (V)", std::to_string(vRefMinus)},
            {"Vref+ (V)", std::to_string(vRefPlus)},
            {"Resolution (bits)", std::to_string(resolutionBits)},
            {"Analog Input (V)", std::to_string(analogInputVoltage)},
            {"Conversion Delay (ms)", std::to_string(conversionDelayMs)},
            {"Output Code", std::to_string(currentCode)}
        };
    }

    void SetProperties(const std::vector<std::string>& values) override {
        if (values.size() >= 1) label = values[0];
        if (values.size() >= 2) { try { vRefMinus = std::stof(values[1]); } catch (...) {} }
        if (values.size() >= 3) { try { vRefPlus = std::stof(values[2]); } catch (...) {} }
        if (values.size() >= 4) {
            try {
                const int bits = std::max(1, std::min(16, std::stoi(values[3])));
                resolutionBits = bits;
                height = std::max(70.0f, 20.0f + resolutionBits * 11.0f);
                RebuildPins();
            } catch (...) {}
        }
        if (values.size() >= 5) { try { analogInputVoltage = std::stof(values[4]); } catch (...) {} }
        if (values.size() >= 6) { try { conversionDelayMs = std::max(0.0f, std::stof(values[5])); } catch (...) {} }
        if (!pins.empty()) pins[0].voltage = analogInputVoltage;
        Update();
    }

    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 20, isSelected ? 150 : 20, isSelected ? 255 : 20, 255);
        DrawLine(renderer, 15, 8, width - 15, 8);
        DrawLine(renderer, width - 15, 8, width - 15, height - 8);
        DrawLine(renderer, width - 15, height - 8, 15, height - 8);
        DrawLine(renderer, 15, height - 8, 15, 8);
        DrawLine(renderer, 0, height / 2.0f, 15, height / 2.0f);
        for (int i = 0; i < resolutionBits; ++i) {
            const float py = pins[i + 1].offsetY;
            DrawLine(renderer, width - 15, py, width, py);
        }
        DrawLine(renderer, 28, height * 0.36f, 42, height * 0.50f);
        DrawLine(renderer, 42, height * 0.50f, 28, height * 0.64f);
        DrawLine(renderer, 42, height * 0.50f, 60, height * 0.50f);
    }
};

// ==========================================
// 15. مبدل دیجیتال به آنالوگ (DAC)
// ==========================================
class DACComponent : public Component {
public:
    float vRefMinus = 0.0f;
    float vRefPlus = 5.0f;
    int resolutionBits = 8;
    float conversionDelayMs = 0.0f;
    float analogOutputVoltage = 0.0f;

private:
    Uint64 conversionStartMs = 0;
    bool conversionPending = false;
    float pendingOutputVoltage = 0.0f;

    void RebuildPins() {
        pins.clear();
        const float top = 10.0f;
        const float spacing = (resolutionBits <= 1) ? 0.0f : (height - 20.0f) / float(resolutionBits - 1);
        for (int i = 0; i < resolutionBits; ++i) {
            const float py = (resolutionBits == 1) ? height / 2.0f : top + i * spacing;
            pins.push_back({"D" + std::to_string(i), 0.0f, py, false});
        }
        pins.push_back({"VOUT", width, height / 2.0f, true});
        pins.back().voltage = analogOutputVoltage;
    }

    unsigned int ReadInputCode() const {
        unsigned int code = 0;
        for (int i = 0; i < resolutionBits && i < static_cast<int>(pins.size()) - 1; ++i) {
            if (pins[i].voltage >= 2.5f) code |= (1u << i);
        }
        return code;
    }

    unsigned int MaxCode() const { return (1u << resolutionBits) - 1u; }

    float CalculateOutputVoltage(unsigned int code) const {
        if (resolutionBits < 1 || vRefPlus <= vRefMinus) return vRefMinus;
        const unsigned int maxCode = MaxCode();
        return vRefMinus + (float(code) / float(maxCode)) * (vRefPlus - vRefMinus);
    }

public:
    DACComponent(float x, float y) : Component(x, y, ComponentType::DAC) {
        label = "DAC";
        width = 100.0f;
        height = 110.0f;
        RebuildPins();
    }

    void Update() override {
        const float desired = CalculateOutputVoltage(ReadInputCode());
        const Uint64 now = SDL_GetTicks();
        if (std::fabs(desired - analogOutputVoltage) < 0.0001f) {
            conversionPending = false;
            pendingOutputVoltage = desired;
            return;
        }
        if (conversionDelayMs <= 0.0f) {
            analogOutputVoltage = desired;
            if (!pins.empty()) pins.back().voltage = analogOutputVoltage;
            conversionPending = false;
            return;
        }
        if (!conversionPending || std::fabs(pendingOutputVoltage - desired) > 0.0001f) {
            pendingOutputVoltage = desired;
            conversionStartMs = now;
            conversionPending = true;
            return;
        }
        if (now - conversionStartMs >= static_cast<Uint64>(conversionDelayMs)) {
            analogOutputVoltage = pendingOutputVoltage;
            if (!pins.empty()) pins.back().voltage = analogOutputVoltage;
            conversionPending = false;
        }
    }

    std::vector<PropertyField> GetProperties() override {
        return {
            {"Label", label},
            {"Vref- (V)", std::to_string(vRefMinus)},
            {"Vref+ (V)", std::to_string(vRefPlus)},
            {"Resolution (bits)", std::to_string(resolutionBits)},
            {"Digital Input Code", std::to_string(ReadInputCode())},
            {"Conversion Delay (ms)", std::to_string(conversionDelayMs)},
            {"Analog Output (V)", std::to_string(analogOutputVoltage)}
        };
    }

    void SetProperties(const std::vector<std::string>& values) override {
        if (values.size() >= 1) label = values[0];
        if (values.size() >= 2) { try { vRefMinus = std::stof(values[1]); } catch (...) {} }
        if (values.size() >= 3) { try { vRefPlus = std::stof(values[2]); } catch (...) {} }
        if (values.size() >= 4) {
            try {
                resolutionBits = std::max(1, std::min(16, std::stoi(values[3])));
                height = std::max(70.0f, 20.0f + resolutionBits * 11.0f);
                RebuildPins();
            } catch (...) {}
        }
        if (values.size() >= 5) {
            try {
                unsigned int code = std::min(static_cast<unsigned int>(std::stoul(values[4])), MaxCode());
                for (int i = 0; i < resolutionBits; ++i) pins[i].voltage = ((code >> i) & 1u) ? 5.0f : 0.0f;
            } catch (...) {}
        }
        if (values.size() >= 6) { try { conversionDelayMs = std::max(0.0f, std::stof(values[5])); } catch (...) {} }
        Update();
    }

    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 20, isSelected ? 150 : 20, isSelected ? 255 : 20, 255);
        DrawLine(renderer, 15, 8, width - 15, 8);
        DrawLine(renderer, width - 15, 8, width - 15, height - 8);
        DrawLine(renderer, width - 15, height - 8, 15, height - 8);
        DrawLine(renderer, 15, height - 8, 15, 8);
        for (int i = 0; i < resolutionBits; ++i) {
            const float py = pins[i].offsetY;
            DrawLine(renderer, 0, py, 15, py);
        }
        DrawLine(renderer, width - 15, height / 2.0f, width, height / 2.0f);
        DrawLine(renderer, 35, height * 0.35f, 52, height * 0.50f);
        DrawLine(renderer, 52, height * 0.50f, 35, height * 0.65f);
        DrawLine(renderer, 52, height * 0.50f, 72, height * 0.50f);
    }
};

// ==========================================
// 16. میکروکنترلر (بخش ۳.۷ - رابط بارگذاری فریمور در حافظه‌ی Flash)
// ==========================================
class MicrocontrollerComponent : public Component {
public:
    // ظرفیت پیش‌فرض حافظه‌ی فلش (مشابه یک AVR ساده مثل ATmega8 => ۸ کیلوبایت).
    int flashSizeBytes = 8192;
    std::vector<uint8_t> flashMemory; // آرایه‌ای که نقش حافظه‌ی Flash واقعی میکرو را ایفا می‌کند

    std::string firmwarePath;
    bool firmwareLoaded = false;
    std::string lastLoadMessage = "هنوز فریمور بارگذاری نشده";
    int lastBytesLoaded = 0;

    // ================================================================
    // بخش ۷.۴ - قلب تپنده‌ی شبیه‌ساز: PC، ثبات‌ها و RAM داخلی
    // بخش ۷.۶ - دو پورت ۸بیتی ورودی/خروجی (Port A روی پایه‌های سمت
    // راست چیپ، Port B روی پایه‌های سمت چپ)
    // ================================================================
    ProgramCounter pc;
    RegisterFile regs;
    InternalRAM ram;
    Port portA;
    Port portB;

    // شمارنده‌ی سیکل اجرا: هر چند میلی‌ثانیه یک دستور اجرا شود
    Uint64 lastCycleTicks = 0;
    int cycleIntervalMs = 200;

    // ایندکسِ ثابتِ پایه‌ها روی pins (برای خوانایی کد Update/Draw)
    static constexpr int PIN_VCC = 0;
    static constexpr int PIN_GND = 1;
    static constexpr int PIN_RESET = 2;
    static constexpr int PORTB_START = 3;  // ۸ پایه: ایندکس ۳..۱۰ (سمت چپ)
    static constexpr int PORTA_START = 11; // ۸ پایه: ایندکس ۱۱..۱۸ (سمت راست)

    MicrocontrollerComponent(float x, float y) : Component(x, y, ComponentType::MICROCONTROLLER) {
        label = "MCU";
        width = 100.0f; height = 200.0f;
        flashMemory.assign(static_cast<size_t>(flashSizeBytes), 0xFF); // حافظه‌ی خالی = 0xFF
        RebuildPins();
    }

    void RebuildPins() {
        pins.clear();
        const float top = 12.0f;
        // ---- سمت چپ: VCC، GND، RESET و پورت B (۸ پایه) => جمعاً ۱۱ پایه ----
        const int leftCount = 11;
        const float leftSpacing = (height - 2.0f * top) / float(leftCount - 1);
        pins.push_back({"VCC",   0.0f, top + 0 * leftSpacing, false});
        pins.push_back({"GND",   0.0f, top + 1 * leftSpacing, false});
        pins.push_back({"RESET", 0.0f, top + 2 * leftSpacing, false});
        for (int i = 0; i < 8; ++i) {
            pins.push_back({"PB" + std::to_string(i), 0.0f, top + (3 + i) * leftSpacing, false});
        }
        // ---- سمت راست: پورت A (۸ پایه) ----
        const int rightCount = 8;
        const float rightSpacing = (height - 2.0f * top) / float(rightCount - 1);
        for (int i = 0; i < 8; ++i) {
            pins.push_back({"PA" + std::to_string(i), width, top + i * rightSpacing, false});
        }
    }

    // ---------------------------------------------------------------
    // بخش ۷.۶ - همگام‌سازی پورت‌های منطقی با پایه‌های فیزیکیِ قطعه
    // ---------------------------------------------------------------
    static bool IsHigh(float voltage) { return voltage >= 2.5f; }

    // خواندن ولتاژ فعلیِ پایه‌ها (که توسط WireSystem::PropagateVoltages
    // از قطعات جانبیِ متصل روی سیم رسیده) در بیت‌های ورودیِ هر پورت.
    void ReadInputsFromPins() {
        for (int b = 0; b < 8; ++b) {
            if (!portA.IsOutput(b)) portA.SetInputBit(b, IsHigh(pins[PORTA_START + b].voltage));
            if (!portB.IsOutput(b)) portB.SetInputBit(b, IsHigh(pins[PORTB_START + b].voltage));
        }
    }

    // نوشتن مقدارِ بیت‌های خروجیِ هر پورت روی پایه‌ی فیزیکیِ متناظر، تا
    // در فریمِ بعدی توسط WireSystem به قطعات جانبیِ متصل منتقل شود.
    void WriteOutputsToPins() {
        for (int b = 0; b < 8; ++b) {
            Pin& pa = pins[PORTA_START + b];
            pa.isOutput = portA.IsOutput(b);
            if (pa.isOutput) pa.voltage = portA.ReadOutputBit(b) ? 5.0f : 0.0f;

            Pin& pb = pins[PORTB_START + b];
            pb.isOutput = portB.IsOutput(b);
            if (pb.isOutput) pb.voltage = portB.ReadOutputBit(b) ? 5.0f : 0.0f;
        }
    }

    // ---------------------------------------------------------------
    // بخش ۷.۵ - اجرای یک دستورِ رمزگشایی‌شده (فراخوانیِ InstructionDecoder
    // برای Fetch، و اجرای عملیات روی وضعیت همین قطعه)
    // ---------------------------------------------------------------
    uint8_t ResolveOperand(uint8_t mode, uint8_t value) const {
        switch (mode) {
            case 1: return (value == 8) ? regs.GetACC() : regs.GetR(value); // ثبات
            case 2: return ram.Read(value);                                  // RAM
            default: return value;                                          // Immediate
        }
    }
    void WriteDest(uint8_t dest, uint8_t value) {
        if (dest == 8) regs.SetACC(value); else regs.SetR(dest, value);
    }
    uint8_t ReadDest(uint8_t dest) const {
        return (dest == 8) ? regs.GetACC() : regs.GetR(dest);
    }

    void ExecuteOneInstruction() {
        const Instruction ins = InstructionDecoder::Fetch(flashMemory, pc.Get());
        switch (ins.op) {
            case OpCode::MOV: {
                const uint8_t val = ResolveOperand(ins.operand2, ins.operand3);
                WriteDest(ins.operand1, val);
                break;
            }
            case OpCode::ADD: {
                const uint8_t val = ResolveOperand(ins.operand2, ins.operand3);
                const uint8_t cur = ReadDest(ins.operand1);
                WriteDest(ins.operand1, static_cast<uint8_t>(cur + val));
                break;
            }
            case OpCode::JMP: {
                const uint16_t addr = static_cast<uint16_t>((ins.operand1 << 8) | ins.operand2);
                pc.Set(addr);
                return; // آدرس صراحتاً تنظیم شد؛ نباید با Advance بازنویسی شود
            }
            case OpCode::SETB: {
                Port& port = (ins.operand1 == 0) ? portA : portB;
                port.SetBitDirection(ins.operand2, true);
                port.WriteBit(ins.operand2, true);
                break;
            }
            case OpCode::CLR: {
                Port& port = (ins.operand1 == 0) ? portA : portB;
                port.SetBitDirection(ins.operand2, true);
                port.WriteBit(ins.operand2, false);
                break;
            }
            case OpCode::NOP:
            default:
                break;
        }
        pc.Advance(Instruction::SIZE_BYTES);
    }

    // یک سیکل شبیه‌سازی: خواندنِ ورودی‌ها -> (هر cycleIntervalMs) اجرای
    // یک دستور -> نوشتنِ خروجی‌ها روی پایه‌های فیزیکی.
    void Update() override {
        if (pins.size() < static_cast<size_t>(PORTA_START + 8)) return;

        // پایه‌ی RESET با سطح بالا فعال است: وضعیت داخلی صفر می‌شود
        if (IsHigh(pins[PIN_RESET].voltage)) {
            pc.Reset();
            regs.Reset();
            ram.Reset();
            portA = Port();
            portB = Port();
            WriteOutputsToPins();
            return;
        }

        ReadInputsFromPins();

        if (firmwareLoaded) {
            const Uint64 now = SDL_GetTicks();
            if (now - lastCycleTicks >= static_cast<Uint64>(cycleIntervalMs)) {
                lastCycleTicks = now;
                ExecuteOneInstruction();
            }
        }

        WriteOutputsToPins();
    }

    // ---------------------------------------------------------------
    // وظیفه‌ی اصلیِ بخش ۳.۷: باز کردن/خواندن فایل hex، اعتبارسنجی آن و
    // استخراج + بارگذاری مستقیمِ Opcode/داده‌ها در آرایه‌ی flashMemory.
    // منطق واقعیِ پارس کردن داخل IntelHexLoader است؛ این تابع فقط نتیجه
    // را می‌گیرد و وضعیت قطعه را به‌روز می‌کند.
    // ---------------------------------------------------------------
    bool LoadFirmware(const std::string& filepath) {
        std::fill(flashMemory.begin(), flashMemory.end(), static_cast<uint8_t>(0xFF));
        IntelHexLoader::Result r = IntelHexLoader::LoadFile(filepath, flashMemory);
        firmwareLoaded = r.success;
        lastLoadMessage = r.message;
        lastBytesLoaded = r.bytesLoaded;
        if (r.success) firmwarePath = filepath;
        return r.success;
    }

    // خواندن یک بایت (Opcode/داده) از حافظه‌ی فلش - برای استفاده‌ی آینده
    // توسط یک شبیه‌ساز اجرای برنامه (خارج از محدوده‌ی بخش ۳.۷)
    uint8_t ReadFlashByte(unsigned int address) const {
        if (address >= flashMemory.size()) return 0xFF;
        return flashMemory[address];
    }

    std::vector<PropertyField> GetProperties() override {
        return {
            {"Label", label},
            {"Flash Size (bytes)", std::to_string(flashSizeBytes)},
            {"Firmware File (.hex)", firmwarePath},
            {"Status", lastLoadMessage},
            {"PC", std::to_string(pc.Get())},
            {"ACC", std::to_string(regs.GetACC())},
            {"Cycle Speed (ms)", std::to_string(cycleIntervalMs)}
        };
    }

    void SetProperties(const std::vector<std::string>& values) override {
        if (values.size() >= 1) label = values[0];
        if (values.size() >= 2) {
            try {
                int newSize = std::max(256, std::stoi(values[1]));
                if (newSize != flashSizeBytes) {
                    flashSizeBytes = newSize;
                    flashMemory.assign(static_cast<size_t>(flashSizeBytes), 0xFF);
                    firmwareLoaded = false;
                    lastLoadMessage = "اندازه‌ی فلش تغییر کرد - فریمور را دوباره بارگذاری کنید";
                }
            } catch (...) {}
        }
        // بارگذاری فریمور فقط وقتی مسیر جدید/متفاوتی وارد شده باشد اجرا می‌شود
        if (values.size() >= 3 && !values[2].empty() && values[2] != firmwarePath) {
            LoadFirmware(values[2]);
        }
        // فیلد "Status" فقط نمایشی است؛ ورودی کاربر در آن نادیده گرفته می‌شود
        if (values.size() >= 5) { try { pc.Set(static_cast<uint16_t>(std::stoi(values[4]))); } catch (...) {} }
        if (values.size() >= 6) { try { regs.SetACC(static_cast<uint8_t>(std::stoi(values[5]))); } catch (...) {} }
        if (values.size() >= 7) { try { cycleIntervalMs = std::max(1, std::stoi(values[6])); } catch (...) {} }
    }

    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 10,
                                          isSelected ? 150 : 10,
                                          isSelected ? 255 : 10, 255);
        DrawLine(renderer, 10, 0, width - 10, 0);
        DrawLine(renderer, width - 10, 0, width - 10, height);
        DrawLine(renderer, width - 10, height, 10, height);
        DrawLine(renderer, 10, height, 10, 0);
        DrawLine(renderer, 10, 10, 18, 0); // بریدگی جهت‌یاب گوشه‌ی چیپ

        // پایه‌های سمت چپ: VCC، GND، RESET و پورت B
        for (int i = 0; i < PORTA_START && i < static_cast<int>(pins.size()); ++i) {
            const float py = pins[i].offsetY;
            DrawLine(renderer, 0, py, 10, py);
        }
        // پایه‌های سمت راست: پورت A
        for (size_t i = PORTA_START; i < pins.size(); ++i) {
            const float py = pins[i].offsetY;
            DrawLine(renderer, width - 10, py, width, py);
        }

        // چون این کلاس به رندر متن دسترسی ندارد، وضعیت بارگذاری با یک
        // نشانگر ساده در مرکز چیپ نمایش داده می‌شود: علامت + برای موفق،
        // علامت × برای بدون فریمور/خطا.
        if (firmwareLoaded) {
            DrawLine(renderer, width / 2.0f - 6, height / 2.0f, width / 2.0f + 6, height / 2.0f);
            DrawLine(renderer, width / 2.0f, height / 2.0f - 6, width / 2.0f, height / 2.0f + 6);
        } else {
            DrawLine(renderer, width / 2.0f - 6, height / 2.0f - 6, width / 2.0f + 6, height / 2.0f + 6);
            DrawLine(renderer, width / 2.0f - 6, height / 2.0f + 6, width / 2.0f + 6, height / 2.0f - 6);
        }
    }
};

// ==========================================
// 17. حافظه‌ی خارجی - RAM/EEPROM (بخش ۷.۷)
// ==========================================
// باس آدرس (۸ خط) و باس داده (۸ خط) به‌صورت انتزاعی با پایه‌های قطعه
// پیاده‌سازی شده‌اند. میکروکنترلر با تغییر ولتاژ پایه‌های RD/WR و آدرس
// (از طریق پورت‌ها و سیم‌کشی روی بوم)، محتوای این حافظه را می‌خواند یا
// در آن می‌نویسد.
class ExternalMemoryComponent : public Component {
public:
    std::vector<uint8_t> memory; // ۲۵۶ بایت (متناسب با ۸ خط آدرس)

    static constexpr int ADDR_START = 0;  // پایه‌های ۰..۷  = A0..A7
    static constexpr int DATA_START = 8;  // پایه‌های ۸..۱۵ = D0..D7
    static constexpr int PIN_RD = 16;
    static constexpr int PIN_WR = 17;

    ExternalMemoryComponent(float x, float y) : Component(x, y, ComponentType::EXTERNAL_MEMORY) {
        label = "RAM/EEPROM";
        width = 110.0f; height = 170.0f;
        memory.assign(256, 0x00);
        RebuildPins();
    }

    void RebuildPins() {
        pins.clear();
        const float top = 10.0f;
        const float leftSpacing = (height - 2.0f * top) / 7.0f;
        for (int i = 0; i < 8; ++i) {
            pins.push_back({"A" + std::to_string(i), 0.0f, top + i * leftSpacing, false});
        }
        const float rightSpacing = (height - 2.0f * top) / 7.0f;
        for (int i = 0; i < 8; ++i) {
            pins.push_back({"D" + std::to_string(i), width, top + i * rightSpacing, true});
        }
        pins.push_back({"RD", width * 0.35f, height, false});
        pins.push_back({"WR", width * 0.65f, height, false});
    }

    static bool IsHigh(float v) { return v >= 2.5f; }

    void Update() override {
        if (pins.size() < static_cast<size_t>(PIN_WR + 1)) return;

        uint8_t addr = 0;
        for (int i = 0; i < 8; ++i) if (IsHigh(pins[ADDR_START + i].voltage)) addr |= static_cast<uint8_t>(1u << i);

        const bool writeEnabled = IsHigh(pins[PIN_WR].voltage);
        const bool readEnabled  = IsHigh(pins[PIN_RD].voltage);

        if (writeEnabled) {
            uint8_t data = 0;
            for (int i = 0; i < 8; ++i) if (IsHigh(pins[DATA_START + i].voltage)) data |= static_cast<uint8_t>(1u << i);
            memory[addr] = data;
            for (int i = 0; i < 8; ++i) pins[DATA_START + i].isOutput = false; // باس داده در حالت نوشتن توسط منبع بیرونی رانده می‌شود
        } else if (readEnabled) {
            const uint8_t data = memory[addr];
            for (int i = 0; i < 8; ++i) {
                pins[DATA_START + i].isOutput = true;
                pins[DATA_START + i].voltage = ((data >> i) & 1) ? 5.0f : 0.0f;
            }
        } else {
            for (int i = 0; i < 8; ++i) pins[DATA_START + i].isOutput = false; // حالت پرظرفیت (Hi-Z)
        }
    }

    std::vector<PropertyField> GetProperties() override {
        return {
            {"Label", label},
            {"Size (bytes)", std::to_string(memory.size())}
        };
    }
    void SetProperties(const std::vector<std::string>& values) override {
        if (!values.empty()) label = values[0];
    }

    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 60, isSelected ? 150 : 60, isSelected ? 255 : 140, 255);
        DrawLine(renderer, 10, 0, width - 10, 0);
        DrawLine(renderer, width - 10, 0, width - 10, height);
        DrawLine(renderer, width - 10, height, 10, height);
        DrawLine(renderer, 10, height, 10, 0);
        for (int i = 0; i < 8; ++i) { const float py = pins[ADDR_START + i].offsetY; DrawLine(renderer, 0, py, 10, py); }
        for (int i = 0; i < 8; ++i) { const float py = pins[DATA_START + i].offsetY; DrawLine(renderer, width - 10, py, width, py); }
        DrawLine(renderer, pins[PIN_RD].offsetX, height - 8, pins[PIN_RD].offsetX, height);
        DrawLine(renderer, pins[PIN_WR].offsetX, height - 8, pins[PIN_WR].offsetX, height);
    }
};

// ==========================================
// 18. نمایشگر کاراکتری LCD 16x2 (بخش ۷.۸)
// ==========================================
// فرامین ورودی (روی گذرگاه RS/EN/D0..D7) مطابق منطق ساده‌شده‌ی HD44780:
//   RS=0 و داده=0x01              -> پاک کردن صفحه (Clear Display)
//   RS=0 و داده دارای بیت 0x80    -> تنظیم آدرس مکان‌نما (DDRAM Address)
//   RS=1                          -> نوشتن یک کاراکتر در مکان‌نمای فعلی
// لچ‌کردن داده با لبه‌ی بالارونده‌ی پایه‌ی EN انجام می‌شود (دقیقاً مثل
// ال‌سی‌دی‌های واقعی).
class LCD1602Component : public Component {
public:
    static constexpr int COLS = 16;
    static constexpr int ROWS = 2;
    char screenBuffer[ROWS][COLS];
    int cursorRow = 0, cursorCol = 0;
    bool prevEnableHigh = false;

    static constexpr int PIN_RS = 0;
    static constexpr int PIN_EN = 1;
    static constexpr int DATA_START = 2; // D0..D7 -> پایه‌های ۲..۹

    LCD1602Component(float x, float y) : Component(x, y, ComponentType::LCD_1602) {
        label = "LCD 16x2";
        width = 200.0f; height = 76.0f;
        for (auto& row : screenBuffer) for (auto& c : row) c = ' ';
        RebuildPins();
    }

    void RebuildPins() {
        pins.clear();
        pins.push_back({"RS", 12.0f, height, false});
        pins.push_back({"EN", 30.0f, height, false});
        for (int i = 0; i < 8; ++i) {
            pins.push_back({"D" + std::to_string(i), 55.0f + i * 17.0f, height, false});
        }
    }

    static bool IsHigh(float v) { return v >= 2.5f; }

    void Update() override {
        if (pins.size() < static_cast<size_t>(DATA_START + 8)) return;
        const bool enableHigh = IsHigh(pins[PIN_EN].voltage);

        if (enableHigh && !prevEnableHigh) { // لبه‌ی بالارونده‌ی EN: لچ کردن باس داده
            const bool registerSelect = IsHigh(pins[PIN_RS].voltage);
            uint8_t data = 0;
            for (int i = 0; i < 8; ++i) if (IsHigh(pins[DATA_START + i].voltage)) data |= static_cast<uint8_t>(1u << i);

            if (!registerSelect) { // فرمان
                if (data == 0x01) { // Clear Display
                    for (auto& row : screenBuffer) for (auto& c : row) c = ' ';
                    cursorRow = 0; cursorCol = 0;
                } else if (data & 0x80) { // Set DDRAM Address / تنظیم مکان‌نما
                    const uint8_t addr = data & 0x7F;
                    if (addr < 0x40) { cursorRow = 0; cursorCol = std::min<int>(addr, COLS - 1); }
                    else { cursorRow = 1; cursorCol = std::min<int>(addr - 0x40, COLS - 1); }
                }
            } else { // داده: نوشتن یک کاراکتر در مکان‌نمای فعلی
                if (cursorRow < ROWS && cursorCol < COLS) screenBuffer[cursorRow][cursorCol] = static_cast<char>(data);
                cursorCol++;
                if (cursorCol >= COLS) { cursorCol = 0; cursorRow = (cursorRow + 1) % ROWS; }
            }
        }
        prevEnableHigh = enableHigh;
    }

    // برای دسترسیِ EditorPage جهت رسم متن واقعیِ خط با TextRenderer
    // (خودِ Component به رندر متن دسترسی ندارد).
    std::string GetLine(int row) const {
        if (row < 0 || row >= ROWS) return "";
        return std::string(screenBuffer[row], COLS);
    }

    std::vector<PropertyField> GetProperties() override {
        return {
            {"Label", label},
            {"Line 1", GetLine(0)},
            {"Line 2", GetLine(1)}
        };
    }
    void SetProperties(const std::vector<std::string>& values) override {
        if (!values.empty()) label = values[0];
        // خطوط نمایش فقط خواندنی هستند؛ محتوایشان توسط شبیه‌سازیِ میکرو پر می‌شود
    }

    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 90, isSelected ? 255 : 40, 255);
        DrawLine(renderer, 0, 0, width, 0);
        DrawLine(renderer, width, 0, width, height - 16);
        DrawLine(renderer, width, height - 16, 0, height - 16);
        DrawLine(renderer, 0, height - 16, 0, 0);
        for (const auto& p : pins) DrawLine(renderer, p.offsetX, height - 16, p.offsetX, height);

        // زمینه‌ی سبزِ صفحه‌نمایش (متن واقعی توسط EditorPage با TextRenderer رسم می‌شود)
        SDL_SetRenderDrawColor(renderer, 200, 230, 200, 255);
        SDL_FRect screenRect = {4, 4, width - 8, height - 24};
        auto p1 = LocalToWorld(screenRect.x, screenRect.y);
        auto p2 = LocalToWorld(screenRect.x + screenRect.w, screenRect.y + screenRect.h);
        SDL_FRect worldRect = {std::min(p1.first, p2.first), std::min(p1.second, p2.second),
                                std::fabs(p2.first - p1.first), std::fabs(p2.second - p1.second)};
        SDL_RenderFillRect(renderer, &worldRect);
    }
};

// ==========================================
// 19. صفحه‌کلید ماتریسی ۴×۴ (بخش ۷.۹)
// ==========================================
// یک قطعه‌ی کاملاً غیرفعال (بدون منبع ولتاژ داخلی): با کلیک ماوس روی هر
// خانه، آن کلید «فشرده‌شده» علامت می‌خورد و در هر سیکل، ولتاژِ سطر
// متصل را مستقیماً به ستونِ متناظر منتقل می‌کند - دقیقاً مثل یک سوییچِ
// فیزیکی در تقاطعِ سطر/ستون. میکروکنترلر با روشن‌کردن پیاپیِ سطرها
// (خروجی) و خواندنِ ستون‌ها (ورودی)، کلید فشرده‌شده را طیِ اسکنِ متوالی
// تشخیص می‌دهد.
class Keypad4x4Component : public Component {
public:
    static constexpr int SIZE = 4;
    bool keyPressed[SIZE][SIZE] = {};

    Keypad4x4Component(float x, float y) : Component(x, y, ComponentType::KEYPAD_4X4) {
        label = "KEYPAD 4x4";
        width = 140.0f; height = 160.0f;
        RebuildPins();
    }

    void RebuildPins() {
        pins.clear();
        for (int i = 0; i < SIZE; ++i) pins.push_back({"ROW" + std::to_string(i), 0.0f, gridTop + i * cell + cell / 2.0f, false});
        for (int i = 0; i < SIZE; ++i) pins.push_back({"COL" + std::to_string(i), width, gridTop + i * cell + cell / 2.0f, true});
    }

    bool HandleClick(float mouseX, float mouseY) override {
        const float lx = mouseX - x, ly = mouseY - y;
        if (lx < 6.0f || lx > width - 6.0f || ly < gridTop || ly > gridTop + cell * SIZE) return false;
        const int col = static_cast<int>((lx - 6.0f) / ((width - 12.0f) / SIZE));
        const int row = static_cast<int>((ly - gridTop) / cell);
        if (row < 0 || row >= SIZE || col < 0 || col >= SIZE) return false;
        keyPressed[row][col] = !keyPressed[row][col];
        return true;
    }

    void Update() override {
        // اتصال مستقیمِ ولتاژِ سطرِ فعال به ستونِ متناظر، برای هر کلیدِ فشرده‌شده
        for (int c = 0; c < SIZE; ++c) {
            bool driven = false;
            float v = 0.0f;
            for (int r = 0; r < SIZE; ++r) {
                if (keyPressed[r][c]) { driven = true; v = pins[r].voltage; break; }
            }
            pins[SIZE + c].isOutput = driven;
            if (driven) pins[SIZE + c].voltage = v;
        }
    }

    std::vector<PropertyField> GetProperties() override {
        std::string pressedList;
        for (int r = 0; r < SIZE; ++r)
            for (int c = 0; c < SIZE; ++c)
                if (keyPressed[r][c]) pressedList += "(" + std::to_string(r) + "," + std::to_string(c) + ") ";
        if (pressedList.empty()) pressedList = "-";
        return { {"Label", label}, {"Pressed Keys", pressedList} };
    }
    void SetProperties(const std::vector<std::string>& values) override {
        if (!values.empty()) label = values[0];
    }

    void Draw(SDL_Renderer* renderer) override {
        SDL_SetRenderDrawColor(renderer, isSelected ? 0 : 0, isSelected ? 150 : 0, isSelected ? 255 : 0, 255);
        DrawLine(renderer, 6, gridTop, width - 6, gridTop);
        DrawLine(renderer, width - 6, gridTop, width - 6, gridTop + cell * SIZE);
        DrawLine(renderer, width - 6, gridTop + cell * SIZE, 6, gridTop + cell * SIZE);
        DrawLine(renderer, 6, gridTop + cell * SIZE, 6, gridTop);

        for (int r = 0; r < SIZE; ++r) {
            for (int c = 0; c < SIZE; ++c) {
                const float cx0 = 6.0f + c * (width - 12.0f) / SIZE;
                const float cy0 = gridTop + r * cell;
                const float cx1 = 6.0f + (c + 1) * (width - 12.0f) / SIZE;
                const float cy1 = gridTop + (r + 1) * cell;
                if (keyPressed[r][c]) {
                    SDL_SetRenderDrawColor(renderer, 255, 210, 0, 255);
                    SDL_FRect keyRect = {cx0 + 2, cy0 + 2, (cx1 - cx0) - 4, (cy1 - cy0) - 4};
                    auto p1 = LocalToWorld(keyRect.x, keyRect.y);
                    auto p2 = LocalToWorld(keyRect.x + keyRect.w, keyRect.y + keyRect.h);
                    SDL_FRect worldRect = {std::min(p1.first, p2.first), std::min(p1.second, p2.second),
                                            std::fabs(p2.first - p1.first), std::fabs(p2.second - p1.second)};
                    SDL_RenderFillRect(renderer, &worldRect);
                    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                }
                DrawLine(renderer, cx0, cy0, cx1, cy0);
                DrawLine(renderer, cx1, cy0, cx1, cy1);
                DrawLine(renderer, cx1, cy1, cx0, cy1);
                DrawLine(renderer, cx0, cy1, cx0, cy0);
            }
        }
        for (int i = 0; i < SIZE; ++i) DrawLine(renderer, 0, pins[i].offsetY, 6, pins[i].offsetY);
        for (int i = 0; i < SIZE; ++i) DrawLine(renderer, width - 6, pins[SIZE + i].offsetY, width, pins[SIZE + i].offsetY);
    }

private:
    static constexpr float gridTop = 20.0f;
    static constexpr float cell = 30.0f;
};

#endif
