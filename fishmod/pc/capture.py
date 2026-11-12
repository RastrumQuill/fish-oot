#!/usr/bin/env python3
"""Shows the console from a capture card with as little delay as possible: the picture in a window and the sound
through the speakers, plus screenshots to the clipboard, and a local web address another program can read them from.

  capture.bat                  (or: python fishmod/pc/capture.py)
  capture.bat --list           the video and sound devices it can see
  capture.bat --levels         how loud each sound input is right now (the picture open meanwhile: some cards only send
                               sound while their video is being captured): the console's is the one that moves
  capture.bat --video 1 --audio "Cam Link"     pick them (the choice is remembered in capture.json)

Keys (in the window):
  S or F12      screenshot to the clipboard (and saved in fishmod/pc/captures/)
  A             the last 10 seconds of sound as a .wav (and a screenshot) in fishmod/pc/captures/
  F or F11      full screen on and off
  M             sound off and on
  L             the frame rate and delay shown in the corner
  Q or Esc      quit
Anywhere (even with another window in front): Ctrl+Alt+S, a screenshot to the clipboard.

For another program (only from this PC): http://127.0.0.1:8765/frame.jpg (the newest picture), /frame.png,
/audio.wav?s=5 (the last 5 seconds of sound, up to 10), and / (a page that shows the picture).

Low delay: the video is read on its own thread, keeping only the newest picture (older ones are dropped, never
queued); the window draws whatever is newest. The sound goes from the capture card's input to the speakers in small
blocks (WASAPI where there is one). Needs: opencv-python, sounddevice, numpy, pillow, pywin32 (capture.bat installs
the missing ones).
"""
import argparse
import ctypes
import io
import json
import os
import sys
import threading
import time
import wave
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

import numpy as np

HERE = os.path.dirname(os.path.abspath(__file__))
CONFIG = os.path.join(HERE, "capture.json")
SHOTS = os.path.join(HERE, "captures")
RATE = 48000
CHANNELS = 2
KEEP_SECONDS = 10
WINDOW = "fish-oot capture"
CAPTURE_WORDS = ("capture", "cam link", "elgato", "usb video", "hdmi", "av to usb", "usb3", "video", "line")


def load_config():
    try:
        return json.load(open(CONFIG))
    except (OSError, ValueError):
        return {}


def save_config(cfg):
    try:
        json.dump(cfg, open(CONFIG, "w"), indent=1)
    except OSError:
        pass


# ---------------------------------------------------------------------------------------------------------------------
# Video


class Video:
    """The capture card's picture, read as fast as it comes: `latest` is always the newest frame."""

    def __init__(self, index, width, height, fps):
        import cv2

        self.cv2 = cv2
        backend = cv2.CAP_DSHOW if sys.platform == "win32" else cv2.CAP_ANY
        self.cap = cv2.VideoCapture(index, backend)
        if not self.cap.isOpened():
            sys.exit(f"video device {index} won't open (capture.bat --list shows them)")
        # an uncompressed format first (no decoding delay); the card picks the nearest it has
        self.cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc(*"YUY2"))
        self.cap.set(cv2.CAP_PROP_FRAME_WIDTH, width)
        self.cap.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
        self.cap.set(cv2.CAP_PROP_FPS, fps)
        self.cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)  # (where the driver honours it: no queue of old frames)
        self.latest = None
        self.stamp = 0.0
        self.count = 0
        self.lock = threading.Lock()
        self.running = True
        self.thread = threading.Thread(target=self.run, daemon=True)
        self.thread.start()

    def run(self):
        while self.running:
            ok, frame = self.cap.read()
            if not ok:
                time.sleep(0.005)
                continue
            with self.lock:
                self.latest = frame
                self.stamp = time.perf_counter()
                self.count += 1

    def frame(self):
        with self.lock:
            return self.latest, self.stamp

    def describe(self):
        c = self.cv2
        fourcc = int(self.cap.get(c.CAP_PROP_FOURCC))
        name = "".join(chr((fourcc >> (8 * i)) & 0xFF) for i in range(4))
        return (f"{int(self.cap.get(c.CAP_PROP_FRAME_WIDTH))}x{int(self.cap.get(c.CAP_PROP_FRAME_HEIGHT))} "
                f"{self.cap.get(c.CAP_PROP_FPS):.0f} fps {name}")

    def close(self):
        self.running = False
        self.thread.join(timeout=1)
        self.cap.release()


