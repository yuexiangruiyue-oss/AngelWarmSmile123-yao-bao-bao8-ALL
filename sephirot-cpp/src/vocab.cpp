// ═══════════════════════════════════════════════════════════════════════════
// vocab.cpp  —  16质点词汇表（中文关键词 → PTX / AVX-512 / DML 机器码）
//
// 每条机器码均标注来源：
//   PTX : NVIDIA PTX ISA Reference Manual 8.5  §6–§9
//   AVX : Intel® 64/IA-32 SDM Vol.2  Chapter 5 (AVX-512F/BF16/BITALG)
//   DML : Microsoft DirectML Operator Specifications (Windows AI)
// ═══════════════════════════════════════════════════════════════════════════

#include "sephirot.h"
#include <mutex>

namespace sephirot {

// ───────────────────────────────────────────────────────────────────────────
// VocabTable 单例实现
// ───────────────────────────────────────────────────────────────────────────

VocabTable& VocabTable::instance() {
    static VocabTable inst;
    return inst;
}

void VocabTable::register_entry(OpcodeEntry e) {
    kw_list_.push_back(e.keyword);
    table_.emplace(e.keyword, std::move(e));
}

const OpcodeEntry* VocabTable::lookup(const std::string& keyword) const {
    auto it = table_.find(keyword);
    return (it != table_.end()) ? &it->second : nullptr;
}

// ───────────────────────────────────────────────────────────────────────────
// 注册所有16质点词汇（+ 合成质点 理智/慈爱）
// ───────────────────────────────────────────────────────────────────────────

struct VocabRegistrar {
    VocabRegistrar() {
        auto& vt = VocabTable::instance();

        // ── 王冠 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "王冠",
            // PTX sm_89  §6.3 Memory Instructions: mad, st
            "// PTX sm_89  [PTX ISA §6.3]\n"
            ".entry keter_kernel(.param .u64 p_input, .param .u64 p_meta) {\n"
            "    .reg .u64 %rd<4>;\n"
            "    ld.param.u64  %rd0, [p_input];\n"
            "    mad.lo.u64    %rd1, %rd0, 1, 0;   // identity scatter\n"
            "    st.global.u64 [%rd1], %rd0;        // pass-through\n"
            "    ret;\n"
            "}",
            // AVX-512F  [SDM Vol.2 §VMOVDQU64]
            "VMOVDQU64 zmm0, [rsi]        ; load 512-bit input (64 bytes)\n"
            "VPXORQ    zmm1, zmm1, zmm1   ; zero mask\n"
            "VMOVDQU64 [rdi], zmm0        ; identity store to output",
            // DirectML
            "DML_OPERATOR_IDENTITY        ; tensor passthrough"
        });

