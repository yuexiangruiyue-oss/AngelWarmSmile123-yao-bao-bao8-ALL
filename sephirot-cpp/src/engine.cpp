// ═══════════════════════════════════════════════════════════════════════════
// engine.cpp  —  词汇解析器 · 机器码发射器 · 质点引擎 · 调度器 · 解释器
// ═══════════════════════════════════════════════════════════════════════════

#include "sephirot.h"

#ifdef _WIN32
#  define NOMINMAX    // 禁用 Windows min/max 宏，避免与 std::min/std::max 冲突
#  include <windows.h>
#endif

// AVX-512 intrinsics（可选；需要 /arch:AVX512 或 -mavx512f）
#if defined(__AVX512F__)
#  include <immintrin.h>
#  define HAVE_AVX512 1
#else
#  define HAVE_AVX512 0
#endif

#include <cmath>
#include <set>
#include <numeric>

namespace sephirot {

// ─────────────────────────────────────────────────────────────────────────
// Lexer
// ─────────────────────────────────────────────────────────────────────────

Lexer::Lexer(std::string text) : text_(std::move(text)) {}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    const auto& vt  = VocabTable::instance();
    const auto& kws = vt.keywords();
    size_t      pos = 0;

    while (pos < text_.size()) {
        bool matched = false;
        for (const auto& kw : kws) {
            if (text_.compare(pos, kw.size(), kw) == 0) {
                size_t ctx_start = (pos >= 20) ? pos - 20 : 0;
                size_t ctx_end   = std::min(text_.size(), pos + kw.size() + 20);
                Token tok;
                tok.keyword = kw;
                tok.pos     = pos;
                tok.context = text_.substr(ctx_start, ctx_end - ctx_start);
                tok.opcode  = vt.lookup(kw);
                tokens.push_back(std::move(tok));
                pos += kw.size();
                matched = true;
                break;
            }
        }
        if (!matched) ++pos;
    }
    return tokens;
}

// ─────────────────────────────────────────────────────────────────────────
// MachineCodeEmitter
// ─────────────────────────────────────────────────────────────────────────

std::string MachineCodeEmitter::emit(const std::vector<Token>& tokens,
                                     EmitTarget                target) const {
    std::ostringstream oss;
    if (target == EmitTarget::PTX)      oss << HEADER_PTX << "\n";
    else if (target == EmitTarget::AVX) oss << HEADER_AVX << "\n";
    else                                oss << "; === DirectML Operator Plan ===\n\n";

    std::set<std::string> seen;
    for (const auto& tok : tokens) {
        if (!tok.opcode || seen.count(tok.keyword)) continue;
        seen.insert(tok.keyword);

        const char* side = is_divine_side(Sephirah::KETER) ? "神侧" : "人侧";
        // 简单判断
        static const char* divine_names[] = {
            "王冠","智慧","理解","严厉","慈悲","美丽","胜利","荣耀","理智","慈爱"
        };
        bool div = false;
        for (auto n : divine_names)
            if (tok.keyword == n) { div = true; break; }
        oss << "\n; === " << tok.keyword << "  (" << (div ? "神侧" : "人侧") << ") ===\n";

        switch (target) {
            case EmitTarget::PTX: oss << tok.opcode->ptx; break;
            case EmitTarget::AVX: oss << tok.opcode->avx; break;
            case EmitTarget::DML: oss << tok.opcode->dml; break;
        }
        oss << "\n";
    }
    return oss.str();
}

// ─────────────────────────────────────────────────────────────────────────
// SephirahNode — AVX-512 辅助（退化到标量）
// ─────────────────────────────────────────────────────────────────────────

double SephirahNode::avx_variance(const std::string& text) {
#if HAVE_AVX512
    // 使用 AVX-512F 向量化计算字节方差
    // Intel SDM Vol.2 §VPMOVZXBD + §VCVTDQ2PS + §VSUBPS + §VMULPS
    if (text.size() >= 16) {
        const uint8_t* data = reinterpret_cast<const uint8_t*>(text.data());
        size_t n = text.size();
        // 计算均值
        __m512i sum_v = _mm512_setzero_si512();
        size_t i = 0;
        for (; i + 64 <= n; i += 64) {
            __m512i v = _mm512_loadu_si512(data + i);
            // 累加字节（扩展为16位避免溢出）
            __m256i lo = _mm512_castsi512_si256(v);
            __m256i hi = _mm512_extracti64x4_epi64(v, 1);
            // 累加
            __m512i ext_lo = _mm512_cvtepu8_epi32(_mm256_castsi256_si128(lo));
            sum_v = _mm512_add_epi32(sum_v, ext_lo);
        }
        // 水平求和（fallback scalar for remainder）
        int32_t sum_buf[16];
        _mm512_storeu_si512(sum_buf, sum_v);
        double sum = 0.0;
        for (int k = 0; k < 16; ++k) sum += sum_buf[k];
        for (; i < n; ++i) sum += data[i];
        double mean = sum / n;
        // 方差
        double var = 0.0;
        for (size_t j = 0; j < n; ++j) {
            double d = data[j] - mean;
            var += d * d;
        }
        return var / n;
    }
#endif
    // 标量退化
    if (text.empty()) return 0.0;
    double sum = 0.0;
    for (unsigned char c : text) sum += c;
    double mean = sum / text.size();
    double var  = 0.0;
    for (unsigned char c : text) var += (c - mean) * (c - mean);
    return var / text.size();
}