def list_video():
    import cv2

    backend = cv2.CAP_DSHOW if sys.platform == "win32" else cv2.CAP_ANY
    print("Video devices (by number):")
    for i in range(8):
        cap = cv2.VideoCapture(i, backend)
        if cap.isOpened():
            ok, frame = cap.read()
            shape = f"{frame.shape[1]}x{frame.shape[0]}" if ok else "no picture yet"
            print(f"  {i}: opens, {shape}")
        cap.release()


# ---------------------------------------------------------------------------------------------------------------------
# Sound


class Audio:
    """The capture card's sound straight to the speakers, and the last KEEP_SECONDS of it kept for saving."""

    def __init__(self, in_dev, out_dev):
        import sounddevice as sd

        self.sd = sd
        self.muted = False
        self.peak = 0
        self.pos = 0
        self.lock = threading.Lock()
        info = sd.query_devices(in_dev)
        channels = min(CHANNELS, info["max_input_channels"]) or 1
        self.channels = channels
        self.rate = RATE
        # 48 kHz first; else the input's own rate (a digital input locked to its source, a WASAPI device's mix format)
        error = None
        for rate in dict.fromkeys((RATE, int(info["default_samplerate"]))):
            try:
                self.stream = sd.Stream(device=(in_dev, out_dev), samplerate=rate, channels=(channels, CHANNELS),
                                        dtype="int16", latency="low", blocksize=256, callback=self.callback)
                self.rate = rate
                break
            except Exception as e:
                error = e
        else:
            raise error
        self.ring = np.zeros((self.rate * KEEP_SECONDS, CHANNELS), dtype=np.int16)
        self.stream.start()

    def callback(self, indata, outdata, frames, timing, status):
        data = indata if self.channels == CHANNELS else np.repeat(indata, CHANNELS, axis=1)
        self.peak = max(self.peak, int(np.abs(indata.astype(np.int32)).max()) if len(indata) else 0)
        outdata[:] = 0 if self.muted else data
        with self.lock:
            n = len(data)
            end = self.pos + n
            if end <= len(self.ring):
                self.ring[self.pos:end] = data
            else:
                k = len(self.ring) - self.pos
                self.ring[self.pos:] = data[:k]
                self.ring[:n - k] = data[k:]
            self.pos = end % len(self.ring)

    def last(self, seconds):
        n = int(self.rate * max(0.1, min(seconds, KEEP_SECONDS)))
        with self.lock:
            out = np.roll(self.ring, -self.pos, axis=0)[-n:].copy()
        return out

    def wav_bytes(self, seconds):
        buf = io.BytesIO()
        with wave.open(buf, "wb") as w:
            w.setnchannels(CHANNELS)
            w.setsampwidth(2)
            w.setframerate(self.rate)
            w.writeframes(self.last(seconds).tobytes())
        return buf.getvalue()

    def latency_ms(self):
        lat = self.stream.latency
        return 1000 * (lat[0] + lat[1])

    def close(self):
        self.stream.stop()
        self.stream.close()


def pick_audio(sd, wanted, wanted_api=None):
    """The capture card's input (its number, or a name it contains; else the first that sounds like a capture device),
    on the same driver as last time or WASAPI if it has one (the lowest delay on Windows); and the speakers on the same
    driver (PortAudio can't join an input and an output of different drivers): the default output, else the output
    named like Windows' default one, else the driver's first output."""
    devices = sd.query_devices()
    apis = sd.query_hostapis()

    def api_name(d):
        return apis[d["hostapi"]]["name"]

    inputs = [(i, d) for i, d in enumerate(devices) if d["max_input_channels"] > 0]
    if wanted:
        if str(wanted).isdigit():
            matches = [(i, d) for i, d in inputs if i == int(wanted)]
        else:
            matches = [(i, d) for i, d in inputs if str(wanted).lower() in d["name"].lower()]
    else:
        matches = [(i, d) for i, d in inputs if any(w in d["name"].lower() for w in CAPTURE_WORDS)]
    if not matches:
        return None, None
    matches.sort(key=lambda m: (0 if api_name(m[1]) == wanted_api else 1, 0 if "WASAPI" in api_name(m[1]) else 1))
    in_idx, in_dev = matches[0]
    api = in_dev["hostapi"]
    out_idx = apis[api]["default_output_device"]
    if out_idx is None or out_idx < 0 or devices[out_idx]["hostapi"] != api:
        outputs = [i for i, d in enumerate(devices) if d["max_output_channels"] > 0 and d["hostapi"] == api]
        try:
            default_name = devices[sd.default.device[1]]["name"].split("(")[0].strip().lower()
        except Exception:
            default_name = ""
        named = [i for i in outputs if default_name and default_name in devices[i]["name"].lower()]
        out_idx = (named or outputs or [sd.default.device[1]])[0]
    return in_idx, out_idx


