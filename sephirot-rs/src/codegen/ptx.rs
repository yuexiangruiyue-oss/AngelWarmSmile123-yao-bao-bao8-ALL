/// PTX sm_89 codegen backend - NVIDIA Ada Lovelace / RTX 4050
/// Outputs ptxas-assemblable ASCII-only PTX (CUDA 13.2 compatible)
use crate::codegen::CodeEmitter;
use crate::error::Result;
use crate::ir::{IrProgram, IrPipeline, IrStage, IrValue};
use crate::lang::Sephirah;

pub struct PtxEmitter;

impl CodeEmitter for PtxEmitter {
    fn target_name(&self) -> &str { "PTX sm_89 (NVIDIA Ada Lovelace)" }
    fn extension(&self) -> &str { "ptx" }

    fn emit(&self, ir: &IrProgram) -> Result<String> {
        let mut out = String::new();

        // Header (ASCII only for ptxas compatibility)
        out.push_str("//\n");
        out.push_str("// SephirotLang v1.0 - 16 Sephiroth PTX Kernel\n");
        out.push_str("// Target: sm_89 (RTX 4050 Ada Lovelace)\n");
        out.push_str("// PTX ISA 8.5 / CUDA 13.2\n");
        out.push_str("//\n\n");

        out.push_str(".version 8.5\n");
        out.push_str(".target sm_89\n");
        out.push_str(".address_size 64\n\n");

        // Collect unique params from all pipelines
        let mut all_params: Vec<String> = Vec::new();
        for pipeline in &ir.pipelines {
            for stage in &pipeline.stages {
                for arg in &stage.args {
                    if !all_params.contains(arg) {
                        all_params.push(arg.clone());
                    }
                }
            }
        }

        // Emit kernels for each pipeline
        for pipeline in &ir.pipelines {
            out.push_str(&emit_pipeline_ptx(pipeline, &all_params));
        }

        Ok(out)
    }
}

/// Convert f64 to PTX hex float literal (0fXXXXXXXX)
fn f32_to_ptx_hex(val: f64) -> String {
    let bits = (val as f32).to_bits();
    format!("0f{:08X}", bits)
}

fn emit_pipeline_ptx(pipeline: &IrPipeline, all_params: &[String]) -> String {
    let mut s = String::new();

    s.push_str("// ============================================================\n");
    s.push_str(&format!("// Pipeline: {}\n", pipeline.name));
    s.push_str("// ============================================================\n\n");

    // Kernel signature - up to 8 params for simplicity
    let param_count = std::cmp::min(all_params.len(), 8);
    let param_count = std::cmp::max(param_count, pipeline.stages.len() + 1);

    s.push_str(&format!(".visible .entry sephirot_kernel(\n"));
    for i in 0..std::cmp::min(param_count, 9) {
        if i < all_params.len() {
            s.push_str(&format!("    .param .u64 p{},\n", i));
        } else {
            s.push_str(&format!("    .param .u64 p{},\n", i));
        }
    }
    s.push_str("    .param .u64 p_output\n");
    s.push_str(") {\n");
    s.push_str("    .reg .pred  %p<4>;\n");
    s.push_str("    .reg .f32   %f<64>;\n");
    s.push_str("    .reg .u32   %r<4>;\n");
    s.push_str("    .reg .u64   %rd<12>;\n\n");

    // Load all param pointers into general-purpose registers
    for i in 0..std::cmp::min(param_count, 9) {
        s.push_str(&format!("    ld.param.u64 %rd{}, [p{}];\n", i, i));
    }
    s.push_str("    ld.param.u64 %rd10, [p_output];\n\n");

    // Thread index
    s.push_str("    mov.u32 %r0, %tid.x;\n");
    s.push_str("    cvt.u64.u32 %rd11, %r0;\n\n");

    // Generate PTX for each stage
    let mut last_reg = 0u32;
    for stage in &pipeline.stages {
        let (code, final_reg) = emit_stage_ptx(stage, &all_params);
        s.push_str(&code);
        last_reg = final_reg;
    }

    // Write output
    s.push_str(&format!(
        "    st.global.f32 [%rd10], %f{};\n\n",
        last_reg
    ));

    s.push_str("    ret;\n");
    s.push_str("}\n\n");

    s
}