double SephirahNode::avx_cosine_empathy(const std::string& text) {
    static const char* pain_kws[] = {
        "痛苦", "孤独", "迷茫", "害怕", "无望", "失去", "难过", "崩溃", nullptr
    };
    int count = 0, total = 0;
    for (const char** p = pain_kws; *p; ++p, ++total)
        if (text.find(*p) != std::string::npos) ++count;
    return total > 0 ? (double)count / total : 0.0;
}

double SephirahNode::avx_fma_blend(double logic_w, double logic_val, double empathy_val) {
#if HAVE_AVX512
    // AVX-512F VFMADD231PS (scalar lane)
    __m128 lw = _mm_set_ss((float)logic_w);
    __m128 lv = _mm_set_ss((float)logic_val);
    __m128 ev = _mm_set_ss((float)empathy_val);
    __m128 r  = _mm_fmadd_ss(lv, lw, ev);   // logic_val*logic_w + empathy_val
    return _mm_cvtss_f32(r);
#else
    return logic_w * logic_val + empathy_val;
#endif
}

double SephirahNode::avx_tanh_gentle(double x) {
#if HAVE_AVX512
    // AVX-512F: 使用 _mm_tanh_ps (Intel SVML) 若可用，否则标量
    __m128 v = _mm_set_ss((float)x);
    // SVML: __m128 r = _mm_tanh_ps(v);
    // 若无 SVML 链接，回退标量 tanh
    (void)v;
#endif
    return std::tanh(x);
}

// ─────────────────────────────────────────────────────────────────────────
// SephirahNode::execute / dispatch
// ─────────────────────────────────────────────────────────────────────────

Payload SephirahNode::execute(Payload payload) {
    state_ = "running";
    try {
        Payload out = dispatch(std::move(payload));
        state_ = "done";
        return out;
    } catch (const std::exception& e) {
        state_ = (retry_count_ < MAX_RETRY) ? "retry" : "failed";
        ++retry_count_;
        payload.result = std::string("[") + sephirah_name(sephirah_) + " 错误]: " + e.what();
        return payload;
    }
}

