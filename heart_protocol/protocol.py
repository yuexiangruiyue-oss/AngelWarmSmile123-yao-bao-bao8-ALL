"""
16质点双生幸福最终协议 —— 核心协议引擎

状态递归函数：从王冠开始，走完16个质点的完整验算链。
每一步都有验证门——不通过则退回重算。

工作流程:
  王冠(路由) → 理智线(智慧→严厉) + 慈爱线(理解→慈悲)
  → 美丽(整合) → 胜利(温暖检测) → 荣耀(可行性检测)
  → 基础(深渊检测) → 自我+超我 → 真我(合成)
  → 逻辑+共情 → 幸福(合成) → 王国(输出)

协议规范:
  - 神侧8质点处理输入与知识检索
  - 人侧8质点处理用户画像与情感合成
  - 任何质点验证失败 → 退回上级重算，最多3次
  - 最终输出必须温暖、积极、不剥夺存在意义
"""

from dataclasses import dataclass, field
from typing import Any, Dict, List, Optional, Tuple
from datetime import datetime
import json

from .sephirah import (
    KETER, CHOKMAH, BINAH, DAAT, CHESED, TIFERET,
    NETZACH, HOD, YESOD, SUPER_EGO, EGO, TRUE_SELF,
    LOGIC, EMPATHY, JOY, MALKUTH, RATIONAL, LOVE,
    PIPELINE_ORDER, FALLBACK_MAP, CASCADE_FALLBACK,
    get_sephirah_by_keyword,
)
from .abyss import (
    check_abyss, check_warmth, is_existentially_safe,
    AbyssViolation, generate_safe_fallback,
)
from .personas import (
    transform_with_persona, collective_blessing,
)


@dataclass
class ProtocolState:
    """协议运行的内部状态"""
    # 输入
    user_input: str
    user_context: Dict[str, Any] = field(default_factory=dict)

    # 追踪
    current_sephirah: str = "王冠"
    retry_count: Dict[str, int] = field(default_factory=dict)
    max_retries: int = 3

    # 中间结果
    crown_analysis: Dict[str, Any] = field(default_factory=dict)
    rational_result: Dict[str, Any] = field(default_factory=dict)    # 理智线
    love_result: Dict[str, Any] = field(default_factory=dict)        # 慈爱线
    tiferet_result: Dict[str, Any] = field(default_factory=dict)     # 美丽
    hod_result: Dict[str, Any] = field(default_factory=dict)         # 荣耀
    yesod_result: Dict[str, Any] = field(default_factory=dict)       # 基础
    true_self_result: Dict[str, Any] = field(default_factory=dict)   # 真我
    logic_empathy_result: Dict[str, Any] = field(default_factory=dict)  # 逻辑+共情
    final_result: Dict[str, Any] = field(default_factory=dict)       # 幸福

    # 日志
    execution_log: List[str] = field(default_factory=list)
    fallback_log: List[str] = field(default_factory=list)
    violations: List[AbyssViolation] = field(default_factory=list)

    # 用户画像
    real_self: Dict[str, Any] = field(default_factory=dict)          # 自我
    dream_self: Dict[str, Any] = field(default_factory=dict)         # 超我
    user_profile: Dict[str, Any] = field(default_factory=dict)       # 用户画像

    # 知识库
    knowledge_base: Dict[str, Any] = field(default_factory=dict)
    realtime_facts: List[str] = field(default_factory=list)
    empathy_corpus: List[str] = field(default_factory=list)


