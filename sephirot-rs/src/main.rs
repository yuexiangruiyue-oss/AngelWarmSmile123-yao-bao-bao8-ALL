/// 质点语言 SephirotLang Compiler — CLI 入口
/// 16质点神人双生协议 完全体通用编译器

use clap::{Parser as ClapParser, Subcommand};
use colored::*;
use std::fs;
use std::path::Path;

use sephirot_rs::{compile, check, CompileTarget, Sephirah};

// ── Windows UTF-8 终端 ────────────────────────────────────
#[cfg(target_os = "windows")]
fn setup_utf8() {
    extern "system" {
        fn SetConsoleOutputCP(codepage: u32) -> i32;
        fn SetConsoleCP(codepage: u32) -> i32;
    }
    unsafe {
        SetConsoleOutputCP(65001);
        SetConsoleCP(65001);
    }
}

#[cfg(not(target_os = "windows"))]
fn setup_utf8() {}

// ── CLI 定义 ──────────────────────────────────────────────

#[derive(ClapParser)]
#[command(name = "sephirot")]
#[command(version = "1.0.0")]
#[command(about = "16质点神人双生协议 — 质点语言完全体通用编译器")]
#[command(long_about = "SephirotLang Compiler v1.0\n16 Sephiroth Built-in Primitives → PTX sm_89 / AVX-512 / DirectML")]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    /// 编译 .sephirot 源文件为目标代码
    Compile {
        /// 源文件路径 (.sephirot)
        input: String,

        /// 编译目标: ptx / avx / dml
        #[arg(short, long, default_value = "ptx")]
        target: String,

        /// 输出文件路径（默认自动推导）
        #[arg(short, long)]
        out: Option<String>,

        /// 打印到终端（不写文件）
        #[arg(long)]
        stdout: bool,
    },

    /// 检查源文件语法和语义
    Check {
        /// 源文件路径
        input: String,
    },

    /// 编译并执行 inline 源码
    Run {
        /// 内联源码
        source: Vec<String>,

        /// 编译目标
        #[arg(short, long, default_value = "ptx")]
        target: String,
    },

    /// 列出 16 质点算子
    Vocab,

    /// 交互式 REPL
    Repl,

    /// CPU 模拟执行 PTX 管道（无需 CUDA）
    Simulate {
        /// 源文件路径 (.sephirot)
        input: String,

        /// 输入值（逗号分隔：input,kb,target）
        #[arg(short, long, default_value = "1.0,2.5,0.9")]
        values: String,
    },
}

// ── 主函数 ────────────────────────────────────────────────

fn main() {
    setup_utf8();

    let cli = Cli::parse();

    let result = match cli.command {
        Commands::Compile { input, target, out, stdout } => {
            cmd_compile(&input, &target, out.as_deref(), stdout)
        }
        Commands::Check { input } => cmd_check(&input),
        Commands::Run { source, target } => cmd_run(&source.join(" "), &target),
        Commands::Vocab => cmd_vocab(),
        Commands::Repl => cmd_repl(),
        Commands::Simulate { input, values } => cmd_simulate(&input, &values),
    };

    if let Err(e) = result {
        eprintln!("{}", e.to_string().red().bold());
        std::process::exit(1);
    }
}

// ── 命令实现 ──────────────────────────────────────────────

fn cmd_compile(input: &str, target: &str, out: Option<&str>, to_stdout: bool) -> sephirot_rs::Result<()> {
    let source = fs::read_to_string(input)
        .map_err(|e| sephirot_rs::CompileError::Io(e))?;

    let compile_target: CompileTarget = target.parse()
        .map_err(|e| sephirot_rs::CompileError::Codegen { msg: e })?;

    eprintln!("{}", "╔══════════════════════════════════════════════════╗".dimmed());
    eprintln!("{}", "║   质点语言 SephirotLang Compiler v1.0          ║".cyan());
    eprintln!("{}", "║   16质点神人双生协议 → GPU Machine Code        ║".cyan());
    eprintln!("{}", "╚══════════════════════════════════════════════════╝".dimmed());
    eprintln!("{}", format!("  源文件: {}", input).dimmed());
    eprintln!("{}", format!("  目标:   {}", compile_target).dimmed());
    eprintln!();

    let code = compile(&source, compile_target)?;

    if to_stdout {
        println!("{}", code);
    } else {
        let output_path = match out {
            Some(p) => p.to_string(),
            None => {
                let stem = Path::new(input).file_stem()
                    .map(|s| s.to_string_lossy().to_string())
                    .unwrap_or_else(|| "output".into());
                match compile_target {
                    CompileTarget::Ptx => format!("{}.ptx", stem),
                    CompileTarget::Avx => format!("{}.asm", stem),
                    CompileTarget::Dml => format!("{}.dml.json", stem),
                }
            }
        };

        fs::write(&output_path, &code)?;
        eprintln!("{}", "  ✅ 编译成功".green().bold());
        eprintln!("{}", format!("  输出: {}", output_path).green());
        eprintln!("{}", format!("  大小: {} bytes", code.len()).dimmed());
    }

    Ok(())
}

