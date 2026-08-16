#ifndef MCUCORE_H
#define MCUCORE_H

#include <cstdint>
#include <vector>
#include <array>
#include <algorithm>

// ================================================================
// بخش ۷.۴ - ساختار داخلی شیءگرای میکرو
// ================================================================
// این فایل «قلب تپنده‌ی» شبیه‌ساز میکروکنترلر را می‌سازد: هر مسئولیت در
// کلاس مجزای خودش پیاده شده تا کاملاً با رویکرد شیءگرا سازگار باشد.
// این کلاس‌ها به‌عمد از MicrocontrollerComponent (که در Component.h است)
// جدا نگه داشته شده‌اند تا «نگهداری وضعیتِ ثبات/حافظه/پورت» از
// «نمایش گرافیکی + پایه‌های فیزیکیِ قطعه روی بوم» تفکیک بماند.
// ================================================================

// ----------------------------------------------------------------
// Program Counter (PC)
// آدرس دستورالعمل فعلی در حافظه‌ی Flash را نگه می‌دارد و پس از هر
// دستور به‌روزرسانی می‌شود.
// ----------------------------------------------------------------
class ProgramCounter {
public:
    uint16_t Get() const { return address; }
    void Set(uint16_t addr) { address = addr; }
    void Advance(uint16_t bytes) { address = static_cast<uint16_t>(address + bytes); }
    void Reset() { address = 0; }
private:
    uint16_t address = 0;
};

// ----------------------------------------------------------------
// ثبات‌های کاربردی (Registers)
// شامل ثبات انباره (Accumulator) و هشت ثبات عمومی R0..R7، هر دو
// به‌صورت متغیرهای عضو کپسوله‌شده در همین کلاس.
// ----------------------------------------------------------------
class RegisterFile {
public:
    uint8_t GetACC() const { return acc; }
    void SetACC(uint8_t v) { acc = v; }

    uint8_t GetR(int index) const {
        return (index >= 0 && index < static_cast<int>(r.size())) ? r[index] : 0;
    }
    void SetR(int index, uint8_t v) {
        if (index >= 0 && index < static_cast<int>(r.size())) r[index] = v;
    }

    void Reset() { r.fill(0); acc = 0; }
private:
    std::array<uint8_t, 8> r{};
    uint8_t acc = 0;
};

// ----------------------------------------------------------------
// RAM داخلی (حافظه‌ی موقت حین اجرای برنامه)
// ----------------------------------------------------------------
class InternalRAM {
public:
    explicit InternalRAM(size_t sizeBytes = 256) : mem(sizeBytes, 0) {}

    uint8_t Read(uint16_t addr) const {
        return addr < mem.size() ? mem[addr] : 0;
    }
    void Write(uint16_t addr, uint8_t value) {
        if (addr < mem.size()) mem[addr] = value;
    }
    void Reset() { std::fill(mem.begin(), mem.end(), 0); }
    size_t Size() const { return mem.size(); }
private:
    std::vector<uint8_t> mem;
};

// ----------------------------------------------------------------
// بخش ۷.۶ - مدیریت پورت ورودی/خروجی
// یک Port، هشت بیت را مدیریت می‌کند (شبیه Port A / Port B). هر بیت
// می‌تواند مستقل به‌صورت ورودی یا خروجی پیکربندی شود (شبیه‌سازیِ
// سادهٔ ثبات جهت‌دهی/DDR). خودِ Port از ولتاژ فیزیکیِ پایه‌ها خبر
// ندارد؛ MicrocontrollerComponent مسئول تبدیل بیت<->ولتاژ روی
// پایه‌های واقعیِ قطعه (برای اثرگذاری روی قطعات جانبیِ متصل) است.
// ----------------------------------------------------------------
class Port {
public:
    void SetBitDirection(int bit, bool isOutput) {
        if (bit < 0 || bit > 7) return;
        if (isOutput) direction |= static_cast<uint8_t>(1u << bit);
        else direction &= static_cast<uint8_t>(~(1u << bit));
    }
    bool IsOutput(int bit) const {
        return bit >= 0 && bit < 8 && (direction & (1u << bit));
    }

    void WriteBit(int bit, bool high) {
        if (bit < 0 || bit > 7) return;
        if (high) outputLatch |= static_cast<uint8_t>(1u << bit);
        else outputLatch &= static_cast<uint8_t>(~(1u << bit));
    }
    bool ReadOutputBit(int bit) const {
        return bit >= 0 && bit < 8 && (outputLatch & (1u << bit));
    }