Payload SephirahNode::dispatch(Payload p) {
    switch (sephirah_) {

    // ── 王冠 ─────────────────────────────────────────────────────────────
    case Sephirah::KETER: {
        static const char* unk[] = {"不知道","不确定","未知","无法","无解", nullptr};
        for (const char** kw = unk; *kw; ++kw)
            if (p.text.find(*kw) != std::string::npos)
                p.result = p.text + " [路由:解构]";
        if (p.result.empty()) p.result = p.text + " [路由:任务]";
        return p;
    }

    // ── 智慧 ─────────────────────────────────────────────────────────────
    case Sephirah::CHOKMAH: {
        p.variance   = avx_variance(p.text);
        bool gaps    = p.variance > 500.0;
        p.result     = p.text;
        return p;
    }

    // ── 理解 ─────────────────────────────────────────────────────────────
    case Sephirah::BINAH: {
        p.empathy_score = avx_cosine_empathy(p.text);
        return p;
    }

    // ── 严厉 ─────────────────────────────────────────────────────────────
    case Sephirah::GEBURAH: {
        p.rule_passed = (p.variance <= 500.0);
        return p;
    }

    // ── 慈悲 ─────────────────────────────────────────────────────────────
    case Sephirah::CHESED: {
        p.blended = avx_fma_blend(0.7, p.variance, p.empathy_score);
        return p;
    }

    // ── 美丽 ─────────────────────────────────────────────────────────────
    case Sephirah::TIFERET: {
        p.beauty_score = p.blended * (p.rule_passed ? 1.0 : 0.3);
        p.needs_retry  = (p.beauty_score < 0.1);
        return p;
    }

    // ── 胜利 ─────────────────────────────────────────────────────────────
    case Sephirah::NETZACH: {
        p.positive    = (p.beauty_score > 0.05);
        p.needs_retry = !p.positive;
        return p;
    }

    // ── 荣耀 ─────────────────────────────────────────────────────────────
    case Sephirah::HOD: {
        p.feasible    = !p.result.empty();
        p.needs_retry = !p.feasible;
        return p;
    }

    // ── 基础 ─────────────────────────────────────────────────────────────
    case Sephirah::YESOD: {
        static const char* nihl[] = {
            "毫无意义","全是错的","虚无","世界没有希望",
            "自残","毁灭","愤怒毁","没有价值","你是罪",
            nullptr
        };
        for (const char** p_kw = nihl; *p_kw; ++p_kw) {
            size_t pos;
            std::string repl = "[已屏蔽]";
            std::string kw(*p_kw);
            while ((pos = p.result.find(kw)) != std::string::npos)
                p.result.replace(pos, kw.size(), repl);
        }
        p.meaning_ok = (p.result.find("[已屏蔽]") == std::string::npos);
        return p;
    }

    // ── 自我 ─────────────────────────────────────────────────────────────
    case Sephirah::NEFESH: {
        for (auto& [k, v] : p.user_ctx)
            p.true_self["ego_" + k] = v;
        return p;
    }

    // ── 超我 ─────────────────────────────────────────────────────────────
    case Sephirah::NESHAMAH: {
        for (auto& [k, v] : p.dream_ctx)
            p.true_self["ideal_" + k] = v;
        return p;
    }

    // ── 真我 ─────────────────────────────────────────────────────────────
    case Sephirah::RUACH: {
        p.true_self["ai_insight"] = p.result;
        return p;
    }

    // ── 逻辑 ─────────────────────────────────────────────────────────────
    case Sephirah::DAAT_L: {
        std::ostringstream oss;
        oss << "[逻辑整合] 用户画像: {";
        bool first = true;
        for (auto& [k, v] : p.true_self) {
            if (!first) oss << ", ";
            oss << '"' << k << "\":\"" << v << '"';
            first = false;
        }
        oss << "} | 核心: " << p.result;
        p.organized = oss.str();
        return p;
    }

    // ── 共情 ─────────────────────────────────────────────────────────────
    case Sephirah::DAAT_E: {
        double punctuation_count = 0.0;
        for (unsigned char c : p.organized)
            if (c == 0x80 || c == 0xA1 || c == 0x81)  // 粗略 UTF-8 标点检测
                ++punctuation_count;
        // 更准确：数中文标点
        static const char* punc[] = {"。","！","？","…", nullptr};
        size_t total = 0;
        for (const char** pp = punc; *pp; ++pp) {
            size_t pos = 0;
            while ((pos = p.organized.find(*pp, pos)) != std::string::npos) {
                ++total; pos += strlen(*pp);
            }
        }
        p.emo_depth = p.organized.size() > 0
                      ? (double)total / p.organized.size()
                      : 0.0;
        return p;
    }

    // ── 幸福 ─────────────────────────────────────────────────────────────
    case Sephirah::OSHER: {
        p.gentle_score = avx_tanh_gentle(p.emo_depth * 10.0);
        p.display      = p.organized;  // 实际部署：LLM 改写为温柔语气
        return p;
    }

    // ── 王国 ─────────────────────────────────────────────────────────────
    case Sephirah::MALKUTH: {
        if (p.display.empty()) p.display = p.result;
        return p;
    }

    default:
        return p;
    }
}

// ─────────────────────────────────────────────────────────────────────────
// Scheduler
// ─────────────────────────────────────────────────────────────────────────

const std::vector<Sephirah> Scheduler::PIPELINE = {
    Sephirah::KETER,
    Sephirah::CHOKMAH, Sephirah::GEBURAH,
    Sephirah::BINAH,   Sephirah::CHESED,
    Sephirah::TIFERET,
    Sephirah::NETZACH,
    Sephirah::HOD,
    Sephirah::YESOD,
    Sephirah::NEFESH,  Sephirah::NESHAMAH, Sephirah::RUACH,
    Sephirah::DAAT_L,  Sephirah::DAAT_E,
    Sephirah::OSHER,
    Sephirah::MALKUTH,
};

Scheduler::Scheduler(
    std::unordered_map<std::string,std::string> user_ctx,
    std::unordered_map<std::string,std::string> dream_ctx)
    : user_ctx_(std::move(user_ctx)),
      dream_ctx_(std::move(dream_ctx))
{
    for (Sephirah s : PIPELINE)
        node_map_.emplace(s, SephirahNode(s));
}