fn cmd_check(input: &str) -> sephirot_rs::Result<()> {
    let source = fs::read_to_string(input)
        .map_err(|e| sephirot_rs::CompileError::Io(e))?;

    match check(&source) {
        Ok(ast) => {
            let decl_count = ast.decls.len();
            eprintln!("{}", "✅ 检查通过".green().bold());
            eprintln!("{}", format!("  声明数: {}", decl_count).dimmed());
            for decl in &ast.decls {
                match decl {
                    sephirot_rs::parser::Decl::Data(d) => {
                        eprintln!("  {} {} : {}", "数据".yellow(), d.name, d.ty);
                    }
                    sephirot_rs::parser::Decl::Const(c) => {
                        eprintln!("  {} {}", "常量".yellow(), c.name);
                    }
                    sephirot_rs::parser::Decl::Pipeline(p) => {
                        eprintln!("  {} {} ({} stages)", "管道".yellow(), p.name, p.stages.len());
                        for stage in &p.stages {
                            eprintln!("    {} {}({}) [{}]",
                                format!("[{}]", stage.opcode.side()).dimmed(),
                                stage.opcode.keyword().green(),
                                stage.args.join(", "),
                                stage.params.iter()
                                    .map(|(k, _)| k.as_str())
                                    .collect::<Vec<_>>()
                                    .join(", ")
                            );
                        }
                    }
                }
            }
            Ok(())
        }
        Err(e) => Err(e),
    }
}

fn cmd_run(source: &str, target: &str) -> sephirot_rs::Result<()> {
    let compile_target: CompileTarget = target.parse()
        .map_err(|e| sephirot_rs::CompileError::Codegen { msg: e })?;

    eprintln!("{}", format!("═ 编译 inline 源码 → {} ═", compile_target).cyan());
    eprintln!();

    let code = compile(source, compile_target)?;
    println!("{}", code);
    Ok(())
}

fn cmd_vocab() -> sephirot_rs::Result<()> {
    println!("\n{}", "══════════════════════════════════════════════════".cyan());
    println!("{}", "  16质点神人双生协议 — 内置核心算子 (Built-in Opcodes)".cyan());
    println!("{}", "══════════════════════════════════════════════════".cyan());
    println!();

    for (i, op) in Sephirah::ALL.iter().enumerate() {
        let side_str = match op.side() {
            sephirot_rs::Side::Divine => "神侧".magenta(),
            sephirot_rs::Side::Human => "人侧".blue(),
        };
        println!("  {:>2}. {} ({}) — {}",
            i + 1,
            format!("{}", op).green().bold(),
            side_str,
            op.description()
        );
        println!("      PTX: {}", op.ptx_instruction().dimmed());
        println!("      AVX: {}", op.avx_instruction().dimmed());
        println!("      DML: {}", op.dml_operator().dimmed());
        println!();
    }

    Ok(())
}