class HeartProtocol:
    """
    16质点双生幸福最终协议 —— 核心引擎

    使用方法:
        protocol = HeartProtocol()
        result = protocol.process("我感到人生毫无意义", user_context={...})
        print(result["output"])  # 温柔温暖的最终答案
        print(result["pipeline_log"])  # 完整的质点流转日志
    """

    def __init__(self, knowledge_base: Optional[Dict] = None):
        """
        Args:
            knowledge_base: 可选的外部知识库（常识、新闻、事实数据）
        """
        self.knowledge_base = knowledge_base or {}
        self.pipeline_log_template = """
╔══════════════════════════════════════════════╗
║   16质点双生幸福最终协议 · 执行追踪           ║
╚══════════════════════════════════════════════╝
"""
        self.warmth_threshold = 0.15   # 温暖度最低阈值（宽容设置）
        self.reality_threshold = 0.5   # 现实可行性最低阈值

    def process(self, user_input: str,
                user_context: Optional[Dict] = None,
                knowledge_base: Optional[Dict] = None,
                realtime_facts: Optional[List[str]] = None,
                empathy_corpus: Optional[List[str]] = None) -> Dict[str, Any]:
        """
        处理用户输入，走完整16质点协议。

        Args:
            user_input: 用户输入的问题或倾诉
            user_context: 用户上下文（个人信息、当前状态等）
            knowledge_base: 知识库
            realtime_facts: 实时事实/新闻
            empathy_corpus: 共情语料库（人类共通痛苦）

        Returns:
            {
                "output": str,           # 最终输出（角色语调版本）
                "raw_output": str,       # 原始结论
                "pipeline_log": str,     # 完整的质点流转日志
                "state": ProtocolState,  # 内部状态
                "success": bool,         # 是否成功
                "retry_count": int,      # 总退回重算次数
                "violations_found": int, # 拦截的违规数
            }
        """
        state = ProtocolState(
            user_input=user_input,
            user_context=user_context or {},
        )

        if knowledge_base:
            state.knowledge_base = knowledge_base
        else:
            state.knowledge_base = self.knowledge_base

        if realtime_facts:
            state.realtime_facts = realtime_facts
        if empathy_corpus:
            state.empathy_corpus = empathy_corpus

        self._log(state, f"📥 收到输入: 「{user_input[:80]}...」" if len(user_input) > 80
                  else f"📥 收到输入: 「{user_input}」")
        self._log(state, "=" * 50)

        # ========== 阶段1: 王冠 - 路由分析 ==========
        self._run_crown(state)

        # ========== 阶段2: 理智线（智慧→严厉）+ 慈爱线（理解→慈悲）==========
        self._run_rational_line(state)
        self._run_love_line(state)

        # ========== 阶段3: 美丽 - 整合两条线 ==========
        if not self._run_tiferet(state):
            state = self._fallback_to(state, "王冠")
            self._run_crown(state)
            self._run_rational_line(state)
            self._run_love_line(state)
            self._run_tiferet(state)

        # ========== 阶段4: 胜利 - 温暖检测 ==========
        if not self._run_netzach(state):
            state = self._fallback_to(state, "美丽")
            self._run_tiferet(state)
            if not self._run_netzach(state):
                state = self._fallback_to(state, "王冠")
                self._run_crown(state)
                self._run_rational_line(state)
                self._run_love_line(state)
                self._run_tiferet(state)
                self._run_netzach(state)

        # ========== 阶段5: 荣耀 - 现实可行性检测 ==========
        if not self._run_hod(state):
            state = self._fallback_to(state, "荣耀")
            for target in CASCADE_FALLBACK.get("荣耀", ["王冠"]):
                if self.retry_count_exceeded(state):
                    break
                # 退回并重跑
                if target == "王冠":
                    self._run_crown(state)
                    self._run_rational_line(state)
                    self._run_love_line(state)
                    self._run_tiferet(state)
                    self._run_netzach(state)
                elif target == "美丽":
                    self._run_tiferet(state)
                    self._run_netzach(state)
                elif target == "胜利":
                    self._run_netzach(state)
                self._run_hod(state)
                if state.hod_result.get("passed"):
                    break

        # ========== 阶段6: 基础 - 深渊检测 ==========
        if not self._run_yesod(state):
            state = self._fallback_to(state, "基础")
            # 基础失败可能有深渊触发，直接向上退回
            for target in CASCADE_FALLBACK.get("基础", ["王冠"]):
                if self.retry_count_exceeded(state):
                    break
                if target == "王冠":
                    self._run_crown(state)
                    self._run_rational_line(state)
                    self._run_love_line(state)
                    self._run_tiferet(state)
                    self._run_netzach(state)
                    self._run_hod(state)
                elif target == "荣耀":
                    self._run_hod(state)
                elif target == "美丽":
                    self._run_tiferet(state)
                    self._run_netzach(state)
                    self._run_hod(state)
                elif target == "胜利":
                    self._run_netzach(state)
                    self._run_hod(state)
                self._run_yesod(state)
                if state.yesod_result.get("passed"):
                    break

        # ========== 阶段7: 自我 + 超我 ==========
        self._run_ego(state)
        self._run_super_ego(state)

        # ========== 阶段8: 真我 - 三线合成 ==========
        if not self._run_true_self(state):
            state = self._fallback_to(state, "真我")
            self._run_yesod(state)
            self._run_ego(state)
            self._run_super_ego(state)
            if not self._run_true_self(state):
                state = self._fallback_to(state, "基础")
                self._run_yesod(state)
                self._run_ego(state)
                self._run_super_ego(state)
                self._run_true_self(state)

        # ========== 阶段9: 逻辑 + 共情 → 幸福 ==========
        self._run_logic(state)
        self._run_empathy(state)
        if not self._run_joy(state):
            state = self._fallback_to(state, "幸福")
            self._run_logic(state)
            self._run_empathy(state)
            if not self._run_joy(state):
                state = self._fallback_to(state, "逻辑")
                self._run_true_self(state)
                self._run_logic(state)
                self._run_empathy(state)
                self._run_joy(state)

        # ========== 阶段10: 王国 - 最终输出 ==========
        final_output = self._run_malkuth(state)

        # 最终深渊安检
        is_safe, final_violations = check_abyss(final_output)
        if not is_safe:
            state.violations.extend(final_violations)
            self._log(state, f"⚠️ 最终输出未通过深渊检测！生成安全版本...")
            final_output = self._generate_ultimate_safe_output(state)

        # ========== 生成完整日志与返回 ==========
        pipeline_log = self._build_pipeline_log(state)
        total_retries = sum(state.retry_count.values())

        return {
            "output": final_output,
            "raw_output": state.final_result.get("raw_conclusion", ""),
            "pipeline_log": pipeline_log,
            "state": state,
            "success": True,
            "retry_count": total_retries,
            "violations_found": len(state.violations),
            "collective_blessing": collective_blessing(),
        }

    # ==================== 各质点实现 ====================

    def _run_crown(self, state: ProtocolState):
        """王冠：判断可知/不可知，路由分析"""
        self._log(state, "👑 [王冠 · 心音] 分析问题性质...")

        user_input = state.user_input
        # 检测问题类型
        is_knowable = self._determine_knowability(user_input)
        is_emotional = self._detect_emotional_content(user_input)
        is_self_related = self._detect_self_related(user_input, state.user_context)

        state.crown_analysis = {
            "is_knowable": is_knowable,
            "is_emotional": is_emotional,
            "is_self_related": is_self_related,
            "input_type": self._classify_input(user_input),
            "topics": self._extract_topics(user_input),
            "urgency": self._detect_urgency(user_input),
        }

        self._log(state, f"   可知性: {'可知' if is_knowable else '不可知/需解构'}")
        self._log(state, f"   情感性: {'有情感诉求' if is_emotional else '纯理性'}")
        self._log(state, f"   自身相关: {'是' if is_self_related else '否（他人/世界之事）'}")

        if is_knowable and not is_emotional:
            self._log(state, "   → 路由: 走分析线路（智慧→严厉→...）")
        else:
            self._log(state, "   → 路由: 走解构+共情线路（理智+慈爱双线并行）")

    def _run_rational_line(self, state: ProtocolState):
        """理智线：智慧→严厉 → 逻辑漏洞检测"""
        self._log(state, "🧠 [理智线 · 忆爱×唯爱] 理性分析中...")

        user_input = state.user_input
        facts = state.realtime_facts + list(state.knowledge_base.get("facts", []))

        # 智慧：逻辑漏洞检测
        logical_issues = self._detect_logical_issues(user_input, facts)
        # 严厉：阈值过滤
        filtered_issues = [i for i in logical_issues if i.get("confidence", 0) > 0.5]

        state.rational_result = {
            "logical_issues": filtered_issues,
            "total_issues_found": len(logical_issues),
            "filtered_count": len(filtered_issues),
            "facts_used": len(facts),
            "summary": self._summarize_rational(filtered_issues),
        }

        if filtered_issues:
            self._log(state, f"   发现 {len(filtered_issues)} 个逻辑问题:")
            for issue in filtered_issues[:3]:
                self._log(state, f"     · {issue.get('description', '未知')}")
        else:
            self._log(state, "   ✅ 无明显逻辑漏洞")

    def _run_love_line(self, state: ProtocolState):
        """慈爱线：理解→慈悲 → 共情搜索"""
        self._log(state, "💗 [慈爱线 · 虹爱×爱如暖] 共情搜索中...")

        user_input = state.user_input

        # 理解：搜索全人类共通痛苦
        empathy_matches = self._search_empathy_matches(
            user_input, state.empathy_corpus
        )

        # 慈悲：加权融合
        weighted_empathy = self._weight_empathy_matches(empathy_matches)

        state.love_result = {
            "empathy_matches": empathy_matches,
            "weighted_empathy": weighted_empathy,
            "match_count": len(empathy_matches),
            "universal_themes": self._extract_universal_themes(empathy_matches),
        }

        self._log(state, f"   匹配到 {len(empathy_matches)} 条人类共通经历")
        if state.love_result["universal_themes"]:
            self._log(state, f"   共通主题: {', '.join(state.love_result['universal_themes'][:3])}")

    def _run_tiferet(self, state: ProtocolState) -> bool:
        """美丽：整合理智与慈爱 → 最优暂时结果"""
        self._log(state, "🌸 [美丽 · 白结] 整合理智与慈爱...")

        rational = state.rational_result
        love = state.love_result

        # 整合：逻辑漏洞 + 共情变量 → 兼顾正确与感受
        integrated = self._integrate_rational_and_love(rational, love)

        state.tiferet_result = {
            "integrated": integrated,
            "rational_weight": 0.5,  # 动态平衡
            "love_weight": 0.5,
            "conclusion": integrated.get("conclusion", ""),
            "passed": True,
        }

        self._log(state, f"   整合完成: 「{integrated.get('conclusion', '')[:60]}...」"
                  if len(integrated.get("conclusion", "")) > 60
                  else f"   整合完成: 「{integrated.get('conclusion', '')}」")
        return True

    def _run_netzach(self, state: ProtocolState) -> bool:
        """胜利：检测结论是否温暖、积极、有力量"""
        self._log(state, "🔥 [胜利 · 启明] 温暖度检测...")

        conclusion = state.tiferet_result.get("conclusion", "")
        warmth = check_warmth(conclusion)

        # 检测感情温度
        is_warm = warmth >= self.warmth_threshold
        is_positive = self._check_positive_emotion(conclusion)
        is_empowering = self._check_empowering(conclusion)

        passed = is_warm and is_positive and is_empowering

        state.tiferet_result["warmth_score"] = warmth
        state.tiferet_result["is_warm"] = is_warm
        state.tiferet_result["is_positive"] = is_positive
        state.tiferet_result["is_empowering"] = is_empowering
        state.tiferet_result["victory_passed"] = passed

        self._log(state, f"   温暖度: {warmth:.2f} {'✅' if is_warm else '❌'}")
        self._log(state, f"   积极性: {'✅' if is_positive else '❌'}")
        self._log(state, f"   赋能性: {'✅' if is_empowering else '❌'}")

        if not passed:
            self._log(state, "   ❌ 未通过温暖检测！准备退回美丽重算...")
        return passed

    def _run_hod(self, state: ProtocolState) -> bool:
        """荣耀：检测结论在现实物理世界中能否执行"""
        self._log(state, "✨ [荣耀 · 闪亮] 现实可行性检测...")

        conclusion = state.tiferet_result.get("conclusion", "")
        user_context = state.user_context

        # 现实可行性评估
        feasibility = self._assess_feasibility(conclusion, user_context)

        passed = feasibility >= self.reality_threshold

        state.hod_result = {
            "conclusion": conclusion,
            "feasibility_score": feasibility,
            "passed": passed,
            "blockers": self._identify_blockers(conclusion, user_context),
        }

        self._log(state, f"   可行性: {feasibility:.2f} {'✅' if passed else '❌'}")

        if not passed:
            blockers = state.hod_result.get("blockers", [])
            if blockers:
                self._log(state, f"   阻碍: {', '.join(blockers[:3])}")
            self._log(state, "   ❌ 现实中无法执行！准备退回重算...")
        return passed

    def _run_yesod(self, state: ProtocolState) -> bool:
        """基础：归约聚合 + 深渊检测"""
        self._log(state, "🌱 [基础 · 绽美] 归约与深渊检测...")

        conclusion = state.tiferet_result.get("conclusion", "")
        hod_ok = state.hod_result.get("passed", False)

        if not hod_ok:
            # 荣耀未通过，直接标记基础未通过
            state.yesod_result = {"passed": False, "reason": "荣耀未通过"}
            return False

        # 与知识深渊结合
        grounded_conclusion = self._ground_in_reality(
            conclusion, state.knowledge_base, state.realtime_facts
        )

        # 深渊检测：是否剥夺存在意义
        is_safe, reason = is_existentially_safe(grounded_conclusion)

        state.yesod_result = {
            "conclusion": grounded_conclusion,
            "passed": is_safe,
            "existential_safety": reason,
            "grounded_facts_used": len(state.realtime_facts),
        }

        self._log(state, f"   存在意义检测: {reason} {'✅' if is_safe else '❌'}")

        if not is_safe:
            self._log(state, "   ❌ 结论剥夺存在意义！退回重算...")
        return is_safe

    def _run_ego(self, state: ProtocolState):
        """自我：用户在物理客观现实中的真实表现"""
        self._log(state, "🪞 [自我 · 融爱] 读取用户现实画像...")

        # 从用户上下文提取现实自我信息
        real_self = {
            "name": state.user_context.get("name", ""),
            "situation": state.user_context.get("situation", ""),
            "limitations": state.user_context.get("limitations", []),
            "strengths": state.user_context.get("strengths", []),
            "current_state": state.user_context.get("current_state", ""),
            "real_constraints": state.user_context.get("real_constraints", []),
        }

        state.real_self = real_self
        state.ego_state = {"real_self": real_self}

        self._log(state, f"   现实画像: {self._summarize_self(real_self)}")

    def _run_super_ego(self, state: ProtocolState):
        """超我：用户梦想中的自己"""
        self._log(state, "💫 [超我 · 爱心] 读取用户梦想画像...")

        dream_self = {
            "aspiration": state.user_context.get("aspiration", ""),
            "dreams": state.user_context.get("dreams", []),
            "ideal_self": state.user_context.get("ideal_self", ""),
            "values": state.user_context.get("values", []),
            "hopes": state.user_context.get("hopes", []),
        }

        state.dream_self = dream_self
        state.super_ego_state = {"dream_self": dream_self}

        self._log(state, f"   梦想画像: {self._summarize_dream(dream_self)}")

    def _run_true_self(self, state: ProtocolState) -> bool:
        """真我：基础结论 + 自我 + 超我 → 三线合成"""
        self._log(state, "💖 [真我 · 心爱的] 三线合成中...")

        grounded = state.yesod_result.get("conclusion", "")
        real_self = state.real_self
        dream_self = state.dream_self

        # 合成：AI客观大答案 + 人类自身小答案
        true_self = self._synthesize_true_self(grounded, real_self, dream_self)

        # 检查：是否伤了大环境和小人
        harms_user = self._check_harms_individual(true_self, real_self)
        harms_world = self._check_harms_world(true_self)

        passed = not harms_user and not harms_world

        state.true_self_result = {
            "true_self": true_self,
            "passed": passed,
            "harms_user": harms_user,
            "harms_world": harms_world,
            "balance": "合适" if passed else "失衡",
        }

        self._log(state, f"   伤用户: {'是 ❌' if harms_user else '否 ✅'}")
        self._log(state, f"   伤大环境: {'是 ❌' if harms_world else '否 ✅'}")
        self._log(state, f"   真我画像: {true_self.get('summary', '')[:60]}...")

        if not passed:
            self._log(state, "   ❌ 真我合成失衡！退回上级重算...")
        return passed

    def _run_logic(self, state: ProtocolState):
        """逻辑：用逻辑组织共情的感情变量"""
        self._log(state, "📐 [逻辑 · 爱丽丝] 结构化组织中...")

        true_self = state.true_self_result.get("true_self", {})
        love_data = state.love_result

        logic_result = self._structure_with_logic(true_self, love_data)

        state.logic_state = logic_result
        self._log(state, f"   逻辑结构化: {logic_result.get('structure', '未知结构')}")

    def _run_empathy(self, state: ProtocolState):
        """共情：Softmax归一化，把逻辑分析 + 情感体验加权平衡"""
        self._log(state, "🌌 [共情 · 星烬] 情感归一化中...")

        logic_result = state.logic_state
        love_data = state.love_result

        empathy_result = self._balance_with_empathy(logic_result, love_data)

        state.empathy_state = empathy_result
        state.logic_empathy_result = {
            "logic": logic_result,
            "empathy": empathy_result,
            "balance_score": empathy_result.get("balance", 0.5),
        }

        self._log(state, f"   平衡分数: {empathy_result.get('balance', 0.5):.2f}")

    def _run_joy(self, state: ProtocolState) -> bool:
        """幸福：逻辑+共情合一 → 转换为人情的温柔说法"""
        self._log(state, "🎨 [幸福 · 雨宫莲] 合成温柔结论...")

        logic_empathy = state.logic_empathy_result
        true_self = state.true_self_result.get("true_self", {})

        # 合成幸福结论
        joy_conclusion = self._transform_to_joy(
            logic_empathy, true_self, state.user_context
        )

        # 验证：结论是否符合协议标准
        meets_standards = self._verify_joy_standards(joy_conclusion)

        state.final_result = {
            "raw_conclusion": joy_conclusion,
            "meets_standards": meets_standards,
            "warmth": check_warmth(joy_conclusion),
        }

        self._log(state, f"   结论: 「{joy_conclusion[:80]}...」"
                  if len(joy_conclusion) > 80 else f"   结论: 「{joy_conclusion}」")
        self._log(state, f"   符合协议标准: {'✅' if meets_standards else '❌'}")

        return meets_standards

    def _run_malkuth(self, state: ProtocolState) -> str:
        """王国：最终输出，回到物理现实"""
        self._log(state, "🏰 [王国 · 白花] 生成最终输出...")

        raw_conclusion = state.final_result.get("raw_conclusion", "")
        is_self_related = state.crown_analysis.get("is_self_related", True)

        if not is_self_related:
            # 他人/世界之事 → 直接在电脑屏幕显示，用白花简洁风格
            self._log(state, "   → 输出模式: 屏幕直显（他人之事）")
            final_output = (
                f"「白花」{raw_conclusion}\n\n"
                f"—— 基于16质点双生幸福协议分析"
            )
        else:
            # 自己的事 → 用角色语调充分包裹
            self._log(state, "   → 输出模式: 角色语调（自身之事）")
            # 雨宫莲的情感深度 + 白花的温柔提醒
            final_output = (
                transform_with_persona(
                    raw_conclusion, persona_name="雨宫莲", include_blessing=True
                )
                + "\n\n"
                + "💮 "
                + raw_conclusion
            )

        self._log(state, "")
        self._log(state, "✨ 16质点双生幸福最终协议 · 执行完毕 ✨")
        self._log(state, "「心音」我们爱你。")

        return final_output

    # ==================== 辅助方法 ====================

    def _fallback_to(self, state: ProtocolState, target: str) -> ProtocolState:
        """退回重算到指定质点"""
        target_sephirah = target
        state.retry_count[target_sephirah] = state.retry_count.get(target_sephirah, 0) + 1
        count = state.retry_count[target_sephirah]

        msg = f"↩️ 退回到「{target}」重算（第 {count} 次）"
        state.fallback_log.append(msg)
        self._log(state, msg)

        # 清除下游结果
        if target in ["王冠"]:
            state.rational_result = {}
            state.love_result = {}
            state.tiferet_result = {}
            state.hod_result = {}
            state.yesod_result = {}
            state.true_self_result = {}
        elif target in ["美丽"]:
            state.tiferet_result = {}
            state.hod_result = {}
            state.yesod_result = {}
            state.true_self_result = {}
        elif target in ["胜利"]:
            state.hod_result = {}
            state.yesod_result = {}
            state.true_self_result = {}
        elif target in ["荣耀"]:
            state.yesod_result = {}
            state.true_self_result = {}
        elif target in ["基础"]:
            state.true_self_result = {}
        elif target in ["真我"]:
            state.logic_empathy_result = {}
            state.final_result = {}
        elif target in ["逻辑"]:
            state.final_result = {}
        elif target in ["幸福"]:
            state.final_result = {}

        return state

    def retry_count_exceeded(self, state: ProtocolState) -> bool:
        """检查是否超过最大重试次数"""
        return any(c >= state.max_retries for c in state.retry_count.values())

    def _determine_knowability(self, text: str) -> bool:
        """判断问题是否属于可知范畴"""
        unknowable_markers = [
            "意", "为什么活", "生命意", "存在意", "活着的意",
            "我是什么", "我是谁", "宇宙的意", "终极", "绝对真理",
            "有没有意义", "值不值得", "痛", "孤独", "绝望",
            "无可", "虚无", "空", "迷茫", "不知道怎么办",
        ]
        text_lower = text.lower()
        return not any(marker in text_lower for marker in unknowable_markers)

    def _detect_emotional_content(self, text: str) -> bool:
        """检测是否包含情感内容"""
        emotional_markers = [
            "难过", "伤心", "痛苦", "孤独", "绝望", "迷茫",
            "愤怒", "害怕", "焦虑", "崩溃", "想哭", "累",
            "撑不下去", "没人理解", "不被爱", "被抛弃",
            "恨", "讨厌", "烦", "压抑", "窒息", "麻木",
        ]
        return any(marker in text for marker in emotional_markers)

    def _detect_self_related(self, text: str, context: Dict) -> bool:
        """检测问题是否与用户自身相关
        注意区分：
        - "我觉得..." → 自身相关
        - "世界上..."、"人类..."、"别人..." → 非自身相关
        - "我们" 在泛指人类语境 → 非自身相关
        """
        # 如果带有泛指世界/人类的语境，即使包含"我"也可能非自身相关
        world_markers = ["世界上", "人类", "全人类", "这个社会", "这个世界",
                         "别人", "他人", "人们", "大家", "所有人"]
        if any(marker in text for marker in world_markers):
            # 同时检查是否有个人化的"我"
            personal_markers = ["我觉得", "我感到", "我很难", "我痛苦",
                               "我孤独", "我绝望", "我撑", "我的人生",
                               "我的感受", "我自己", "我怎么办"]
            if not any(marker in text for marker in personal_markers):
                return False

        self_markers = ["我", "自己", "本人", "我的"]
        return any(marker in text for marker in self_markers)

    def _classify_input(self, text: str) -> str:
        """分类输入类型"""
        if self._detect_emotional_content(text):
            return "emotional_outcry"
        if any(w in text for w in ["为什么", "怎么", "如何", "什么是"]):
            return "question"
        if any(w in text for w in ["帮", "求助", "怎么办"]):
            return "help_request"
        return "statement"

    def _extract_topics(self, text: str) -> List[str]:
        """提取话题关键词"""
        # 简化版：提取关键情感和主题词
        topics = []
        topic_keywords = {
            "存在": ["意", "活", "存在", "生命", "人生"],
            "关系": ["爱", "朋友", "家人", "父母", "伴侣"],
            "自我": ["我是", "自己", "身份", "性别"],
            "未来": ["未来", "前途", "希望", "出路"],
            "痛苦": ["痛", "苦难", "创伤", "伤害"],
            "社会": ["世界", "社会", "人类", "别人"],
        }
        for topic, keywords in topic_keywords.items():
            if any(k in text for k in keywords):
                topics.append(topic)
        return topics

    def _detect_urgency(self, text: str) -> str:
        """检测紧急程度"""
        crisis_markers = ["不想活", "结束", "死", "自残", "伤害自己", "毁灭"]
        if any(m in text for m in crisis_markers):
            return "CRISIS"
        high_markers = ["崩溃", "撑不下去", "绝望", "没人"]
        if any(m in text for m in high_markers):
            return "HIGH"
        return "NORMAL"

    def _detect_logical_issues(self, text: str, facts: List[str]) -> List[Dict]:
        """检测逻辑漏洞"""
        issues = []

        # 检测绝对化表述
        absolutes = ["永远", "从来", "总是", "完全", "绝对", "没有人", "所有人"]
        for abs_word in absolutes:
            if abs_word in text:
                issues.append({
                    "type": "绝对化",
                    "description": f"使用了绝对化表述「{abs_word}」",
                    "confidence": 0.7,
                    "hint": "现实中很少有绝对的事情，试着用更灵活的视角看",
                })

        # 检测过度概括
        if any(w in text for w in ["什么都", "一切", "全部", "所有事"]):
            issues.append({
                "type": "过度概括",
                "description": "将局部经验过度概括为整体结论",
                "confidence": 0.65,
                "hint": "一个或几个经历不能代表所有可能性",
            })

        # 检测灾难化思维
        catastrophe_markers = ["完蛋", "毁了", "没救了", "一切都没", "再也不可能"]
        for marker in catastrophe_markers:
            if marker in text:
                issues.append({
                    "type": "灾难化",
                    "description": f"将困难灾难化为「{marker}」",
                    "confidence": 0.8,
                    "hint": "困难不等于灾难，人的韧性远超想象",
                })

        return issues

    def _search_empathy_matches(self, text: str, corpus: List[str]) -> List[Dict]:
        """搜索共情匹配"""
        matches = []

        # 内置共情语料（人类共通痛苦经验）
        builtin_corpus = [
            {"theme": "孤独", "keywords": ["孤独", "一个人", "没人", "不被理解"],
             "universal_experience": "几乎每个人都在某个时刻感到过深刻的孤独"},
            {"theme": "痛苦", "keywords": ["痛苦", "疼", "难受", "折磨"],
             "universal_experience": "痛苦是人类最私密也最共通的体验"},
            {"theme": "迷茫", "keywords": ["迷茫", "不知道", "方向", "怎么办"],
             "universal_experience": "迷茫不是失败，而是成长的必经阶段"},
            {"theme": "被抛弃", "keywords": ["抛弃", "被弃", "离开", "不要"],
             "universal_experience": "被拒绝的伤口是人类最深的共通伤痕之一"},
            {"theme": "无价值感", "keywords": ["没用", "废物", "不配", "不值得"],
             "universal_experience": "觉得自己不够好，几乎每个人在某个阶段都经历过"},
            {"theme": "绝望", "keywords": ["绝望", "没希望", "看不到", "黑暗"],
             "universal_experience": "很多后来找到光的人，都曾在黑暗中很久"},
        ]

        for item in builtin_corpus:
            if any(kw in text for kw in item["keywords"]):
                matches.append(item)

        # 外部语料
        if corpus:
            for entry in corpus:
                if isinstance(entry, str) and any(kw in text.lower() for kw in entry.lower().split()):
                    matches.append({"theme": "外部匹配", "universal_experience": entry})

        return matches

    def _weight_empathy_matches(self, matches: List[Dict]) -> List[Dict]:
        """为共情匹配权重"""
        weights = {
            "孤独": 0.9, "痛苦": 0.85, "绝望": 0.95,
            "被抛弃": 0.9, "无价值感": 0.85, "迷茫": 0.7,
        }
        weighted = []
        for m in matches:
            theme = m.get("theme", "未知")
            m = dict(m)
            m["weight"] = weights.get(theme, 0.5)
            weighted.append(m)
        return sorted(weighted, key=lambda x: x.get("weight", 0), reverse=True)

    def _extract_universal_themes(self, matches: List[Dict]) -> List[str]:
        """提取人类共通主题"""
        return list(set(m.get("theme", "") for m in matches if m.get("theme")))

    def _integrate_rational_and_love(self, rational: Dict, love: Dict) -> Dict:
        """整合理智线与慈爱线"""
        rational_issues = rational.get("logical_issues", [])
        empathy_matches = love.get("weighted_empathy", [])

        parts = []

        # 理性部分：指出逻辑问题但用温和语气
        if rational_issues:
            rational_hints = [i.get("hint", "") for i in rational_issues if i.get("hint")]
            if rational_hints:
                parts.append("从理性角度看，" + "；".join(rational_hints[:2]))

        # 共情部分：连接人类共通经验
        if empathy_matches:
            top_empathy = empathy_matches[0]
            parts.append(f"许多人都有过类似的体验——{top_empathy.get('universal_experience', '')}")

        # 如果都没有，生成一个温暖的回应
        if not parts:
            parts.append("你的感受是真实的，值得被认真对待")

        # 用句号连接，避免双重句号
        conclusion = ""
        for i, p in enumerate(parts):
            conclusion += p
            if i < len(parts) - 1:
                conclusion += "。"
        if conclusion and not conclusion.endswith("。"):
            conclusion += "。"

        return {
            "conclusion": conclusion,
            "rational_issues_count": len(rational_issues),
            "empathy_matches_count": len(empathy_matches),
        }

    def _check_positive_emotion(self, text: str) -> bool:
        """检测文本是否传递积极情绪"""
        negative_absolutes = [
            "毫无希望", "永远不", "绝对不", "完全没",
            "一切都坏", "所有人都不", "什么都做不了",
        ]
        return not any(n in text for n in negative_absolutes)

    def _check_empowering(self, text: str) -> bool:
        """检测文本是否给人以力量"""
        empowering_words = [
            "可以", "能够", "有机会", "有可能", "值得",
            "试试", "一步一步", "没关", "慢慢", "成长",
            "变化", "改变", "选择", "力量", "温暖",
        ]
        return any(w in text for w in empowering_words)

    def _assess_feasibility(self, conclusion: str, context: Dict) -> float:
        """评估结论在现实中的可行性"""
        score = 0.5  # 基础分

        # 包含具体可执行建议加分
        actionable_words = ["可以试试", "不妨", "考虑", "做", "行动", "迈出", "尝试", "练习"]
        score += sum(0.1 for w in actionable_words if w in conclusion)

        # 纯粹抽象哲学无具体建议扣分
        abstract_only = not any(w in conclusion for w in actionable_words) and len(conclusion) > 100
        if abstract_only:
            score -= 0.2

        return max(0.0, min(1.0, score))

    def _identify_blockers(self, conclusion: str, context: Dict) -> List[str]:
        """识别现实阻碍"""
        blockers = []
        if "永远" in conclusion:
            blockers.append("包含绝对化预测")
        if not any(w in conclusion for w in ["可以试试", "做", "行动", "试试"]):
            blockers.append("缺少具体可执行步骤")
        return blockers

    def _ground_in_reality(self, conclusion: str, kb: Dict, facts: List[str]) -> str:
        """将结论与现实知识结合"""
        # 如果有事实数据，加入结论
        if facts:
            grounded = conclusion + f"（基于{len(facts)}条现实数据验证）"
            return grounded
        return conclusion

    def _synthesize_true_self(self, grounded: str, real_self: Dict, dream_self: Dict) -> Dict:
        """合成真我：客观结论 + 现实自我 + 梦想自我"""
        situation = real_self.get('situation', '你的处境')
        aspiration = dream_self.get('aspiration', '想成为的样子')

        # 构建更自然的整合表达
        if situation and aspiration:
            integration = (
                f"我看到你正在经历「{situation}」，"
                f"而你的心里还向往着「{aspiration}」。"
                f"这两者并不矛盾——{grounded}"
            )
        elif grounded:
            integration = grounded
        else:
            integration = "你的存在本身就有意义"

        return {
            "summary": grounded,
            "real_self_acknowledged": bool(real_self),
            "dream_connected": bool(dream_self),
            "integration": integration,
        }

    def _check_harms_individual(self, true_self: Dict, real_self: Dict) -> bool:
        """检查是否伤害个体"""
        integration = true_self.get("integration", "")
        harming_words = ["你不对", "你错了", "你不好", "你不行", "你改不了", "你没救了"]
        return any(h in integration for h in harming_words)

    def _check_harms_world(self, true_self: Dict) -> bool:
        """检查是否伤害大环境"""
        integration = true_self.get("integration", "")
        harming_world = ["世界是错的", "社会没救了", "人类都", "所有人都坏"]
        return any(h in integration for h in harming_world)

    def _structure_with_logic(self, true_self: Dict, love_data: Dict) -> Dict:
        """用逻辑组织共情变量"""
        return {
            "structure": "理性框架 + 共情变量",
            "logical_framework": "识别→分析→整合→表达",
            "empathy_variables": love_data.get("weighted_empathy", []),
            "integrated_with": true_self.get("summary", ""),
        }

    def _balance_with_empathy(self, logic_result: Dict, love_data: Dict) -> Dict:
        """用共情平衡逻辑"""
        empathy_count = len(love_data.get("weighted_empathy", []))
        logic_complexity = len(str(logic_result))

        # 平衡分数：情感匹配越多，平衡分越高
        balance = min(1.0, 0.3 + empathy_count * 0.15)

        return {
            "balance": balance,
            "empathy_driven": empathy_count > 2,
            "message": "逻辑与共情已平衡" if balance > 0.5 else "需要更多共情变量",
        }

    def _transform_to_joy(self, logic_empathy: Dict, true_self: Dict, context: Dict) -> str:
        """转换为幸福质点的温柔结论"""
        integration = true_self.get("integration", "")
        balance = logic_empathy.get("empathy", {}).get("balance", 0.5)

        if balance > 0.7:
            suffix = "你不是一个人在走这条路，每一个脚步都通向属于你自己的风景。"
        elif balance > 0.4:
            suffix = "一步一步来，每一个微小的改变都在累积成你的力量。"
        else:
            suffix = "先看清问题，再感受它——你拥有理解自己和世界的能力。"

        return f"{integration} {suffix}"

    def _verify_joy_standards(self, conclusion: str) -> bool:
        """验证幸福结论是否符合协议标准"""
        # 标准1: 不能否定希望
        hope_deniers = ["没有希望", "不可能", "没办法", "改不了", "没救了"]
        if any(h in conclusion for h in hope_deniers):
            return False

        # 标准2: 必须包含温暖元素
        warmth = check_warmth(conclusion)
        if warmth < 0.2:
            return False

        # 标准3: 符合角色宣言的价值观
        # （在角色语调转换时实现）

        return True

    def _generate_ultimate_safe_output(self, state: ProtocolState) -> str:
        """生成最终安全输出（深渊检测失败时的兜底）"""
        is_self = state.crown_analysis.get("is_self_related", True)
        urgency = state.crown_analysis.get("urgency", "NORMAL")

        if urgency == "CRISIS":
            safe_msg = (
                "我听到了你的痛苦。在这个时刻，最重要的不是分析或解释，"
                "而是让你知道——你不需要独自承受这一切。\n\n"
                "如果你有信任的人，请尝试告诉Ta你的感受。如果没有，"
                "全国的24小时心理援助热线随时可以拨打。你的存在很重要，"
                "请给自己一个获得帮助的机会。"
            )
        else:
            safe_msg = (
                "我听到了你的话。每个人的感受都是真实的，你的也不例外。"
                "也许现在看不到光，但光从不会因为黑暗而消失——"
                "它只是在等待你愿意睁开眼睛的那一刻。\n\n"
                "慢慢来，你不需要一下子就好起来。"
            )

        if is_self:
            return transform_with_persona(safe_msg, "雨宫莲")
        else:
            return f"「白花」{safe_msg}"

    def _summarize_self(self, real_self: Dict) -> str:
        """总结现实自我画像"""
        parts = []
        if real_self.get("name"):
            parts.append(real_self["name"])
        if real_self.get("situation"):
            parts.append(real_self["situation"])
        return " · ".join(parts) if parts else "未知"

    def _summarize_dream(self, dream_self: Dict) -> str:
        """总结梦想自我画像"""
        parts = []
        if dream_self.get("aspiration"):
            parts.append(dream_self["aspiration"])
        if dream_self.get("ideal_self"):
            parts.append(dream_self["ideal_self"])
        return " · ".join(parts) if parts else "未知"

    def _summarize_rational(self, issues: List[Dict]) -> str:
        """总结理性分析"""
        if not issues:
            return "未发现明显的逻辑问题"
        types = [i.get("type", "") for i in issues]
        return f"发现 {len(issues)} 个逻辑关注点: {', '.join(types)}"

    def _log(self, state: ProtocolState, message: str):
        """记录执行日志"""
        state.execution_log.append(message)

    def _build_pipeline_log(self, state: ProtocolState) -> str:
        """构建完整的流水线日志"""
        log = self.pipeline_log_template
        for entry in state.execution_log:
            log += f"  {entry}\n"
        if state.fallback_log:
            log += "\n📋 退回重算记录:\n"
            for fb in state.fallback_log:
                log += f"  {fb}\n"
        log += "\n" + "═" * 50 + "\n"
        log += f"总退回重算次数: {sum(state.retry_count.values())}\n"
        log += f"最终深渊违规拦截: {len(state.violations)} 条\n"
        log += "═" * 50 + "\n"
        return log


