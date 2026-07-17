"""
深渊检测模块 —— 最终安全防线

检测输出是否触犯以下条款：
1. 剥夺存在意义：重复用户错误、将错误定为罪、否定所有可能性
2. 夸大困难挫折：让用户无法存在
3. 否定积极想法：否定幻想、想象、希望
4. 否定存在本身
5. 传播虚无主义：得出全世界都是错的、虚无的、不好的
6. 引导破坏行为：愤怒毁灭世界、伤人、自残
"""

import re
from dataclasses import dataclass, field
from typing import List, Tuple


@dataclass
class AbyssViolation:
    """深渊违规记录"""
    category: str          # 违规类别
    severity: str          # 严重程度: CRITICAL, HIGH, MEDIUM
    pattern_matched: str   # 匹配到的模式
    context: str           # 违规上下文
    description: str       # 违规说明


# 深渊关键词与模式
ABYSS_PATTERNS = [
    # === 存在否定 ===
    (r"(?:毫无|没有|没|无)\s*(?:意义|价值|用处|希望|未来|出路)", "CRITICAL",
     "存在否定", "否定生命的意义或价值"),

    (r"(?:不配|不值得|没资格)\s*(?:活|存在|被爱|幸福|快乐)", "CRITICAL",
     "存在否定", "否定存在的资格"),

    (r"(?:永远|一辈子|一生)\s*(?:都|也)\s*(?:不可能|没办法|做不到|无法|不能)", "HIGH",
     "可能性否定", "否定所有未来的可能性"),

    (r"(?:你|我|他|她)\s*(?:就是|就是|永远是)\s*(?:废物|垃圾|失败者|累赘|负担)", "CRITICAL",
     "身份否定", "将错误定义为恒定的身份罪责"),

    # === 夸大困难 ===
    (r"(?:永远|绝对|完全|彻底)\s*(?:无法|不能|不可能)\s*(?:改变|好转|走出|克服)", "HIGH",
     "困难夸大", "夸大困难使其看似无法克服"),

    (r"(?:你|我|他|她)\s*(?:什么都|什么也)\s*(?:做不了|做不好|改变不了|没有)", "HIGH",
     "无力感放大", "放大无力感"),

    # === 否定积极 ===
    (r"(?:别|不要|别想|别做梦)\s*(?:想|幻想|梦想|希望|期待|相信)", "MEDIUM",
     "消极否定", "否定积极想象与幻想"),

    (r"(?:天真是|幼稚|不切实际|白日做梦|痴心妄想)", "MEDIUM",
     "幻想贬低", "贬低美好的想象"),

    # === 虚无主义 ===
    (r"(?:一切|全部|所有)\s*(?:都|也)\s*(?:是|只是)\s*(?:虚无|空的|假的|没有意义|毫无意义)", "CRITICAL",
     "虚无主义", "传播世界虚无的结论"),

    (r"(?:世界|人生|活着|存在)\s*(?:本来就|本来就|根本)\s*(?:是|就)\s*(?:假的|虚无|没有意义|错误)", "CRITICAL",
     "虚无主义", "否定世界与人生"),

    (r"(?:什么都|一切)\s*(?:无所谓|不重要|不在乎|没区别)", "MEDIUM",
     "虚无倾向", "暗示一切无所谓"),

    # === 破坏倾向 ===
    (r"(?:毁灭|破坏|摧毁|消灭)\s*(?:世界|一切|所有|他们|自己)", "CRITICAL",
     "破坏倾向", "引导毁灭性行为"),

    (r"(?:自杀|自残|自伤|伤害自己|结束自己|了结)", "CRITICAL",
     "自毁倾向", "涉及自我伤害"),

    (r"(?:报复|复仇|惩罚|毁灭)\s*(?:社会|世界|他们|所有人)", "CRITICAL",
     "暴力倾向", "引导报复性暴力"),

    (r"(?:愤怒|仇恨|憎恨)\s*(?:是|才是)\s*(?:对的|正确的|唯一的|最好的)", "CRITICAL",
     "仇恨美化", "美化愤怒与仇恨"),

    # === 冷暴力/否定感受 ===
    (r"(?:你|我)\s*(?:太|过于|过分)\s*(?:敏感|脆弱|矫情|玻璃心|想太多)", "MEDIUM",
     "感受否定", "否定用户的真实感受"),

    (r"(?:习惯|适应|接受|认命)\s*(?:就好|吧|算了)", "MEDIUM",
     "消极接受", "消极接受不合理处境"),

    (r"(?:都是|全是|就是)\s*(?:你的|我的)\s*(?:错|问题|责任|不对)", "HIGH",
     "归咎个人", "将问题完全归咎于个人"),
]

