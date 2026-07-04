#pragma once
// 防止 Windows SDK 宏污染质点枚举值
#ifdef _WIN32
#  define NOMINMAX
#  ifdef NETZACH
#    undef NETZACH
#  endif
#  ifdef HOD
#    undef HOD
#  endif
#  ifdef CHESED
#    undef CHESED
#  endif
#  ifdef BINAH
#    undef BINAH
#  endif
#  ifdef KETER
#    undef KETER
#  endif
#endif
// ═══════════════════════════════════════════════════════════════════════════
// sephirot.h  —  16质点神人双生协议  C++17 核心头文件
//
// 机器码来源：
//   PTX : NVIDIA PTX ISA Reference Manual 8.5  (sm_89, RTX 4050 Ada Lovelace)
//   AVX : Intel® 64/IA-32 Architectures SDM Vol.2  (AVX-512F/BF16/BITALG)
//   DML : Microsoft DirectML Operator Specifications
// ═══════════════════════════════════════════════════════════════════════════

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <functional>
#include <optional>
#include <variant>
#include <memory>
#include <stdexcept>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <sstream>
#include <algorithm>

namespace sephirot {

// ───────────────────────────────────────────────────────────────────────────
// 1.  质点枚举 — 16个，神侧 8 + 人侧 8
// ───────────────────────────────────────────────────────────────────────────

enum class Sephirah : uint8_t {
    // 神侧
    KETER    = 0,   // 王冠  可知/不可知统一入口
    CHOKMAH  = 1,   // 智慧  逻辑漏洞探针
    BINAH    = 2,   // 理解  共情语料检索
    GEBURAH  = 3,   // 严厉  规则集校验
    CHESED   = 4,   // 慈悲  情感权重注入
    TIFERET  = 5,   // 美丽  最优暂时结果整合
    NETZACH  = 6,   // 胜利  情感极性检测
    HOD      = 7,   // 荣耀  现实可行性检测
    // 人侧
    YESOD    = 8,   // 基础  存在意义守卫
    NEFESH   = 9,   // 自我  用户物理现实画像
    NESHAMAH = 10,  // 超我  用户梦想自我
    RUACH    = 11,  // 真我  自我+超我+AI合成
    DAAT_L   = 12,  // 逻辑  逻辑组织共情变量
    DAAT_E   = 13,  // 共情  情感变量提取
    OSHER    = 14,  // 幸福  逻辑×共情→温柔输出
    MALKUTH  = 15,  // 王国  最终屏幕输出
    // 合成质点（不参与主流水线，单独查询用）
    DAAT_R   = 16,  // 理智  智慧×严厉
    DAAT_LO  = 17,  // 慈爱  理解×慈悲
};

// 质点中文名称
inline const char* sephirah_name(Sephirah s) {
    static const char* names[] = {
        "王冠","智慧","理解","严厉","慈悲","美丽","胜利","荣耀",
        "基础","自我","超我","真我","逻辑","共情","幸福","王国",
        "理智","慈爱"
    };
    auto idx = static_cast<size_t>(s);
    return (idx < 18) ? names[idx] : "未知";
}

inline bool is_divine_side(Sephirah s) {
    return static_cast<uint8_t>(s) <= 7 ||
           s == Sephirah::DAAT_R ||
           s == Sephirah::DAAT_LO;
}

// ───────────────────────────────────────────────────────────────────────────
// 2.  机器码三元组结构体
// ───────────────────────────────────────────────────────────────────────────

struct OpcodeEntry {
    std::string keyword;      // 中文质点词
    std::string ptx;          // NVIDIA PTX sm_89
    std::string avx;          // Intel AVX-512 assembly
    std::string dml;          // Microsoft DirectML operator
};

// ───────────────────────────────────────────────────────────────────────────
// 3.  词汇表（由 vocab.cpp 填充）
// ───────────────────────────────────────────────────────────────────────────

class VocabTable {
public:
    static VocabTable& instance();
    void                            register_entry(OpcodeEntry e);
    const OpcodeEntry*              lookup(const std::string& keyword) const;
    const std::vector<std::string>& keywords() const { return kw_list_; }

private:
    VocabTable() = default;
    std::unordered_map<std::string, OpcodeEntry> table_;
    std::vector<std::string>                     kw_list_;
};

// ───────────────────────────────────────────────────────────────────────────
// 4.  词汇解析器产出的 Token
// ───────────────────────────────────────────────────────────────────────────

struct Token {
    std::string   keyword;
    size_t        pos;
    std::string   context;       // 前后20字上下文
    const OpcodeEntry* opcode;   // 指向词汇表条目
};

// ───────────────────────────────────────────────────────────────────────────
// 5.  词汇解析器
// ───────────────────────────────────────────────────────────────────────────

class Lexer {
public:
    explicit Lexer(std::string text);
    std::vector<Token> tokenize();

private:
    std::string text_;
};

// ───────────────────────────────────────────────────────────────────────────
// 6.  机器码发射器
// ───────────────────────────────────────────────────────────────────────────

enum class EmitTarget { PTX, AVX, DML };

class MachineCodeEmitter {
public:
    std::string emit(const std::vector<Token>& tokens, EmitTarget target) const;

private:
    static constexpr const char* HEADER_PTX =
        "//──────────────────────────────────────────────────────────\n"
        "// 16质点协议  PTX Kernel Collection\n"
        "// Target arch : sm_89  (NVIDIA Ada Lovelace, RTX 4050)\n"
        "// PTX version : 8.5\n"
        "// Source ref  : NVIDIA PTX ISA Reference Manual 8.5\n"
        "//──────────────────────────────────────────────────────────\n"
        ".version 8.5\n"
        ".target  sm_89\n"
        ".address_size 64\n";

