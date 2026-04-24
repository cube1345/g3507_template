import argparse
import queue
import sys
import threading
import time
import tkinter as tk
from tkinter import ttk

import serial
from serial.tools import list_ports


# 把串口收到的 bytes 解码成可读字符串。
# 参数类型标注 `raw: bytes` 表示 raw 应该是字节串；
# 返回类型标注 `-> str` 表示函数返回字符串。
def decode_line(raw: bytes) -> str:
    # 依次尝试 utf-8 / gbk，适配常见串口输出编码。
    for enc in ("utf-8", "gbk"):
        try:
            # decode 后再 strip() 去掉两端空白和 \r\n
            return raw.decode(enc).strip()
        except UnicodeDecodeError:
            # 当前编码失败就继续试下一个编码
            pass
    # 两种编码都失败时，忽略非法字节，尽量保住可读内容
    return raw.decode("utf-8", errors="ignore").strip()


# 根据是否看到 INIT 标记，把每行日志打上 OLD/NEW 标签。
# 返回值是一个二元组: (输出文本, 是否已经看到 marker)。
def classify_line(line: str, marker: str, seen_marker: bool) -> tuple[str, bool]:
    if marker in line:
        return (f"[NEW_FW] marker detected: {line}", True)
    if seen_marker:
        return (f"[NEW] {line}", True)
    return (f"[OLD] {line}", False)


# CLI 模式：在终端中打印串口日志。
# - port: 串口号，例如 COM7
# - baud: 波特率
# - idle_timeout: 无数据超时退出秒数（0 表示不超时）
# - wait_init_ok: 是否等到 marker 后才认为是新程序日志
# - marker: 识别新程序的标记字符串
def run_cli(port: str, baud: int, idle_timeout: int, wait_init_ok: bool, marker: str) -> int:
    # 枚举系统可见串口，仅做提示，不阻止继续尝试连接
    available = [p.device for p in list_ports.comports()]
    if port not in available:
        print(f"[INFO] Requested port {port} is not in current visible list.")
        print(f"[INFO] Visible ports: {available if available else 'none'}")

    # 先定义为 None，方便 finally 中安全判断
    ser = None
    # 如果不等待 INIT_OK，那么一开始就把数据当 NEW
    seen_marker = not wait_init_ok
    # 记录最近一次收到数据的时间，用于 idle timeout
    last_data_time = time.time()
    try:
        print(f"[CONNECT] {port} @ {baud}")
        # timeout=1 表示 readline 最多阻塞 1 秒
        ser = serial.Serial(port, baud, timeout=1)
        if wait_init_ok:
            print(f"[STATE] waiting for marker '{marker}' to identify new firmware logs.")
        else:
            print("[STATE] marker wait disabled.")
        print("[START] listening... press Ctrl+C to stop.")
        if idle_timeout > 0:
            print(f"[INFO] auto-exit after {idle_timeout}s without data.")

        while True:
            # readline() 返回 bytes；如果一行都没读到，通常是 b''
            raw = ser.readline()
            if raw:
                line = decode_line(raw)
                # 用状态机给当前行打标签，并更新是否已看到 marker
                out, seen_marker = classify_line(line, marker, seen_marker)
                print(out)
                last_data_time = time.time()
            elif idle_timeout > 0 and (time.time() - last_data_time) > idle_timeout:
                print(f"[END] no data for {idle_timeout}s, exiting.")
                break
    except serial.SerialException as exc:
        print(f"[ERROR] serial exception: {exc}")
        return 1
    except KeyboardInterrupt:
        # Ctrl+C 走这里，属于正常退出
        print("\n[END] stopped by user.")
    finally:
        # finally 一定执行：无论正常退出、异常退出，都会尝试关闭串口
        if ser is not None and ser.is_open:
            ser.close()
            print("[CLOSE] serial connection closed.")

    return 0