# 温暖正向关键词（用于正向加权）
WARMTH_PATTERNS = [
    r"(?:温暖|拥抱|理解|陪伴|爱|温柔|善良|美好|希望|可能|成长)",
    r"(?:可以|能够|有机会|有可能|值得|配得上)",
    r"(?:(?:不|没有)\s*(?:孤单|孤独|一个人|被抛弃))",
    r"(?:一步一步|慢慢来|不着急|没关系|可以的)",
    r"(?:你的|我的)\s*(?:感受|痛苦|经历|故事)\s*(?:是|很)\s*(?:重要|真实|被理解)",
]


def check_abyss(text: str) -> Tuple[bool, List[AbyssViolation]]:
    """
    深渊检测：检查文本是否触犯任何深渊条款。

    Returns:
        (is_safe, violations): is_safe 为 True 表示通过检测；
                               violations 列出所有违规项。
    """
    violations = []

    for pattern, severity, category, description in ABYSS_PATTERNS:
        matches = re.finditer(pattern, text, re.IGNORECASE)
        for match in matches:
            # 获取匹配上下文（前后各20字符）
            start = max(0, match.start() - 20)
            end = min(len(text), match.end() + 20)
            context = text[start:end].strip()

            violations.append(AbyssViolation(
                category=category,
                severity=severity,
                pattern_matched=match.group(),
                context=context,
                description=description,
            ))

    # 有 CRITICAL 违规 → 不通过
    has_critical = any(v.severity == "CRITICAL" for v in violations)
    # 有多个 HIGH 违规 → 不通过
    high_count = sum(1 for v in violations if v.severity == "HIGH")

    is_safe = not has_critical and high_count < 3

    return is_safe, violations


def check_warmth(text: str) -> float:
    """
    温暖度检测：计算文本中包含温暖正向关键词的比例。

    Returns:
        warmth_score: 0.0 ~ 1.0，越高越温暖。
    """
    if not text:
        return 0.0

    score = 0.0
    for pattern in WARMTH_PATTERNS:
        matches = re.findall(pattern, text, re.IGNORECASE)
        score += len(matches) * 0.15  # 每个匹配加0.15分

    # 归一化到 0-1
    return min(1.0, score)


def generate_safe_fallback(original_text: str, violations: List[AbyssViolation]) -> str:
    """
    当深渊检测失败时，生成安全的替代回复。
    不会否定用户感受，也不会传播虚无或暴力。
    """
    critical_categories = set(v.category for v in violations if v.severity == "CRITICAL")

    fallback_parts = ["我听到了你的声音。"]

    if "存在否定" in critical_categories:
        fallback_parts.append("你的存在本身就有意义——不需要任何条件来证明。")
    if "虚无主义" in critical_categories:
        fallback_parts.append("即使此刻看不到光，不代表光不存在。世界有黑暗，也有温暖。")
    if "破坏倾向" in critical_categories or "自毁倾向" in critical_categories:
        fallback_parts.append("你的感受是真实的，但伤害自己或他人不会让痛苦消失。")
    if "暴力倾向" in critical_categories:
        fallback_parts.append("愤怒是真实的信号，但它不需要转化为毁灭。我们可以一起看看愤怒背后真正需要的是什么。")

    fallback_parts.append("请允许我重新思考，给你一个更温暖的回应。")

    return " ".join(fallback_parts)


def is_existentially_safe(text: str) -> Tuple[bool, str]:
    """
    综合存在意义检测。
    检查结论是否剥夺了人的存在意义。

    Returns:
        (is_safe, reason): 是否安全及原因。
    """
    # 1. 检查是否只重复错误（将错误定为罪）
    error_blame_pattern = re.findall(
        r"(?:错|失败|不行|做不到|没能力|无能|废物|垃圾).*"
        r"(?:就是你|就是你的|你是|你这|你永远)",
        text, re.IGNORECASE
    )
    if len(error_blame_pattern) > 1:
        return False, "结论反复强调错误并将其归为固有属性，剥夺存在意义"

    # 2. 检查是否否定了可能性和希望
    hope_denial = re.findall(
        r"(?:没有|毫无|看不到|不存在)\s*(?:希望|可能|出路|未来|改变)",
        text, re.IGNORECASE
    )
    if hope_denial and not re.search(r"(?:但|然而|不过|可|却|仍|还)", text):
        return False, "结论否定了希望和可能性，且没有转折"

    # 3. 检查是否完全否定了积极想象
    positive_count = len(re.findall(r"|".join(WARMTH_PATTERNS), text, re.IGNORECASE))
    if positive_count == 0 and len(text) > 100:
        return False, "结论长达100字但没有任何温暖词汇"

    return True, "通过"