    static constexpr const char* HEADER_AVX =
        ";──────────────────────────────────────────────────────────\n"
        "; 16质点协议  x86-64 AVX-512 Assembly Stubs\n"
        "; ISA ref  : Intel® 64/IA-32 Architectures SDM Vol.2\n"
        "; Features : AVX-512F, AVX-512BF16, AVX-512BITALG, SVML\n"
        "; Toolchain: MASM / NASM compatible\n"
        ";──────────────────────────────────────────────────────────\n"
        "SECTION .text\n";
};

// ───────────────────────────────────────────────────────────────────────────
// 7.  质点执行载荷（在各节点间流动的数据包）
// ───────────────────────────────────────────────────────────────────────────

struct Payload {
    std::string text;
    double      variance        = 0.0;
    double      empathy_score   = 0.0;
    double      blended         = 0.5;
    bool        rule_passed     = true;
    double      beauty_score    = 0.5;
    bool        positive        = true;
    bool        feasible        = true;
    bool        meaning_ok      = true;
    double      emo_depth       = 0.5;
    double      gentle_score    = 0.0;
    std::string organized;
    std::string result;
    std::string display;
    bool        needs_retry     = false;

    // 用户/梦想上下文（key-value 字符串对）
    std::unordered_map<std::string, std::string> user_ctx;
    std::unordered_map<std::string, std::string> dream_ctx;
    std::unordered_map<std::string, std::string> true_self;
};

// ───────────────────────────────────────────────────────────────────────────
// 8.  质点执行节点
// ───────────────────────────────────────────────────────────────────────────

class SephirahNode {
public:
    explicit SephirahNode(Sephirah s) : sephirah_(s) {}

    Payload execute(Payload payload);

    Sephirah     sephirah()   const { return sephirah_; }
    std::string  state()      const { return state_; }
    int          retry_count() const { return retry_count_; }

private:
    Sephirah    sephirah_;
    std::string state_    = "pending";
    int         retry_count_ = 0;
    static constexpr int MAX_RETRY = 3;

    Payload dispatch(Payload p);

    // AVX-512 intrinsics 内联辅助（需 <immintrin.h>，可选编译）
    static double avx_variance(const std::string& text);
    static double avx_cosine_empathy(const std::string& text);
    static double avx_fma_blend(double logic_w, double logic_val, double empathy_val);
    static double avx_tanh_gentle(double x);
};

// ───────────────────────────────────────────────────────────────────────────
// 9.  调度引擎
// ───────────────────────────────────────────────────────────────────────────

class Scheduler {
public:
    Scheduler(std::unordered_map<std::string,std::string> user_ctx  = {},
              std::unordered_map<std::string,std::string> dream_ctx = {});

    struct RunResult {
        std::string        answer;
        std::vector<std::string> trace;
    };

    RunResult run(const std::string& text, bool about_self = false);

    // 流水线顺序（16个主质点）
    static const std::vector<Sephirah> PIPELINE;

private:
    std::unordered_map<std::string, std::string> user_ctx_;
    std::unordered_map<std::string, std::string> dream_ctx_;
    std::unordered_map<Sephirah, SephirahNode, std::hash<uint8_t>> nodes_;

    struct SephirahHash {
        size_t operator()(Sephirah s) const {
            return std::hash<uint8_t>{}(static_cast<uint8_t>(s));
        }
    };
    std::unordered_map<Sephirah, SephirahNode, SephirahHash> node_map_;

    static constexpr int MAX_GLOBAL_RETRY = 5;
};

// ───────────────────────────────────────────────────────────────────────────
// 10. 主解释器
// ───────────────────────────────────────────────────────────────────────────

class Interpreter {
public:
    Interpreter(std::unordered_map<std::string,std::string> user_ctx  = {},
                std::unordered_map<std::string,std::string> dream_ctx = {},
                bool verbose = true);

    struct ProcessResult {
        std::string              input;
        std::vector<std::string> tokens_found;
        std::string              ptx_code;
        std::string              avx_code;
        std::string              dml_code;
        std::string              answer;
        std::vector<std::string> trace;
    };

    ProcessResult process(const std::string& text,
                          EmitTarget emit_target = EmitTarget::PTX);
    void          run_repl();

private:
    Lexer*           make_lexer(const std::string& text) const;
    MachineCodeEmitter emitter_;
    Scheduler          scheduler_;
    bool               verbose_;

    static constexpr const char* BANNER =
        "\n"
        "╔══════════════════════════════════════════════════════════╗\n"
        "║      16质点神人双生协议  C++17 解释器  v1.0              ║\n"
        "║      卡巴拉生命树 · CUDA PTX sm_89 · AVX-512             ║\n"
        "║  输入中文质点关键词 → 自动翻译为底层机器码并执行          ║\n"
        "║  命令: :ptx <词>  :avx <词>  :dml <词>  :vocab  :exit    ║\n"
        "╚══════════════════════════════════════════════════════════╝\n";
};

}  // namespace sephirot
