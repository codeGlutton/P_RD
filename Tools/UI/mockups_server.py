"""Static server for the catalogue that can also save edits to disk.

왜 필요한가
-----------
분류와 칸 자리는 브라우저 localStorage 에만 있었다. 브라우저를 지우거나 다른
기기에서 열면 사라지고, 내가 값을 읽으려면 매번 페이지에서 긁어와야 했다.
실제로 한 번은 시험하다가 사람이 해 둔 것을 지웠다.

``python -m http.server`` 는 POST 를 안 받는다. 그래서 파일 주는 일은 그대로
하고 POST /save 만 더한 얇은 서버를 둔다.

    POST /save   {"cats": {...}, "rects": {...}}  -> cats_user.json / rects_user.json

받는 것은 이 폴더 안의 정해진 두 파일뿐이다. 경로를 인자로 받지 않으므로
엉뚱한 자리에 쓸 길이 없다.

쓰기:
    python Tools/UI/mockups_server.py [포트]
"""

import json
import sys
import threading
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path

HERE = Path(__file__).resolve().parent / "mockups"
PORT = int(sys.argv[1]) if len(sys.argv) > 1 else 8766
# 저장을 허락하는 이름. 여기 없는 이름은 무시한다.
ALLOWED = {"cats": "cats_user.json", "rects": "rects_user.json",
           "picks": "gen_picks.json", "tryon": "tryon.json",
           "summary": "summary_layout.json"}


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(HERE), **kwargs)

    def do_POST(self):
        if self.path.rstrip("/") != "/save":
            self.send_error(404)
            return
        try:
            length = int(self.headers.get("Content-Length", "0"))
            payload = json.loads(self.rfile.read(length).decode("utf-8"))
        except Exception:  # noqa: BLE001
            self.send_error(400, "bad json")
            return

        wrote = {}
        for key, filename in ALLOWED.items():
            if key not in payload:
                continue
            body = json.dumps(payload[key], ensure_ascii=False, indent=1)
            target = HERE / filename
            # 덮기 전에 앞판을 남긴다.
            #
            # 한 번은 "이 화면 전부" 를 잘못 눌러 맞춰 둔 것이 통째로 날아갔다.
            # 브라우저에는 되돌릴 길이 없고 파일은 이미 덮인 뒤였다. 여기서
            # 한 판만 챙겨 두면 그때 그 값을 그대로 되살릴 수 있다.
            if target.is_file():
                before = target.read_text(encoding="utf-8")
                if before.strip() and before != body:
                    (HERE / f"{target.stem}.prev.json").write_text(
                        before, encoding="utf-8")
            target.write_text(body, encoding="utf-8")
            wrote[filename] = len(payload[key])

        answer = json.dumps(wrote, ensure_ascii=False).encode("utf-8")
        self.send_response(200)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(answer)))
        self.end_headers()
        self.wfile.write(answer)

    def end_headers(self):
        # 고친 페이지가 캐시에 걸려 옛 것이 뜨는 일이 잦았다. 캐시를 끈다.
        self.send_header("Cache-Control", "no-store")
        super().end_headers()

    def log_message(self, *args):
        pass


def watch():
    """만드는 중인 것을 genstatus.json 에 계속 적는다.

    전에는 이걸 따로 띄웠는데, 포트 충돌을 정리하려고 python 프로세스를
    죽일 때마다 같이 죽었다. 사람이 "왜 자꾸 꺼지냐" 고 물은 것이 이것이다.
    서버 안에 두면 서버가 사는 동안 같이 산다 -- 띄우는 것을 잊을 수도 없다.
    """
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    import time

    import gen_status

    while True:
        try:
            state = {"at": int(time.time()), "downloads": gen_status.downloads(),
                     "queue": gen_status.queue_state(), "images": gen_status.publish()}
            gen_status.STATUS.write_text(
                json.dumps(state, ensure_ascii=False), encoding="utf-8")
        except Exception:  # noqa: BLE001
            # 이 곁다리 때문에 서버가 죽으면 안 된다. 다음 바퀴에 다시 해 본다.
            pass
        time.sleep(5)


if __name__ == "__main__":
    threading.Thread(target=watch, daemon=True).start()
    print(f"http://127.0.0.1:{PORT}/assets.html  ({HERE})")
    print(f"http://127.0.0.1:{PORT}/gen.html     만드는 중인 것")
    ThreadingHTTPServer(("127.0.0.1", PORT), Handler).serve_forever()