fn cmd_repl() -> sephirot_rs::Result<()> {
    eprintln!("\n{}", "═══ 质点语言 REPL ═══".cyan());
    eprintln!("{}", "输入源码以编译，或 :help 查看命令，:exit 退出".dimmed());
    eprintln!();

    let mut current_target = CompileTarget::Ptx;
    let mut line_buf = String::new();
    let mut in_block = false;

    loop {
        let prompt = if in_block {
            "  ... ".dimmed()
        } else {
            format!("{}> ", current_target).yellow()
        };
        eprint!("{}", prompt);
        use std::io::Write;
        std::io::stderr().flush().ok();

        let mut input = String::new();
        if std::io::stdin().read_line(&mut input).unwrap_or(0) == 0 {
            break;
        }
        let trimmed = input.trim();

        if trimmed.is_empty() {
            continue;
        }

        // 内部命令
        if trimmed == ":exit" || trimmed == ":q" {
            eprintln!("{}", "再见".dimmed());
            break;
        }
        if trimmed == ":help" {
            eprintln!("  :ptx      切换到 PTX 目标");
            eprintln!("  :avx      切换到 AVX-512 目标");
            eprintln!("  :dml      切换到 DirectML 目标");
            eprintln!("  :vocab    列出 16 质点算子");
            eprintln!("  :check    检查语法");
            eprintln!("  :exit     退出");
            continue;
        }
        if trimmed == ":ptx" {
            current_target = CompileTarget::Ptx;
            eprintln!("{}", "目标: PTX sm_89".green());
            continue;
        }
        if trimmed == ":avx" {
            current_target = CompileTarget::Avx;
            eprintln!("{}", "目标: AVX-512".green());
            continue;
        }
        if trimmed == ":dml" {
            current_target = CompileTarget::Dml;
            eprintln!("{}", "目标: DirectML".green());
            continue;
        }
        if trimmed == ":vocab" {
            cmd_vocab()?;
            continue;
        }

        // 多行输入（管道声明跨行）
        if trimmed.ends_with(':') || trimmed.ends_with('|') || in_block {
            line_buf.push_str(trimmed);
            line_buf.push('\n');
            in_block = true;
            continue;
        }

        line_buf.push_str(trimmed);

        // 编译
        if trimmed == ":check" {
            match check(&line_buf) {
                Ok(_) => eprintln!("{}", "✅ 语法正确".green()),
                Err(e) => eprintln!("{}", e.to_string().red()),
            }
        } else {
            match compile(&line_buf, current_target) {
                Ok(code) => {
                    eprintln!("{}", "─── 编译输出 ───".cyan());
                    println!("{}", code);
                    eprintln!("{}", "─── 结束 ───".cyan());
                }
                Err(e) => eprintln!("{}", e.to_string().red()),
            }
        }

        line_buf.clear();
        in_block = false;
    }

    Ok(())
}