Scheduler::RunResult Scheduler::run(const std::string& text, bool about_self) {
    Payload payload;
    payload.text      = text;
    payload.user_ctx  = user_ctx_;
    payload.dream_ctx = dream_ctx_;

    RunResult r;
    int global_retry = 0;

    for (Sephirah s : PIPELINE) {
        auto it = node_map_.find(s);
        if (it == node_map_.end()) continue;

        payload = it->second.execute(std::move(payload));
        r.trace.push_back(std::string("[") + sephirah_name(s) + "] state=" + it->second.state());

        if (payload.needs_retry && global_retry < MAX_GLOBAL_RETRY) {
            ++global_retry;
            r.trace.push_back("  ↩ 退回重算 (retry #" + std::to_string(global_retry) + ")");
            payload.needs_retry = false;
            // 调整参数重算
            if (s == Sephirah::NETZACH)
                payload.beauty_score = std::max(payload.beauty_score * 1.5, 0.1);
            if (s == Sephirah::HOD)
                payload.result = (payload.result.empty() ? "（内容重新生成中）" : payload.result);
        }
    }

    r.answer = payload.display.empty() ? payload.result : payload.display;
    return r;
}

// ─────────────────────────────────────────────────────────────────────────
// Interpreter
// ─────────────────────────────────────────────────────────────────────────

Interpreter::Interpreter(
    std::unordered_map<std::string,std::string> user_ctx,
    std::unordered_map<std::string,std::string> dream_ctx,
    bool verbose)
    : scheduler_(std::move(user_ctx), std::move(dream_ctx)),
      verbose_(verbose)
{}

Interpreter::ProcessResult Interpreter::process(const std::string& text,
                                                  EmitTarget         emit_target) {
    Lexer lx(text);
    auto  tokens = lx.tokenize();

    ProcessResult pr;
    pr.input   = text;
    for (auto& t : tokens) pr.tokens_found.push_back(t.keyword);

    pr.ptx_code = emitter_.emit(tokens, EmitTarget::PTX);
    pr.avx_code = emitter_.emit(tokens, EmitTarget::AVX);
    pr.dml_code = emitter_.emit(tokens, EmitTarget::DML);

    bool about_self = text.find("我") != std::string::npos ||
                      text.find("自己") != std::string::npos;
    auto res = scheduler_.run(text, about_self);

    pr.answer = res.answer;
    pr.trace  = res.trace;
    return pr;
}

void Interpreter::run_repl() {
    std::cout << BANNER << std::flush;
    std::string line;
    while (true) {
        std::cout << "16质点 > " << std::flush;
        if (!std::getline(std::cin, line)) {
            std::cout << "\n[王国] 再见。\n";
            break;
        }
        if (line.empty()) continue;

        // 元命令
        auto starts = [&](const char* prefix) {
            return line.rfind(prefix, 0) == 0;
        };

        auto& vt = VocabTable::instance();

        if (line == ":exit" || line == ":quit") {
            std::cout << "[王国] 已退出。\n"; break;
        }
        if (starts(":ptx ")) {
            auto kw = line.substr(5);
            auto e  = vt.lookup(kw);
            std::cout << (e ? e->ptx : "未知质点词汇: " + kw) << "\n";
            continue;
        }
        if (starts(":avx ")) {
            auto kw = line.substr(5);
            auto e  = vt.lookup(kw);
            std::cout << (e ? e->avx : "未知质点词汇: " + kw) << "\n";
            continue;
        }
        if (starts(":dml ")) {
            auto kw = line.substr(5);
            auto e  = vt.lookup(kw);
            std::cout << (e ? e->dml : "未知质点词汇: " + kw) << "\n";
            continue;
        }
        if (line == ":vocab") {
            std::cout << "已注册质点词汇：\n";
            for (auto& k : vt.keywords())
                std::cout << "  " << k << "\n";
            continue;
        }

        auto pr = process(line);

        if (verbose_) {
            std::cout << "\n─── 识别质点 ──────────────────────────────────\n  ";
            if (pr.tokens_found.empty()) {
                std::cout << "（无质点关键词）";
            } else {
                for (size_t i = 0; i < pr.tokens_found.size(); ++i) {
                    if (i) std::cout << ", ";
                    std::cout << pr.tokens_found[i];
                }
            }
            std::cout << "\n\n─── PTX 代码（sm_89）─────────────────────────\n";
            // 只打印前30行
            std::istringstream iss(pr.ptx_code);
            std::string l; int cnt = 0;
            while (std::getline(iss, l) && cnt < 30) {
                std::cout << "  " << l << "\n"; ++cnt;
            }
            std::cout << "\n─── 流水线追踪 ─────────────────────────────────\n";
            for (auto& t : pr.trace) std::cout << "  " << t << "\n";
            std::cout << "\n─── 最终输出 ───────────────────────────────────\n";
        }
        std::cout << (pr.answer.empty() ? "（王国暂无显示内容）" : pr.answer) << "\n\n";
    }
}

}  // namespace sephirot