        // ── 智慧 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "智慧",
            // PTX §6.7 Control Flow: setp, @pred call
            "// PTX sm_89  [PTX ISA §6.7]\n"
            ".entry chokmah_kernel(.param .u64 p_kb, .param .u64 p_token) {\n"
            "    .reg .u64  %rd<4>;\n"
            "    .reg .pred %p1;\n"
            "    ld.param.u64   %rd0, [p_token];\n"
            "    setp.ne.u64    %p1, %rd0, 0;\n"
            "    @%p1 call      logic_scan, (%rd0);\n"
            "    red.global.add.u64 [p_err_count], 1;\n"
            "    ret;\n"
            "}",
            // AVX-512F  [SDM Vol.2 §VPCMPQ, §VPCOMPRESSQ]
            "VPCMPQ    k1, zmm0, zmm_kb, 4    ; k1 = (tokens != kb_entries)\n"
            "VPCOMPRESSQ [rdi]{k1}, zmm0       ; compress mismatches → output",
            "DML_OPERATOR_GATHER_ND            ; gather knowledge-base mismatches"
        });

        // ── 理解 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "理解",
            // PTX §6.4 Floating-Point: fma, st
            "// PTX sm_89  [PTX ISA §6.4]\n"
            ".entry binah_kernel(.param .u64 p_corpus, .param .u64 p_query) {\n"
            "    .reg .f32 %f<8>;\n"
            "    ld.global.v4.f32 {%f0,%f1,%f2,%f3}, [p_corpus];\n"
            "    fma.rn.f32 %f4, %f0, %f_query, 0.0;  // dot-product stub\n"
            "    st.global.f32 [p_empathy_score], %f4;\n"
            "    ret;\n"
            "}",
            // AVX-512 BF16  [SDM Vol.2 §VDPBF16PS]
            "VDPBF16PS zmm_sim, zmm_query, zmm_corpus  ; BF16 dot-product\n"
            "VREDUCEPS zmm_norm, zmm_sim, 0              ; L2 normalize (AVX-512F §VREDUCEPS)",
            "DML_OPERATOR_BATCH_NORMALIZATION ; normalize empathy scores"
        });

        // ── 严厉 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "严厉",
            // PTX §6.3 Logic: and, setp, selp
            "// PTX sm_89  [PTX ISA §6.3]\n"
            ".entry geburah_kernel(.param .u64 p_input, .param .u64 p_ruleset) {\n"
            "    .reg .u64  %rd<4>;\n"
            "    .reg .pred %p_pass;\n"
            "    ld.param.u64 %rd0, [p_input];\n"
            "    ld.param.u64 %rd1, [p_ruleset];\n"
            "    and.b64      %rd2, %rd0, %rd1;\n"
            "    setp.eq.u64  %p_pass, %rd2, %rd1;\n"
            "    selp.u64     %rd3, %rd0, 0, %p_pass;\n"
            "    st.global.u64 [p_out], %rd3;\n"
            "    ret;\n"
            "}",
            // AVX-512F  [SDM Vol.2 §VPANDQ, §VPCMPQ]
            "VPANDQ    zmm_out, zmm_input, zmm_rules    ; AND ruleset\n"
            "VPCMPQ    k_pass, zmm_out, zmm_rules, 0    ; k_pass = eq mask\n"
            "VMOVDQU64Q zmm_result{k_pass}, zmm_input   ; conditional merge",
            "DML_OPERATOR_ELEMENT_WISE_LOGICAL_AND"
        });

        // ── 慈悲 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "慈悲",
            // PTX §6.4 FMA
            "// PTX sm_89  [PTX ISA §6.4]\n"
            ".entry chesed_kernel(.param .u64 p_logic, .param .u64 p_empathy) {\n"
            "    .reg .f32 %f<4>;\n"
            "    ld.global.f32 %f0, [p_empathy];\n"
            "    ld.global.f32 %f1, [p_logic];\n"
            "    fma.rn.f32    %f2, %f1, 0.7, %f0;   // blend: 0.7*logic + empathy\n"
            "    st.global.f32 [p_blend], %f2;\n"
            "    ret;\n"
            "}",
            // AVX-512F  [SDM Vol.2 §VBROADCASTSS, §VFMADD231PS]
            "VBROADCASTSS zmm_w, dword ptr [rip+weight_07]  ; broadcast 0.7\n"
            "VFMADD231PS  zmm_logic, zmm_empathy, zmm_w     ; zmm_logic = 0.7*logic+empathy",
            "DML_OPERATOR_ELEMENT_WISE_ADD ; blend logic+emotion vectors"
        });

        // ── 美丽 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "美丽",
            // PTX §6.4 FMA + abs
            "// PTX sm_89  [PTX ISA §6.4]\n"
            ".entry tiferet_kernel(.param .u64 p_logic, .param .u64 p_emo) {\n"
            "    .reg .f32 %f_l, %f_e, %f_opt, %f_bal;\n"
            "    ld.global.f32 %f_l, [p_logic];\n"
            "    ld.global.f32 %f_e, [p_emo];\n"
            "    fma.rn.f32    %f_opt, %f_l, %f_e, 0.0;\n"
            "    abs.f32       %f_bal, %f_opt;           // balance score\n"
            "    st.global.f32 [p_result], %f_bal;\n"
            "    ret;\n"
            "}",
            // AVX-512F  [SDM Vol.2 §VMULPS, §VABSPS, §VHADDPS]
            "VMULPS    zmm_opt, zmm_logic, zmm_emo    ; element-wise product\n"
            "VABSPS    zmm_bal, zmm_opt               ; abs balance score\n"
            "VHADDPS   ymm_sum, ymm_bal, ymm_bal      ; horizontal sum (AVX §VHADDPS)",
            "DML_OPERATOR_ELEMENT_WISE_MULTIPLY"
        });

        // ── 胜利 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "胜利",
            // PTX §6.7 setp + @!pred bra
            "// PTX sm_89  [PTX ISA §6.7]\n"
            ".entry netzach_kernel(.param .u64 p_result) {\n"
            "    .reg .f32  %f_sent;\n"
            "    .reg .pred %p_pos;\n"
            "retry_label:\n"
            "    ld.global.f32 %f_sent, [p_result];\n"
            "    setp.gt.f32   %p_pos, %f_sent, 0.5;  // positive threshold\n"
            "    @!%p_pos bra  retry_label;             // loop back if ≤ 0.5\n"
            "    ret;\n"
            "}",
            // AVX-512F  [SDM Vol.2 §VCMPPS, §KTESTW]
            "VCMPPS    k_pos, zmm_sent, zmm_thresh, 14  ; k_pos = (sent > 0.5)\n"
            "KTESTW    k_pos, k_pos                      ; test all-positive\n"
            "JZ        retry_label                        ; jump if any lane ≤ 0.5",
            "DML_OPERATOR_ACTIVATION_RELU ; zero-floor negative sentiment"
        });

        // ── 荣耀 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "荣耀",
            // PTX §6.3 cvt + and + setp
            "// PTX sm_89  [PTX ISA §6.3]\n"
            ".entry hod_kernel(.param .u64 p_result, .param .u64 p_phys_mask) {\n"
            "    .reg .u64  %rd_feasible, %rd_check;\n"
            "    .reg .f32  %f_result;\n"
            "    .reg .pred %p_ok;\n"
            "    ld.global.f32  %f_result,   [p_result];\n"
            "    cvt.u64.f32    %rd_feasible, %f_result;\n"
            "    ld.global.u64  %rd_mask,    [p_phys_mask];\n"
            "    and.b64        %rd_check,   %rd_feasible, %rd_mask;\n"
            "    setp.ne.u64    %p_ok,       %rd_check,   0;\n"
            "    ret;\n"
            "}",
            // AVX-512F  [SDM Vol.2 §VCVTPS2UDQ, §VPANDQ]
            "VCVTPS2UDQ zmm_u, zmm_result              ; float → uint32\n"
            "VPANDQ     zmm_ok, zmm_u, zmm_phys_mask   ; mask physical constraints",
            "DML_OPERATOR_ELEMENT_WISE_CLIP ; clip to [0, physical_max]"
        });

        // ── 基础 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "基础",
            // PTX §6.7 setp + @!pred call
            "// PTX sm_89  [PTX ISA §6.7]\n"
            ".entry yesod_kernel(.param .u64 p_meaning) {\n"
            "    .reg .f32  %f_meaning;\n"
            "    .reg .pred %p_ok;\n"
            "    ld.global.f32 %f_meaning, [p_meaning];\n"
            "    setp.gt.f32   %p_ok, %f_meaning, 0.0;\n"
            "    @!%p_ok call  strip_nihilism, ();\n"
            "    ret;\n"
            "}",
            // AVX-512F  [SDM Vol.2 §VCMPPS, §VPBLENDMD]
            "VCMPPS    k_mean, zmm_meaning, zmm_zero, 14    ; meaning > 0\n"
            "VPBLENDMD zmm_safe{k_mean}, zmm_result, zmm_0  ; zero nihilistic parts",
            "DML_OPERATOR_ACTIVATION_THRESHOLDED_RELU ; threshold on meaning score"
        });

        // ── 自我 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "自我",
            // PTX §6.3 ld v2
            "// PTX sm_89  [PTX ISA §6.3]\n"
            ".entry nefesh_kernel(.param .u64 p_profile) {\n"
            "    .reg .u64 %rd_name, %rd_ctx;\n"
            "    ld.global.v2.u64 {%rd_name, %rd_ctx}, [p_profile];\n"
            "    st.global.v2.u64 [p_ego], {%rd_name, %rd_ctx};\n"
            "    ret;\n"
            "}",
            // AVX-512F  [SDM Vol.2 §VMOVDQU64]
            "VMOVDQU64 zmm_ego, [rsi + user_profile_offset]  ; load physical self",
            "DML_OPERATOR_GATHER ; gather user profile features"
        });

        // ── 超我 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "超我",
            "// PTX sm_89  [PTX ISA §6.3]\n"
            ".entry neshamah_kernel(.param .u64 p_dream) {\n"
            "    .reg .u64 %rd_dream, %rd_aspire;\n"
            "    ld.global.v2.u64 {%rd_dream, %rd_aspire}, [p_dream];\n"
            "    fma.rn.f32 %f_ideal, %f_dream, 1.0, 0.0;\n"
            "    ret;\n"
            "}",
            "VMOVDQU64 zmm_ideal, [rsi + dream_profile_offset] ; load dream self",
            "DML_OPERATOR_GATHER ; gather dream profile features"
        });

        // ── 真我 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "真我",
            // PTX §6.4 fma + add
            "// PTX sm_89  [PTX ISA §6.4]\n"
            ".entry ruach_kernel(.param .u64 p_ego, .param .u64 p_ideal, .param .u64 p_ai) {\n"
            "    .reg .f32 %f_ego, %f_ideal, %f_ai, %f_true;\n"
            "    ld.global.f32 %f_ego,   [p_ego];\n"
            "    ld.global.f32 %f_ideal, [p_ideal];\n"
            "    ld.global.f32 %f_ai,    [p_ai];\n"
            "    fma.rn.f32    %f_true, %f_ego, 0.5, %f_ideal;  // 50/50 ego+ideal\n"
            "    add.f32       %f_true, %f_true, %f_ai;\n"
            "    st.global.f32 [p_true_self], %f_true;\n"
            "    ret;\n"
            "}",
            // AVX-512F  [SDM Vol.2 §VBROADCASTSS, §VFMADD231PS, §VADDPS]
            "VBROADCASTSS zmm_half, dword ptr [rip + weight_05]  ; 0.5\n"
            "VFMADD231PS  zmm_true, zmm_ego,   zmm_half          ; ego*0.5\n"
            "VADDPS       zmm_true, zmm_true,  zmm_ideal\n"
            "VADDPS       zmm_true, zmm_true,  zmm_ai_ans",
            "DML_OPERATOR_GEMM ; matrix blend ego+ideal+ai"
        });

        // ── 逻辑 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "逻辑",
            // PTX §6.3 mul + popc
            "// PTX sm_89  [PTX ISA §6.3  §9.7 POPC]\n"
            ".entry daat_logic_kernel(.param .u64 p_lvec, .param .u64 p_evec) {\n"
            "    .reg .u64 %rd_l, %rd_e, %rd_le, %rd_coh;\n"
            "    ld.global.u64 %rd_l, [p_lvec];\n"
            "    ld.global.u64 %rd_e, [p_evec];\n"
            "    mul.lo.u64    %rd_le,  %rd_l,  %rd_e;\n"
            "    popc.b64      %rd_coh, %rd_le;   // coherence = popcount\n"
            "    st.global.u64 [p_coherence], %rd_coh;\n"
            "    ret;\n"
            "}",
            // AVX-512 BITALG  [SDM Vol.2 §VPOPCNTQ]
            "VPMULUDQ  zmm_le,  zmm_logic, zmm_empathy  ; 64-bit product\n"
            "VPOPCNTQ  zmm_coh, zmm_le                   ; population count (AVX-512 BITALG)",
            "DML_OPERATOR_ELEMENT_WISE_MULTIPLY"
        });

        // ── 共情 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "共情",
            // PTX §6.4 sqrt.approx
            "// PTX sm_89  [PTX ISA §6.4]\n"
            ".entry daat_empathy_kernel(.param .u64 p_human_db) {\n"
            "    .reg .f32 %f_pain, %f_depth, %f_emo;\n"
            "    ld.global.f32    %f_pain,  [p_human_db];\n"
            "    sqrt.approx.f32  %f_depth, %f_pain;       // depth of feeling\n"
            "    mul.f32          %f_emo,   %f_depth, %f_resonance;\n"
            "    st.global.f32    [p_emo_vec], %f_emo;\n"
            "    ret;\n"
            "}",
            // AVX-512F  [SDM Vol.2 §VSQRTPS, §VMULPS]
            "VSQRTPS zmm_depth, zmm_pain               ; sqrt of pain vector\n"
            "VMULPS  zmm_emo,   zmm_depth, zmm_resonance ; weight by resonance",
            "DML_OPERATOR_ACTIVATION_SOFTSIGN ; smooth emotion curve"
        });

        // ── 幸福 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "幸福",
            // PTX §6.4 fma + tanh.approx
            "// PTX sm_89  [PTX ISA §6.4]\n"
            ".entry osher_kernel(.param .u64 p_logic_out, .param .u64 p_emo_out) {\n"
            "    .reg .f32 %f_lo, %f_eo, %f_happy, %f_gentle;\n"
            "    ld.global.f32    %f_lo,     [p_logic_out];\n"
            "    ld.global.f32    %f_eo,     [p_emo_out];\n"
            "    fma.rn.f32       %f_happy,  %f_lo, %f_eo, 0.0;\n"
            "    tanh.approx.f32  %f_gentle, %f_happy;  // squash to (-1,1)\n"
            "    st.global.f32    [p_output], %f_gentle;\n"
            "    ret;\n"
            "}",
            // AVX-512F + Intel SVML  [SDM Vol.2 §VMULPS; SVML §_mm512_tanh_ps]
            "VMULPS zmm_h, zmm_logic_out, zmm_emo_out\n"
            "; tanh via Intel SVML (short vector math library)\n"
            "; VCALL  _mm512_tanh_ps   ; (SVML intrinsic, links with -lsvml)",
            "DML_OPERATOR_ACTIVATION_TANH"
        });

        // ── 王国 ──────────────────────────────────────────────────────────
        vt.register_entry({
            "王国",
            // PTX §6.3 ld.global.u8 + atom.add
            "// PTX sm_89  [PTX ISA §6.3  §6.11 Atomic]\n"
            ".entry malkuth_kernel(.param .u64 p_text, .param .u64 p_display) {\n"
            "    .reg .u64 %rd_idx, %rd_char;\n"
            "    ld.global.u8   %rd_char, [p_text + %rd_idx];\n"
            "    st.global.u8   [p_display + %rd_idx], %rd_char;\n"
            "    atom.global.add.u64 [p_cursor], 1;\n"
            "    ret;\n"
            "}",
            // AVX-512BW  [SDM Vol.2 §VMOVDQU8]
            "VMOVDQU8  zmm_chars, [rsi + text_offset]     ; 64-char block load\n"
            "VMOVDQU8  [rdi + display_offset], zmm_chars  ; write to display buffer",
            "DML_OPERATOR_IDENTITY ; passthrough to render pipeline"
        });

        // ── 理智（合成）智慧×严厉 ──────────────────────────────────────────
        vt.register_entry({
            "理智",
            "// PTX sm_89  [PTX ISA §6.3]\n"
            ".entry daat_rational_kernel(.param .u64 p_wisdom, .param .u64 p_sev) {\n"
            "    .reg .u64 %rd_w, %rd_s, %rd_r, %rd_gaps;\n"
            "    ld.param.u64 %rd_w, [p_wisdom];\n"
            "    ld.param.u64 %rd_s, [p_sev];\n"
            "    and.b64      %rd_r,    %rd_w,    %rd_s;\n"
            "    popc.b64     %rd_gaps, %rd_r;     // gap count\n"
            "    st.global.u64 [p_rational], %rd_r;\n"
            "    ret;\n"
            "}",
            "VPANDQ   zmm_rational, zmm_wisdom, zmm_severity\n"
            "VPOPCNTQ zmm_gaps,     zmm_rational",
            "DML_OPERATOR_ELEMENT_WISE_LOGICAL_AND"
        });

        // ── 慈爱（合成）理解×慈悲 ──────────────────────────────────────────
        vt.register_entry({
            "慈爱",
            "// PTX sm_89  [PTX ISA §6.4]\n"
            ".entry daat_love_kernel(.param .u64 p_under, .param .u64 p_comp) {\n"
            "    .reg .f32 %f_u, %f_c, %f_love;\n"
            "    ld.param.f32 %f_u, [p_under];\n"
            "    ld.param.f32 %f_c, [p_comp];\n"
            "    fma.rn.f32   %f_love, %f_u, %f_c, 0.0;\n"
            "    st.global.f32 [p_love], %f_love;\n"
            "    ret;\n"
            "}",
            "VMULPS zmm_love, zmm_understanding, zmm_compassion",
            "DML_OPERATOR_ELEMENT_WISE_MULTIPLY"
        });
    }
} g_vocab_init;   // 全局静态对象，main() 前自动注册

}  // namespace sephirot