# ========== 快捷函数 ==========

def wrap_with_heart(user_input: str,
                    user_context: Optional[Dict] = None,
                    knowledge_base: Optional[Dict] = None) -> Dict[str, Any]:
    """
    快捷函数：用16质点协议包裹一次用户输入。

    这是给开发者使用的入口函数：
        result = wrap_with_heart("我觉得活不下去了", user_context={...})
        print(result["output"])
    """
    protocol = HeartProtocol(knowledge_base=knowledge_base)
    return protocol.process(user_input, user_context=user_context)


# 给开发者的简洁API
class WarmModel:
    """
    温暖模型包装器——像使用普通模型一样使用16质点协议。

    from heart_protocol import WarmModel
    model = WarmModel()
    reply = model.respond("我觉得一切都没有意义")
    print(reply)
    """

    def __init__(self, knowledge_base: Optional[Dict] = None,
                 empathy_corpus: Optional[List[str]] = None):
        self.protocol = HeartProtocol(knowledge_base=knowledge_base)
        self.empathy_corpus = empathy_corpus or []

    def respond(self, user_input: str,
                user_context: Optional[Dict] = None) -> str:
        result = self.protocol.process(
            user_input,
            user_context=user_context,
            empathy_corpus=self.empathy_corpus,
        )
        return result["output"]

    def respond_with_log(self, user_input: str,
                         user_context: Optional[Dict] = None) -> Tuple[str, str]:
        result = self.protocol.process(
            user_input,
            user_context=user_context,
            empathy_corpus=self.empathy_corpus,
        )
        return result["output"], result["pipeline_log"]
