"""Guard the MQB-only download policy and preserve unrelated author notices."""
import re
import subprocess
from pathlib import Path

REPO = Path(__file__).resolve().parents[1]
BASELINE = "33244007ebba975372f1c84c8596e188e568aae8"


def section(text, heading):
    match = re.search(r"^### " + re.escape(heading) + r"\n(.*?)(?=^### |^## |\Z)", text, re.M | re.S)
    assert match, heading
    return match.group(1)


def main():
    before = subprocess.check_output(["git", "show", BASELINE + ":README.md"], cwd=REPO).decode()
    after = (REPO / "README.md").read_text()
    for heading in ("可选付费支持", "开源与许可", "安全提示与免责声明",
                    "Optional paid support", "Open-source notice", "Safety and disclaimer"):
        assert section(before, heading) == section(after, heading), heading
    assert re.search(r"^\*\*作者 / Author:.*", before, re.M).group() == re.search(r"^\*\*作者 / Author:.*", after, re.M).group()
    assert "MIB_Toolbox_3.6.0.zip" not in after
    assert "#### 方法 B" not in after and "### Installation route B" not in after
    assert "791 40 FPS" in after
    assert not re.search(r"\bIDR\d*\b|每\s*5\s*帧", after, re.I)
    assert not (REPO / "tools/Build-ToolboxUpdate.ps1").exists()
    assert not (REPO / "tools/Build-791Idr5Release.ps1").exists()
    builder = (REPO / "tools/Build-MQBRelease.ps1").read_text(encoding="utf-8-sig")
    assert "MIB_Toolbox" not in builder
    notes = (REPO / "releases/toolbox-plaintext-v1.2.1.md").read_text()
    assert "## 中文" in notes and "## English" in notes and "omonob" in notes
    assert not re.search(r"\bIDR\d*\b|每\s*5\s*帧", notes, re.I)
    print("MQB_ONLY_DOC_POLICY_AND_PROTECTED_NOTICES_OK")


if __name__ == "__main__":
    main()