fn get_param_reg(arg: &str, all_params: &[String]) -> usize {
    all_params.iter().position(|p| p == arg).unwrap_or(0)
}

fn emit_stage_ptx(stage: &IrStage, all_params: &[String]) -> (String, u32) {
    let mut s = String::new();
    let ri = (stage.index * 4) as u32;

    // Header comment
    s.push_str(&format!(
        "    // [{:>2}] {} ({:?}) - {}\n",
        stage.index, stage.opcode.keyword(), stage.opcode.side(), stage.opcode.description()
    ));
    s.push_str(&format!(
        "    // PTX: {}\n", stage.opcode.ptx_instruction()
    ));

    let out_reg = match stage.opcode {
        Sephirah::王冠 => {
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let pr = get_param_reg(arg0, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr
            ));
            ri
        }
        Sephirah::智慧 => {
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let arg1 = stage.args.get(1).map(|s| s.as_str()).unwrap_or("");
            let pr0 = get_param_reg(arg0, all_params);
            let pr1 = get_param_reg(arg1, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr0
            ));
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri + 1, pr1
            ));
            s.push_str(&format!(
                "    mul.f32 %f{}, %f{}, %f{};\n", ri + 2, ri, ri + 1
            ));
            ri + 2
        }
        Sephirah::严厉 => {
            let threshold = get_float_param(&stage.params, "threshold", 0.8);
            let hex = f32_to_ptx_hex(threshold);
            let zero_hex = f32_to_ptx_hex(0.0);
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let pr = get_param_reg(arg0, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr
            ));
            s.push_str(&format!(
                "    mov.f32 %f{}, {};\n", ri + 1, hex
            ));
            s.push_str(&format!(
                "    setp.lt.f32 %p0, %f{}, %f{};\n", ri, ri + 1
            ));
            s.push_str(&format!(
                "    selp.f32 %f{}, {}, %f{}, %p0;\n", ri + 2, zero_hex, ri
            ));
            ri + 2
        }
        Sephirah::理解 => {
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let arg1 = stage.args.get(1).map(|s| s.as_str()).unwrap_or("");
            let pr0 = get_param_reg(arg0, all_params);
            let pr1 = get_param_reg(arg1, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr0
            ));
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri + 1, pr1
            ));
            s.push_str(&format!(
                "    add.f32 %f{}, %f{}, %f{};\n", ri + 2, ri, ri + 1
            ));
            ri + 2
        }
        Sephirah::慈悲 => {
            let weight = get_float_param(&stage.params, "weight", 0.7);
            let hex_w = f32_to_ptx_hex(weight);
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let pr0 = get_param_reg(arg0, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr0
            ));
            s.push_str(&format!(
                "    mov.f32 %f{}, {};\n", ri + 1, hex_w
            ));
            s.push_str(&format!(
                "    fma.rn.f32 %f{}, %f{}, %f{}, %f0;\n", ri + 2, ri, ri + 1
            ));
            ri + 2
        }
        Sephirah::美丽 => {
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let arg1 = stage.args.get(1).map(|s| s.as_str()).unwrap_or("");
            let pr0 = get_param_reg(arg0, all_params);
            let pr1 = get_param_reg(arg1, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr0
            ));
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri + 1, pr1
            ));
            s.push_str(&format!(
                "    mul.f32 %f{}, %f{}, %f{};\n", ri + 2, ri, ri + 1
            ));
            ri + 2
        }
        Sephirah::胜利 => {
            let zero_hex = f32_to_ptx_hex(0.0);
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let pr = get_param_reg(arg0, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr
            ));
            s.push_str(&format!(
                "    setp.ge.f32 %p1, %f{}, {};\n", ri, zero_hex
            ));
            s.push_str(&format!(
                "    selp.f32 %f{}, %f{}, {}, %p1;\n", ri + 1, ri, zero_hex
            ));
            ri + 1
        }
        Sephirah::荣耀 => {
            let half_hex = f32_to_ptx_hex(0.5);
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let pr = get_param_reg(arg0, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr
            ));
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri + 1, pr
            ));
            s.push_str(&format!(
                "    mul.f32 %f{}, %f{}, {};\n", ri + 2, ri, half_hex
            ));
            s.push_str(&format!(
                "    add.f32 %f{}, %f{}, %f{};\n", ri + 3, ri + 2, ri + 1
            ));
            ri + 3
        }
        Sephirah::基础 => {
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let pr = get_param_reg(arg0, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr
            ));
            s.push_str(&format!(
                "    atom.global.add.f32 %f{}, [%rd10], %f{};\n", ri + 1, ri
            ));
            ri + 1
        }
        Sephirah::超我 => {
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let pr = get_param_reg(arg0, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr
            ));
            s.push_str(&format!(
                "    rcp.rn.f32 %f{}, %f{};\n", ri + 1, ri
            ));
            s.push_str(&format!(
                "    mul.f32 %f{}, %f{}, %f{};\n", ri + 2, ri, ri + 1
            ));
            ri + 2
        }
        Sephirah::自我 => {
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let arg1 = stage.args.get(1).map(|s| s.as_str()).unwrap_or("");
            let pr0 = get_param_reg(arg0, all_params);
            let pr1 = get_param_reg(arg1, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr0
            ));
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri + 1, pr1
            ));
            s.push_str(&format!(
                "    mul.f32 %f{}, %f{}, %f{};\n", ri + 2, ri, ri + 1
            ));
            ri + 2
        }
        Sephirah::真我 => {
            let eps_hex = f32_to_ptx_hex(1e-8);
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let pr = get_param_reg(arg0, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr
            ));
            s.push_str(&format!(
                "    add.f32 %f{}, %f{}, {};\n", ri + 1, ri, eps_hex
            ));
            s.push_str(&format!(
                "    rcp.rn.f32 %f{}, %f{};\n", ri + 2, ri + 1
            ));
            s.push_str(&format!(
                "    mul.f32 %f{}, %f{}, %f{};\n", ri + 3, ri, ri + 2
            ));
            ri + 3
        }
        Sephirah::逻辑 => {
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let arg1 = stage.args.get(1).map(|s| s.as_str()).unwrap_or("");
            let pr0 = get_param_reg(arg0, all_params);
            let pr1 = get_param_reg(arg1, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr0
            ));
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri + 1, pr1
            ));
            s.push_str(&format!(
                "    mul.f32 %f{}, %f{}, %f{};\n", ri + 2, ri, ri + 1
            ));
            s.push_str(&format!(
                "    add.f32 %f{}, %f{}, %f{};\n", ri + 3, ri + 2, ri
            ));
            ri + 3
        }
        Sephirah::共情 => {
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let pr = get_param_reg(arg0, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr
            ));
            s.push_str(&format!(
                "    ex2.approx.f32 %f{}, %f{};\n", ri + 1, ri
            ));
            s.push_str(&format!(
                "    mul.f32 %f{}, %f{}, %f{};\n", ri + 2, ri + 1, ri + 1
            ));
            s.push_str(&format!(
                "    rcp.rn.f32 %f{}, %f{};\n", ri + 3, ri + 2
            ));
            s.push_str(&format!(
                "    mul.f32 %f{}, %f{}, %f{};\n", ri + 4, ri + 1, ri + 3
            ));
            ri + 4
        }
        Sephirah::幸福 => {
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let arg1 = stage.args.get(1).map(|s| s.as_str()).unwrap_or("");
            let pr0 = get_param_reg(arg0, all_params);
            let pr1 = get_param_reg(arg1, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr0
            ));
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri + 1, pr1
            ));
            s.push_str(&format!(
                "    sub.f32 %f{}, %f0, %f{};\n", ri + 2, ri
            ));
            s.push_str(&format!(
                "    mul.f32 %f{}, %f{}, %f{};\n", ri + 3, ri + 2, ri + 2
            ));
            ri + 3
        }
        Sephirah::王国 => {
            let arg0 = stage.args.first().map(|s| s.as_str()).unwrap_or("");
            let pr = get_param_reg(arg0, all_params);
            s.push_str(&format!(
                "    ld.global.nc.f32 %f{}, [%rd{}];\n", ri, pr
            ));
            ri
        }
    };

    s.push_str("\n");
    (s, out_reg)
}

fn get_float_param(params: &[(String, IrValue)], key: &str, default: f64) -> f64 {
    for (k, v) in params {
        if k == key {
            if let IrValue::Float(f) = v { return *f; }
            if let IrValue::Integer(n) = v { return *n as f64; }
        }
    }
    default
}