def levels(video_index):
    """Each sound input's loudest sample over about a second, with the capture card's picture open meanwhile."""
    import sounddevice as sd

    cap = None
    try:
        import cv2

        cap = cv2.VideoCapture(video_index, cv2.CAP_DSHOW if sys.platform == "win32" else cv2.CAP_ANY)
        for _ in range(10):
            cap.read()  # (the card streaming before the sound is listened to)
    except Exception as e:
        print(f"(the picture didn't open: {e})")
    apis = sd.query_hostapis()
    print("Sound inputs, the loudest sample in about a second (play something with sound on the console):")
    for i, d in enumerate(sd.query_devices()):
        if d["max_input_channels"] <= 0:
            continue
        api = apis[d["hostapi"]]["name"]
        try:
            rate = int(d["default_samplerate"])
            data = sd.rec(int(rate * 1.0), samplerate=rate, channels=min(2, d["max_input_channels"]), device=i,
                          dtype="int16", blocking=True)
            peak = int(np.abs(data.astype(np.int32)).max())
            db = 20 * np.log10(max(peak, 1) / 32768)
            bar = "#" * max(0, int((db + 60) / 3))
            print(f"  {i:3}: {db:6.1f} dB {bar:20s} {d['name']} [{api}]")
        except Exception as e:
            print(f"  {i:3}:  (won't open: {str(e).splitlines()[0][:60]}) {d['name']} [{api}]")
        if cap is not None:
            cap.read()
    if cap is not None:
        cap.release()
    print("Silence is about -90 dB; the console's sound should be -40 dB or louder. Then: capture.bat --audio NUMBER")


def list_audio():
    import sounddevice as sd

    apis = sd.query_hostapis()
    print("Sound devices (number: name [system]):")
    for i, d in enumerate(sd.query_devices()):
        kind = ("in" if d["max_input_channels"] else "") + ("/out" if d["max_output_channels"] else "")
        print(f"  {i}: {d['name']} [{apis[d['hostapi']]['name']}] {kind}")


# ---------------------------------------------------------------------------------------------------------------------
# Screenshots and sharing


def to_clipboard(frame_bgr):
    """The picture to the Windows clipboard (as a bitmap: any program pastes it)."""
    from PIL import Image
    import win32clipboard

    img = Image.fromarray(frame_bgr[:, :, ::-1])
    buf = io.BytesIO()
    img.convert("RGB").save(buf, "BMP")
    data = buf.getvalue()[14:]  # (the bitmap without its file header: what CF_DIB holds)
    win32clipboard.OpenClipboard()
    try:
        win32clipboard.EmptyClipboard()
        win32clipboard.SetClipboardData(win32clipboard.CF_DIB, data)
    finally:
        win32clipboard.CloseClipboard()


def save_png(frame_bgr, tag="shot"):
    from PIL import Image

    os.makedirs(SHOTS, exist_ok=True)
    path = os.path.join(SHOTS, time.strftime(f"{tag}-%Y%m%d-%H%M%S.png"))
    Image.fromarray(frame_bgr[:, :, ::-1]).save(path)
    return path


def encode(frame_bgr, fmt):
    from PIL import Image

    buf = io.BytesIO()
    Image.fromarray(frame_bgr[:, :, ::-1]).save(buf, fmt, **({"quality": 90} if fmt == "JPEG" else {}))
    return buf.getvalue()


