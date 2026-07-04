// ═══════════════════════════════════════════════════════════════════════════
// main.cpp  —  16质点神人双生协议  C++17 解释器入口
// ═══════════════════════════════════════════════════════════════════════════

#include "sephirot.h"
#include <iostream>
#include <string>
#include <cstring>

#ifdef _WIN32
#  define NOMINMAX
#  include <windows.h>
#  include <io.h>
#  include <fcntl.h>
#endif

using namespace sephirot;

static void set_utf8_console() {
#ifdef _WIN32
    SetConsoleOutputCP(65001);   // UTF-8 output
    SetConsoleCP(65001);         // UTF-8 input
    // 强制 stdout/stderr 进入二进制模式，避免 CRLF 及编码转换
    _setmode(_fileno(stdout), _O_BINARY);
    _setmode(_fileno(stderr), _O_BINARY);
#endif
}

static void print_usage(const char* argv0) {
    std::cout
        << "用法:\n"
        << "  " << argv0 << "                      # 启动 REPL\n"
        << "  " << argv0 << " --text <文本>          # 处理单条文本（PTX输出）\n"
        << "  " << argv0 << " --text <文本> --avx    # 输出 AVX-512 汇编\n"
        << "  " << argv0 << " --text <文本> --dml    # 输出 DirectML 算子\n"
        << "  " << argv0 << " --vocab                # 列出全部质点词汇\n";
}

int main(int argc, char* argv[]) {
    set_utf8_console();

    // ── 解析命令行 ───────────────────────────────────────────────────────
    std::string  text_arg;
    EmitTarget   target  = EmitTarget::PTX;
    bool         show_vocab = false;
    bool         run_repl   = true;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--text") == 0 && i + 1 < argc) {
            text_arg = argv[++i];
            run_repl = false;
        } else if (strcmp(argv[i], "--avx") == 0) {
            target = EmitTarget::AVX;
        } else if (strcmp(argv[i], "--dml") == 0) {
            target = EmitTarget::DML;
        } else if (strcmp(argv[i], "--vocab") == 0) {
            show_vocab = true;
            run_repl   = false;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            print_usage(argv[0]);
            return 0;
        }
    }

    // ── vocab 列表 ──────────────────────────────────────────────────────
    if (show_vocab) {
        std::cout << "已注册质点词汇：\n";
        for (auto& k : VocabTable::instance().keywords())
            std::cout << "  " << k << "\n";
        return 0;
    }

    // ── 单次处理 ────────────────────────────────────────────────────────
    if (!text_arg.empty()) {
        Interpreter interp({}, {}, /*verbose=*/false);
        auto pr = interp.process(text_arg, target);

        std::cout << "识别质点: ";
        if (pr.tokens_found.empty()) {
            std::cout << "（无）";
        } else {
            for (size_t i = 0; i < pr.tokens_found.size(); ++i) {
                if (i) std::cout << ", ";
                std::cout << pr.tokens_found[i];
            }
        }
        std::cout << "\n\n";

        std::string code_label;
        std::string code_body;
        switch (target) {
            case EmitTarget::PTX: code_label = "PTX  (sm_89)"; code_body = pr.ptx_code; break;
            case EmitTarget::AVX: code_label = "AVX-512";      code_body = pr.avx_code; break;
            case EmitTarget::DML: code_label = "DirectML";     code_body = pr.dml_code; break;
        }
        std::cout << "=== " << code_label << " 代码 ===\n" << code_body << "\n";
        std::cout << "\n=== 质点流水线追踪 ===\n";
        for (auto& t : pr.trace) std::cout << "  " << t << "\n";
        std::cout << "\n=== 输出 ===\n"
                  << (pr.answer.empty() ? "（王国暂无显示内容）" : pr.answer) << "\n";
        return 0;
    }

    // ── REPL 交互模式 ───────────────────────────────────────────────────
    if (run_repl) {
        Interpreter interp({}, {}, /*verbose=*/true);
        interp.run_repl();
    }

    return 0;
}
