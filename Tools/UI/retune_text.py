"""영역을 고친 뒤 글자를 다시 맞추는 한 바퀴.

왜 한 바퀴인가
--------------
글자 하나를 제대로 놓으려면 다섯 군데가 순서대로 맞아야 한다.

    1 fit_title_textbox   그어 둔 영역만큼 글자칸을 틀 속에 맞춘다
    2 export_text_style   판의 지금 모습(글꼴·크기·칸)을 뽑는다
    3 fit_text            그 칸에 들어가는 가장 큰 크기를 잰다
    4 center_all_text     가운데로 맞추고 그 크기를 넣는다
    5 export + refresh    뽑아 둔 값과 tryon.json 을 판에 맞춰 씻는다

순서가 어긋나면 옛 값으로 재게 된다. 실제로 칸을 41px 로 키워 놓고 옛
28px 로 재서 13pt 를 내놓은 적이 있다. 그래서 손으로 돌리지 않는다.

    python Tools/UI/retune_text.py
"""

import subprocess
import sys
from pathlib import Path

ROOT = Path("D:/UnrealProjects/P_RD_develop_20260803")
TOOLS = ROOT / "Tools/UI"
UE = Path("C:/Program Files/Epic Games/UE_5.7/Engine/Binaries/Win64/"
          "UnrealEditor-Cmd.exe")
PROJECT = ROOT / "P_RD.uproject"


def unreal(script, label):
    print(f"  [{label}] {script}")
    done = subprocess.run(
        [str(UE), str(PROJECT), "-run=pythonscript",
         f"-script={(TOOLS / script).as_posix()}",
         "-unattended", "-nosplash", "-nopause"],
        capture_output=True, text=True, errors="replace")
    bad = [line for line in (done.stdout or "").splitlines()
           if "LogPython: Error" in line or "Traceback" in line]
    for line in bad[:6]:
        print(f"      {line.strip()[:160]}")
    return not bad


def plain(script, label):
    print(f"  [{label}] {script}")
    done = subprocess.run([sys.executable, str(TOOLS / script)],
                          capture_output=True, text=True, errors="replace")
    for line in (done.stdout or "").splitlines()[:8]:
        print(f"      {line}")
    return done.returncode == 0


def extract(script, label):
    """WBP 편집기 추출을 다시 돌린다. 자리·그림이 여기서 나온다."""
    print(f"  [{label}] {script}")
    done = subprocess.run(
        [str(UE), str(PROJECT), "-run=pythonscript", f"-script={script}",
         "-unattended", "-nop4", "-nosplash", "-nullrhi",
         "-NoCompile", "-NoCompileEditor"],
        capture_output=True, text=True, errors="replace")
    bad = [line for line in (done.stdout or "").splitlines()
           if "LogPython: Error" in line or "Traceback" in line]
    for line in bad[:6]:
        print(f"      {line.strip()[:160]}")
    return not bad


EXPORTER = "D:/UnrealProjects_WBP_Editor/unreal/export_current_develop.py"

# 순서가 곧 규칙이다.
#
# 판을 고친 뒤에는 **추출을 다시 돌려야** 갤러리가 바뀐 것을 본다. 이걸
# 빼먹어서 자리를 357x86 으로 키우고 크기를 37pt 로 올려 놓고도 갤러리는
# 계속 옛 25pt 짜리를 그렸다. 사람이 "왜 확인이 안 되냐" 고 물은 것이 이것이다.
STEPS = [
    (plain, "fit_title_buttons.py", "1/9 타이틀 단추 크기 재기"),
    (unreal, "fit_title_textbox.py", "2/9 글자칸을 그어 둔 영역에 맞춤"),
    (unreal, "export_text_style.py", "3/9 판에서 지금 모습 뽑기"),
    (plain, "fit_text.py", "4/9 칸에 들어가는 크기 재기"),
    (unreal, "center_all_text.py", "5/9 가운데 맞추고 크기 넣기"),
    (extract, EXPORTER, "6/9 WBP 추출 다시 (자리·그림)"),
    (unreal, "export_text_style.py", "7/9 바뀐 글자 모습 뽑기"),
    (unreal, "export_widget_source.py", "8/9 물린 그림 뽑기"),
    (plain, "refresh_tryon_text.py", "9/9 tryon.json 크기 맞추기"),
    (plain, "build_current_gallery.py", "갤러리 다시 굽기"),
]


def main():
    if not UE.is_file():
        print(f"언리얼을 못 찾음: {UE}")
        return 1
    for how, script, label in STEPS:
        print(label)
        if not how(script, label.split()[0]):
            print(f"\n{script} 에서 멈춤 -- 위 오류를 보고 고친 뒤 다시 돌리면 된다.")
            return 1
    print("\n끝. 게임을 다시 띄우면 보인다.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