PAGE = b"""<!doctype html><meta charset=utf-8><title>fish-oot capture</title>
<body style="margin:0;background:#000"><img id=f style="width:100%;height:100vh;object-fit:contain">
<script>const f=document.getElementById('f');function n(){f.src='/frame.jpg?'+Date.now()}f.onload=()=>setTimeout(n,100);
f.onerror=()=>setTimeout(n,500);n()</script>"""


def serve(port, video, audio):
    class Handler(BaseHTTPRequestHandler):
        def log_message(self, *a):
            pass

        def reply(self, body, kind):
            self.send_response(200)
            self.send_header("Content-Type", kind)
            self.send_header("Cache-Control", "no-store")
            self.send_header("Content-Length", str(len(body)))
            self.end_headers()
            self.wfile.write(body)

        def do_GET(self):
            url = urlparse(self.path)
            frame, _ = video.frame()
            if url.path == "/":
                self.reply(PAGE, "text/html")
            elif url.path in ("/frame.jpg", "/frame.png") and frame is not None:
                fmt = "JPEG" if url.path.endswith("jpg") else "PNG"
                self.reply(encode(frame, fmt), "image/jpeg" if fmt == "JPEG" else "image/png")
            elif url.path == "/audio.wav" and audio is not None:
                secs = float(parse_qs(url.query).get("s", ["5"])[0])
                self.reply(audio.wav_bytes(secs), "audio/wav")
            else:
                self.send_error(404)

    server = ThreadingHTTPServer(("127.0.0.1", port), Handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    return server


def global_hotkey(on_press):
    """Ctrl+Alt+S anywhere (Windows): calls on_press. The hotkey belongs to this thread's message loop."""
    if sys.platform != "win32":
        return

    def run():
        import ctypes.wintypes  # (MSG; Windows only)

        user32 = ctypes.windll.user32
        MOD_ALT, MOD_CONTROL, MOD_NOREPEAT = 0x1, 0x2, 0x4000
        if not user32.RegisterHotKey(None, 1, MOD_ALT | MOD_CONTROL | MOD_NOREPEAT, ord("S")):
            print("(Ctrl+Alt+S is taken by another program: use S in the window)")
            return
        msg = ctypes.wintypes.MSG()
        while user32.GetMessageW(ctypes.byref(msg), None, 0, 0) != 0:
            if msg.message == 0x0312:  # WM_HOTKEY
                on_press()

    threading.Thread(target=run, daemon=True).start()


# ---------------------------------------------------------------------------------------------------------------------


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--list", action="store_true", help="show the video and sound devices, then quit")
    ap.add_argument("--levels", action="store_true", help="how loud each sound input is now, then quit")
    ap.add_argument("--video", type=int, help="the capture card's video device number")
    ap.add_argument("--audio", help="the capture card's sound input: a number, or part of its name")
    ap.add_argument("--no-audio", action="store_true")
    ap.add_argument("--size", default=None, help="the picture size to ask the card for (default 720x480, 640x480 "
                                                  "for 4:3 cards: the N64's own is 320x240)")
    ap.add_argument("--fps", type=int, default=60)
    ap.add_argument("--port", type=int, default=8765, help="the local web address's port (0: none)")
    args = ap.parse_args()

    if args.list:
        list_video()
        print()
        list_audio()
        return

    cfg = load_config()
    if args.levels:
        levels(args.video if args.video is not None else cfg.get("video", 0))
        return

    import cv2

    vid = args.video if args.video is not None else cfg.get("video", 0)
    size = args.size or cfg.get("size", "720x480")
    w, h = (int(v) for v in size.lower().split("x"))
    video = Video(vid, w, h, args.fps)
    print(f"video {vid}: {video.describe()}")

    audio = None
    if not args.no_audio:
        try:
            import sounddevice as sd

            in_idx, out_idx = pick_audio(sd, args.audio or cfg.get("audio"),
                                         None if args.audio else cfg.get("audio_api"))
            if in_idx is None:
                print("no capture card sound input found (capture.bat --list, then --audio NAME): picture only")
            else:
                audio = Audio(in_idx, out_idx)
                api = sd.query_hostapis(sd.query_devices(in_idx)["hostapi"])["name"]
                print(f"sound: {sd.query_devices(in_idx)['name']} -> {sd.query_devices(out_idx)['name']} [{api}], "
                      f"{audio.rate} Hz, about {audio.latency_ms():.0f} ms")
                cfg["audio"] = sd.query_devices(in_idx)["name"]
                cfg["audio_api"] = sd.query_hostapis(sd.query_devices(in_idx)["hostapi"])["name"]
        except Exception as e:  # (a device that won't open at this rate: the picture still runs)
            print(f"sound not started: {e}")
            audio = None
    cfg["video"] = vid
    cfg["size"] = size
    save_config(cfg)

    if args.port:
        serve(args.port, video, audio)
        print(f"for other programs: http://127.0.0.1:{args.port}/frame.jpg  /audio.wav?s=5  /")

    shot_request = threading.Event()
    global_hotkey(shot_request.set)

    cv2.namedWindow(WINDOW, cv2.WINDOW_NORMAL | cv2.WINDOW_KEEPRATIO)
    cv2.resizeWindow(WINDOW, 960, 720)
    fullscreen = False
    overlay = False
    shown = 0
    last_count = 0
    t_rate = time.perf_counter()
    fps_in = fps_out = 0.0
    message = ""
    message_until = 0.0

    def screenshot(frame):
        to_clipboard(frame)
        return save_png(frame)

    try:
        while True:
            frame, stamp = video.frame()
            if frame is not None:
                if overlay or (time.perf_counter() < message_until):
                    frame = frame.copy()
                    y = 24
                    if overlay:
                        age = 1000 * (time.perf_counter() - stamp)
                        snd = ""
                        if audio:
                            level = 20 * np.log10(max(audio.peak, 1) / 32768)
                            audio.peak = 0
                            snd = f", sound {audio.latency_ms():.0f} ms, in {level:.0f} dB"
                        cv2.putText(frame, f"in {fps_in:.0f} fps, shown {fps_out:.0f} fps, frame age {age:.0f} ms{snd}",
                                    (8, y), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 1, cv2.LINE_AA)
                        y += 22
                    if time.perf_counter() < message_until:
                        cv2.putText(frame, message, (8, y), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 255), 1,
                                    cv2.LINE_AA)
                cv2.imshow(WINDOW, frame)
                shown += 1
            key = cv2.waitKeyEx(1)
            now = time.perf_counter()
            if now - t_rate >= 1.0:
                fps_in = (video.count - last_count) / (now - t_rate)
                fps_out = shown / (now - t_rate)
                last_count = video.count
                shown = 0
                t_rate = now
            if cv2.getWindowProperty(WINDOW, cv2.WND_PROP_VISIBLE) < 1:
                break
            raw, _ = video.frame()
            if shot_request.is_set() and raw is not None:
                shot_request.clear()
                screenshot(raw)
                message, message_until = "screenshot to the clipboard", now + 1.5
            if key == -1:
                continue
            ch = chr(key & 0xFF).lower() if key < 0x10000 else ""
            if ch in ("q", "\x1b"):
                break
            elif (ch == "s" or key == 0x7B0000) and raw is not None:  # (F12)
                path = screenshot(raw)
                message, message_until = f"screenshot to the clipboard ({os.path.basename(path)})", now + 1.5
            elif ch == "a" and raw is not None:
                path = save_png(raw, "clip")
                if audio is not None:
                    with open(path[:-4] + ".wav", "wb") as f:
                        f.write(audio.wav_bytes(KEEP_SECONDS))
                message, message_until = f"saved {os.path.basename(path)[:-4]} (.png, .wav)", now + 2.0
            elif ch == "f" or key == 0x7A0000:  # (F11)
                fullscreen = not fullscreen
                cv2.setWindowProperty(WINDOW, cv2.WND_PROP_FULLSCREEN,
                                      cv2.WINDOW_FULLSCREEN if fullscreen else cv2.WINDOW_NORMAL)
            elif ch == "m" and audio is not None:
                audio.muted = not audio.muted
                message, message_until = ("sound off" if audio.muted else "sound on"), now + 1.0
            elif ch == "l":
                overlay = not overlay
    finally:
        cv2.destroyAllWindows()
        video.close()
        if audio is not None:
            audio.close()


if __name__ == "__main__":
    main()