    // مقدار خوانده‌شده از پایه‌ی فیزیکی (وقتی بیت به‌صورت ورودی است)
    void SetInputBit(int bit, bool high) {
        if (bit < 0 || bit > 7) return;
        if (high) inputSnapshot |= static_cast<uint8_t>(1u << bit);
        else inputSnapshot &= static_cast<uint8_t>(~(1u << bit));
    }
    bool ReadInputBit(int bit) const {
        return bit >= 0 && bit < 8 && (inputSnapshot & (1u << bit));
    }

    // خواندن منطقیِ یک بیت، صرف‌نظر از جهت آن (برای دستورات اسمبلی)
    bool ReadBit(int bit) const {
        return IsOutput(bit) ? ReadOutputBit(bit) : ReadInputBit(bit);
    }

    void WriteByte(uint8_t v) { outputLatch = v; direction = 0xFF; }
    uint8_t ReadByte() const {
        uint8_t result = 0;
        for (int i = 0; i < 8; ++i) if (ReadBit(i)) result |= static_cast<uint8_t>(1u << i);
        return result;
    }

private:
    uint8_t direction = 0x00;      // 0 = ورودی، 1 = خروجی (به ازای هر بیت)
    uint8_t outputLatch = 0x00;    // مقداری که هنگام خروجی‌بودن روی پایه می‌رود
    uint8_t inputSnapshot = 0x00;  // آخرین مقدار خوانده‌شده از پایه (هنگام ورودی‌بودن)
};

// ----------------------------------------------------------------
// بخش ۷.۵ - رمزگشای دستورات (Instruction Decoder)
// ----------------------------------------------------------------
// این پروژه هیچ فایل تعریف رسمیِ opcode را برای شما مشخص نکرده بود؛
// بنابراین یک قالبِ ساده و مستندِ ۴بایتی برای هر دستور طراحی شده که
// می‌توانید بعداً با جدول Opcode واقعیِ یک معماری خاص (مثلاً 8051)
// جایگزینش کنید — فقط کافی است تابع Fetch و enum زیر را عوض کنید؛
// بقیه‌ی سیستم (اجرای دستور در MicrocontrollerComponent) با همین
// ساختار Instruction کار می‌کند.
//
// قالب هر دستور (۴ بایت پیاپی در حافظه‌ی Flash):
//   بایت ۰: OpCode
//   MOV  [op1=Dest][op2=Mode][op3=Value]
//   ADD  [op1=Dest][op2=Mode][op3=Value]
//        Dest: 0..7 = Rn ،‌ 8 = ACC
//        Mode: 0 = Immediate، 1 = Register(Rn)، 2 = RAM Address
//   JMP  [op1=AddrHigh][op2=AddrLow][op3=-]
//   SETB [op1=PortId][op2=BitIndex][op3=-]   PortId: 0=PortA، 1=PortB
//   CLR  [op1=PortId][op2=BitIndex][op3=-]
// مقدار 0xFF (حافظه‌ی خالی/بدون فریمور) به‌عنوان NOP در نظر گرفته می‌شود.
// ----------------------------------------------------------------
enum class OpCode : uint8_t {
    NOP  = 0x00,
    MOV  = 0x01,
    ADD  = 0x02,
    JMP  = 0x03,
    SETB = 0x04,
    CLR  = 0x05
};

struct Instruction {
    OpCode op = OpCode::NOP;
    uint8_t operand1 = 0;
    uint8_t operand2 = 0;
    uint8_t operand3 = 0;
    static constexpr uint16_t SIZE_BYTES = 4; // اندازه‌ی ثابتِ هر دستور (ساده‌سازیِ شبیه‌ساز)
};

class InstructionDecoder {
public:
    // خواندنِ ۴ بایت از حافظه‌ی Flash در آدرس pc و رمزگشاییِ آن‌ها به
    // یک Instruction قابل‌اجرا. این تابع فقط «می‌خواند و تفسیر می‌کند»؛
    // اجرای واقعیِ دستور (تغییر ثبات/RAM/پورت) در MicrocontrollerComponent
    // انجام می‌شود، چون آن کلاس مالکِ وضعیتِ سخت‌افزاری قطعه است.
    static Instruction Fetch(const std::vector<uint8_t>& flash, uint16_t pc) {
        Instruction ins;
        auto byteAt = [&](uint16_t addr) -> uint8_t {
            return addr < flash.size() ? flash[addr] : 0xFF;
        };
        const uint8_t raw = byteAt(pc);
        ins.op = (raw <= static_cast<uint8_t>(OpCode::CLR)) ? static_cast<OpCode>(raw) : OpCode::NOP;
        ins.operand1 = byteAt(static_cast<uint16_t>(pc + 1));
        ins.operand2 = byteAt(static_cast<uint16_t>(pc + 2));
        ins.operand3 = byteAt(static_cast<uint16_t>(pc + 3));
        return ins;
    }
};

#endif