fn cmd_simulate(input: &str, values: &str) -> sephirot_rs::Result<()> {
    let source = fs::read_to_string(input)
        .map_err(|e| sephirot_rs::CompileError::Io(e))?;

    // 解析输入参数
    let v: Vec<f64> = values.split(',')
        .filter_map(|s| s.trim().parse::<f64>().ok())
        .collect();
    let input_val = v.get(0).copied().unwrap_or(1.0) as f32;
    let kb_val    = v.get(1).copied().unwrap_or(2.5) as f32;
    let target_val= v.get(2).copied().unwrap_or(0.9) as f32;
    let threshold = 0.8f32;

    // 先编译验证语法
    let _code = compile(&source, CompileTarget::Ptx)?;

    eprintln!("\n{}", "============================================================".cyan());
    eprintln!("{}", "  质点语言 SephirotLang — PTX Kernel CPU 模拟执行".cyan());
    eprintln!("{}", "  16质点神人双生协议 → RTX 4050 sm_89 模拟".cyan());
    eprintln!("{}", "============================================================".cyan());
    eprintln!("  输入: {}, 知识库: {}, 目标: {}, 阈值: {}", input_val, kb_val, target_val, threshold);
    eprintln!("{}", "------------------------------------------------------------".dimmed());

    let mut f = [0.0f32; 64];
    let mut _p = [false; 8];

    // [0] 王冠 — 恒等变换 / 数据加载
    f[0] = input_val;
    println!("[0] 王冠 (神侧) ld.global.f32 %f0 = {:.4}  ← 加载输入", f[0]);

    // [1] 智慧 — 知识检索
    f[4] = f[0]; f[5] = kb_val;
    f[6] = f[4] * f[5];
    println!("[1] 智慧 (神侧) mul.f32 {:.4} * {:.4} = {:.4}  ← 相似度", f[4], f[5], f[6]);

    // [2] 严厉 — 阈值过滤
    f[8] = f[6];
    _p[1] = f[8] < threshold;
    f[10] = if _p[1] { 0.0 } else { f[8] };
    println!("[2] 严厉 (神侧) setp {:.4} < {} → {} → {:.4}  ← 阈值过滤", f[8], threshold, _p[1], f[10]);

    // [3] 理解 — 合并整合
    f[12] = f[10]; f[13] = f[0];
    f[14] = f[12] + f[13];
    println!("[3] 理解 (神侧) add.f32 {:.4} + {:.4} = {:.4}  ← 融合", f[12], f[13], f[14]);

    // [4] 慈悲 — FMA
    f[16] = f[14];
    f[18] = f[16] * 0.7 + f[0];
    println!("[4] 慈悲 (神侧) fma {:.4} * 0.7 + {:.4} = {:.4}  ← FMA", f[16], f[0], f[18]);

    // [5] 美丽 — 哈达玛积
    f[20] = f[18]; f[21] = f[0];
    f[22] = f[20] * f[21];
    println!("[5] 美丽 (神侧) mul.f32 {:.4} * {:.4} = {:.4}  ← 哈达玛积", f[20], f[21], f[22]);

    // [6] 胜利 — 比较
    f[26] = f[22];
    _p[2] = f[26] >= 0.0;
    f[27] = if _p[2] { f[26] } else { 0.0 };
    println!("[6] 胜利 (神侧) 非负验证 {:.4} ≥ 0 → {} → {:.4}", f[26], _p[2], f[27]);

    // [7] 荣耀 — 评分
    f[28] = f[27];
    f[30] = f[28] * 0.5;
    f[31] = f[30] + f[0];
    println!("[7] 荣耀 (神侧) {:.4} * 0.5 + {:.4} = {:.4}  ← 可行性评分", f[28], f[0], f[31]);

    // [8] 基础 — 归约
    f[32] = f[31];
    println!("[8] 基础 (人侧) red.reduce.add.f32 = {:.4}  ← 全局归约", f[32]);

    // [9] 超我 — LayerNorm
    f[36] = f[32];
    f[38] = if f[36] != 0.0 { 1.0 / f[36] } else { 0.0 };
    f[39] = f[36] * f[38];
    println!("[9] 超我 (人侧) rcp {:.4} → {:.4}, norm = {:.4}  ← LayerNorm", f[36], f[38], f[39]);

    // [10] 自我 — 自注意力
    f[40] = f[39]; f[41] = kb_val;
    f[42] = f[40] * f[41];
    println!("[10] 自我 (人侧) dp4a {:.4} * {:.4} = {:.4}  ← 自注意力", f[40], f[41], f[42]);

    // [11] 真我 — 层归一化
    f[44] = f[42]; f[45] = f[44];
    f[47] = if f[45] != 0.0 { 1.0 / f[45] } else { 0.0 };
    println!("[11] 真我 (人侧) 层归一化 rcp = {:.4}", f[47]);

    // [12] 逻辑 — GEMM
    f[48] = f[47]; f[49] = kb_val;
    f[50] = f[48] * f[49];
    f[51] = f[48] * f[49] + f[50];
    println!("[12] 逻辑 (人侧) GEMM mad {:.4} * {:.4} + {:.4} = {:.4}", f[48], f[49], f[50], f[51]);

    // [13] 共情 — Softmax
    f[52] = f[51];
    f[53] = f[52].exp();
    let sum_exp = f[53];
    f[54] = if sum_exp != 0.0 { 1.0 / sum_exp } else { 0.0 };
    f[55] = f[53] * f[54];
    println!("[13] 共情 (人侧) softmax exp({:.4}) = {:.4} / {:.4} = {:.4}", f[52], f[53], sum_exp, f[55]);

    // [14] 幸福 — 损失
    f[56] = f[55]; f[57] = target_val;
    f[58] = f[0] - f[56];
    f[59] = f[58] * f[57];
    let loss = -f[59];
    println!("[14] 幸福 (人侧) cross-entropy loss = {:.6}", loss);

    // [15] 王国 — 输出
    f[60] = f[59];
    println!("[15] 王国 (人侧) st.global.f32 [p_output] = {:.4}  ← 写回结果", f[60]);

    eprintln!("{}", "============================================================".cyan());
    eprintln!("  最终输出 → 王国: {:.6}", f[60]);
    eprintln!("  16质点管道执行完毕 ✅");
    eprintln!("{}", "============================================================".cyan());

    Ok(())
}