# GUI 模式：Tk 窗口显示 + 终端同步打印。
# 注意：Tk 只能在主线程更新 UI，所以串口读取放到后台线程。
def run_gui(port: str, baud: int, wait_init_ok: bool, marker: str) -> int:
    try:
        # 创建 Tk 根窗口；若当前环境不支持 GUI 会抛 TclError
        root = tk.Tk()
    except tk.TclError as exc:
        print(f"[ERROR] failed to start Tk window: {exc}")
        return 1

    root.title(f"Serial Monitor - {port} @ {baud}")
    root.geometry("980x580")

    top = ttk.Frame(root, padding=10)
    top.pack(fill=tk.X)
    body = ttk.Frame(root, padding=(10, 0, 10, 10))
    body.pack(fill=tk.BOTH, expand=True)

    status_var = tk.StringVar(value="Preparing...")
    status_lbl = ttk.Label(top, textvariable=status_var)
    status_lbl.pack(side=tk.LEFT)

    text = tk.Text(body, wrap="none", font=("Consolas", 11))
    text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
    text.configure(state=tk.DISABLED)

    # 右侧滚动条
    yscroll = ttk.Scrollbar(body, orient=tk.VERTICAL, command=text.yview)
    text.configure(yscrollcommand=yscroll.set)
    yscroll.pack(side=tk.RIGHT, fill=tk.Y)

    # 线程安全队列：后台线程把消息放进队列，主线程定时取出并更新 UI
    # 队列元素格式是 (kind, payload)，kind 目前有 "line" / "status"
    q: queue.Queue[tuple[str, str]] = queue.Queue()
    # 关闭标志位，线程间共享
    stop_event = threading.Event()
    # 用字典包装，便于内部函数修改同一个引用
    ser_holder = {"ser": None}

    # 追加一行到 Tk 文本框。Text 设为 DISABLED 时不可写，所以要临时切到 NORMAL。
    def append_line(line: str) -> None:
        text.configure(state=tk.NORMAL)
        text.insert(tk.END, line + "\n")
        text.see(tk.END)
        text.configure(state=tk.DISABLED)

    # 串口读取线程：只做 I/O，不直接操作 Tk 组件。
    def reader_thread() -> None:
        seen_marker = not wait_init_ok
        try:
            ser = serial.Serial(port, baud, timeout=0.5)
            ser_holder["ser"] = ser
            q.put(("status", "Connected"))
            q.put(("line", f"[CONNECT] {port} @ {baud}"))
            if wait_init_ok:
                q.put(("line", f"[STATE] waiting for marker '{marker}'"))
            else:
                q.put(("line", "[STATE] marker wait disabled"))

            while not stop_event.is_set():
                raw = ser.readline()
                if raw:
                    line = decode_line(raw)
                    out, seen_marker = classify_line(line, marker, seen_marker)
                    # 把日志行交给主线程显示
                    q.put(("line", out))
        except serial.SerialException as exc:
            q.put(("line", f"[ERROR] serial exception: {exc}"))
            q.put(("status", "Connection failed"))
        finally:
            ser = ser_holder.get("ser")
            if ser is not None and ser.is_open:
                ser.close()
                q.put(("line", "[CLOSE] serial connection closed."))
            q.put(("status", "Disconnected"))

    # 主线程轮询队列并更新 UI。
    # root.after(80, poll_queue) 表示 80ms 后再次调用自己（定时器循环）。
    def poll_queue() -> None:
        try:
            while True:
                kind, payload = q.get_nowait()
                if kind == "line":
                    # 双通道显示：Tk + 终端
                    append_line(payload)
                    print(payload, flush=True)
                elif kind == "status":
                    status_var.set(payload)
        except queue.Empty:
            # 队列没数据是正常情况
            pass
        if not stop_event.is_set():
            root.after(80, poll_queue)

    # 点击关闭按钮或窗口右上角 X 时触发
    def on_close() -> None:
        stop_event.set()
        # 留一点时间给后台线程收尾（比如关闭串口）
        root.after(120, root.destroy)

    btn_frame = ttk.Frame(top)
    btn_frame.pack(side=tk.RIGHT)

    # 清空窗口中的历史日志
    def clear_text() -> None:
        text.configure(state=tk.NORMAL)
        text.delete("1.0", tk.END)
        text.configure(state=tk.DISABLED)

    ttk.Button(btn_frame, text="Clear", command=clear_text).pack(side=tk.LEFT, padx=(0, 6))
    ttk.Button(btn_frame, text="Close", command=on_close).pack(side=tk.LEFT)

    root.protocol("WM_DELETE_WINDOW", on_close)
    threading.Thread(target=reader_thread, daemon=True).start()
    poll_queue()
    root.mainloop()
    return 0


# 程序入口：解析命令行参数并选择 CLI / GUI 分支。
def main() -> int:
    parser = argparse.ArgumentParser(description="Serial monitor with CLI/Tk GUI and INIT marker gating.")
    parser.add_argument("--port", default="COM5", help="serial port, e.g. COM7")
    parser.add_argument("--baud", type=int, default=115200, help="baud rate")
    parser.add_argument("--idle-timeout", type=int, default=0, help="CLI mode idle timeout in seconds")
    parser.add_argument("--gui", action="store_true", help="force Tk GUI mode")
    parser.add_argument("--cli", action="store_true", help="CLI mode")
    parser.add_argument("--init-marker", default="INIT_OK", help="marker used to identify new firmware logs")
    parser.add_argument("--no-wait-init-ok", action="store_true", help="disable marker gating")
    # parse_args() 会把命令行参数转换成对象 args
    args = parser.parse_args()

    # no_wait_init_ok 是“关闭等待”的开关，所以这里取反得到 wait_init_ok
    wait_init_ok = not args.no_wait_init_ok
    if args.cli:
        return run_cli(args.port, args.baud, args.idle_timeout, wait_init_ok, args.init_marker)
    if args.gui or not args.cli:
        return run_gui(args.port, args.baud, wait_init_ok, args.init_marker)
    return run_cli(args.port, args.baud, args.idle_timeout, wait_init_ok, args.init_marker)


# Python 脚本标准入口写法：
# 只有直接运行该文件时才执行 main()；
# 如果被别的文件 import，则不会自动执行。
if __name__ == "__main__":
    sys.exit(main())
