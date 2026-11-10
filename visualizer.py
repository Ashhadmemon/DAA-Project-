import math
import os
import queue
import re
import shutil
import subprocess
import tempfile
import threading
import tkinter as tk
from tkinter import filedialog, messagebox, ttk

FOLDER = os.path.dirname(os.path.abspath(__file__))
STEP_LIMIT = 200         
SAMPLE_SIZE = 4000   



def get_exe(name):
    """Find closest_pair.exe / karatsuba.exe (compile it with g++ if it is missing)."""
    for candidate in (name + ".exe", name):
        path = os.path.join(FOLDER, candidate)
        if os.path.isfile(path):
            return path
    source = os.path.join(FOLDER, name + ".cpp")
    out = os.path.join(FOLDER, name + (".exe" if os.name == "nt" else ""))
    if os.path.isfile(source) and shutil.which("g++"):
        subprocess.run(["g++", "-O2", source, "-o", out], check=True)
        return out
    raise FileNotFoundError(name + ".exe not found in the folder of visualizer.py.\n"
                            "Compile it first:  g++ -O2 " + name + ".cpp -o " + name)


def run_cpp(program, path):
    """Run your program on one input file and return what it printed.
    It runs inside a temporary folder, so your csv / product files are never overwritten."""
    exe = get_exe(program)
    with tempfile.TemporaryDirectory() as tmp:
        shutil.copyfile(path, os.path.join(tmp, "input.txt"))
        flags = 0x08000000 if os.name == "nt" else 0          # do not flash a black console window
        result = subprocess.run([exe], input="input.txt\n", capture_output=True, text=True,
                                cwd=tmp, creationflags=flags)
    out = result.stdout.replace("input.txt", os.path.basename(path))
    out = re.sub(r"^Enter the input file name[^:]*:\s*", "", out)       # remove the question your program prints
    lines = [ln for ln in out.splitlines() if "saved in" not in ln]      # that file lives in the temp folder
    out = "\n".join(lines).strip()
    if not out or "Cannot open" in out:
        raise RuntimeError("The C++ program gave no result.\n" + out + result.stderr)
    return out


def parse_output(out):
    """'Time          : 950.1 ms'  ->  info['Time'] = '950.1 ms'"""
    info = {}
    for line in out.splitlines():
        key, sep, value = line.partition(":")
        if sep:
            info[key.strip()] = value.strip()
    return info


def detect_kind(path):
    """cp... = closest pair file, im... = integer multiplication file (otherwise look inside the file)."""
    base = os.path.basename(path).lower()
    if base.startswith("cp"):
        return "cp"
    if base.startswith("im"):
        return "im"
    with open(path) as f:
        f.readline()
        second = f.readline()
    return "cp" if len(second.split()) == 2 else "im"


def read_points(path):
    """Returns (n, points). For big files only about SAMPLE_SIZE points are kept (every k-th one)."""
    with open(path) as f:
        n = int(f.readline())
        skip = max(1, n // SAMPLE_SIZE) if n > STEP_LIMIT else 1
        pts = []
        for i, line in enumerate(f):
            if i % skip == 0:
                parts = line.split()
                if len(parts) >= 2:
                    pts.append((float(parts[0]), float(parts[1])))
    return n, pts


def read_numbers(path):
    with open(path) as f:
        parts = f.read().split()
    return parts[0], parts[1]


def list_input_files():
    names = [f for f in os.listdir(FOLDER)
             if re.fullmatch(r"(cp|im)\d+\.txt", f.lower())]
    return sorted(names)


#step by step working for small inputs
def closest_pair_steps(points):
    """Same idea as your C++ code: split at the middle, solve both halves,
    then check the strip around the dividing line. Every important moment is saved as a 'step'."""
    P = sorted(points)                      # sorted by x (then y)
    steps = []

    def d2(a, b):
        return (a[0] - b[0]) ** 2 + (a[1] - b[1]) ** 2

    def add(kind, lo, hi, xstar, d2best, pair, strip, text):
        delta = math.sqrt(d2best) if d2best is not None and d2best < 1e29 else None
        steps.append(dict(kind=kind, lo=lo, hi=hi, xstar=xstar, delta=delta,
                          pair=pair, strip=strip, text=text))

    def rec(lo, hi, depth):
        n = hi - lo
        if n <= 3:  # base case: brute force
            best = (1e30, None, None)
            for i in range(lo, hi):
                for j in range(i + 1, hi):
                    d = d2(P[i], P[j])
                    if d < best[0]:
                        best = (d, P[i], P[j])
            pair = (best[1], best[2]) if best[1] else None
            add("base", lo, hi, None, best[0], pair, [],
                f"Depth {depth}: only {n} points here, so just check every pair. "
                f"Best distance = {math.sqrt(best[0]):.3f}")
            return best

        mid = (lo + hi) // 2
        xstar = P[mid - 1][0]
        add("split", lo, hi, xstar, None, None, [],
            f"Depth {depth}: split these {n} points into a left half and a right half at x = {xstar:.2f}")
        left = rec(lo, mid, depth + 1)
        right = rec(mid, hi, depth + 1)
        best = left if left[0] <= right[0] else right
        add("merge", lo, hi, xstar, best[0], (best[1], best[2]), [],
            f"Depth {depth}: left half best = {math.sqrt(left[0]):.3f}, right half best = "
            f"{math.sqrt(right[0]):.3f}  ->  delta = {math.sqrt(best[0]):.3f}")

        # points close to the dividing line, in y-order
        strip = [p for p in P[lo:hi] if (p[0] - xstar) ** 2 < best[0]]
        strip.sort(key=lambda p: (p[1], p[0]))
        add("strip", lo, hi, xstar, best[0], (best[1], best[2]), strip,
            f"Depth {depth}: {len(strip)} points lie within delta of the dividing line. "
            f"A pair across the line can only be hiding here.")
        for i in range(len(strip)):
            for j in range(i + 1, min(len(strip), i + 16)):
                dy = strip[j][1] - strip[i][1]
                if dy * dy >= best[0]:
                    break
                d = d2(strip[i], strip[j])
                if d < best[0]:
                    best = (d, strip[i], strip[j])
                    add("best", lo, hi, xstar, best[0], (best[1], best[2]), strip,
                        f"Depth {depth}: a closer pair ACROSS the line was found! New distance = "
                        f"{math.sqrt(best[0]):.3f}")
        return best

    final = rec(0, len(P), 0)
    add("done", 0, len(P), None, final[0], (final[1], final[2]), [],
        f"DONE. Closest pair = ({final[1][0]:.3f}, {final[1][1]:.3f}) and "
        f"({final[2][0]:.3f}, {final[2][1]:.3f}),  distance = {math.sqrt(final[0]):.6f}")
    return steps, P, math.sqrt(final[0])


def karatsuba_steps(a, b, limit=16):
    """Karatsuba on Python integers. Numbers with <= `limit` digits use ordinary multiplication
    (your C++ code does the same, only with a bigger limit). Returns (list of step texts, product)."""
    steps = []

    def short(n):
        s = str(n)
        return s if len(s) <= 24 else f"{s[:10]}...{s[-10:]} ({len(s)} digits)"

    def rec(x, y, depth):
        ind = "    " * depth
        lx, ly = len(str(x)), len(str(y))
        if lx <= limit or ly <= limit:
            r = x * y
            steps.append(f"{ind}BASE   {short(x)} x {short(y)}\n"
                         f"{ind}       small numbers, ordinary multiplication = {short(r)}")
            return r
        m = (max(lx, ly) + 1) // 2
        p = 10 ** m
        x1, x0 = divmod(x, p)
        y1, y0 = divmod(y, p)
        steps.append(f"{ind}MULTIPLY   {short(x)} x {short(y)}\n"
                     f"{ind}   cut both numbers after {m} digits:\n"
                     f"{ind}   x1 = {short(x1)}   x0 = {short(x0)}\n"
                     f"{ind}   y1 = {short(y1)}   y0 = {short(y0)}")
        z2 = rec(x1, y1, depth + 1)                  # x1 * y1
        z0 = rec(x0, y0, depth + 1)                  # x0 * y0
        z1 = rec(x0 + x1, y0 + y1, depth + 1) - z2 - z0     # (x0+x1)(y0+y1) - z2 - z0
        r = z2 * p * p + z1 * p + z0
        steps.append(f"{ind}COMBINE   z2 = {short(z2)}   z0 = {short(z0)}   z1 = {short(z1)}\n"
                     f"{ind}   (z1 = (x0+x1)(y0+y1) - z2 - z0 : only 3 multiplications instead of 4)\n"
                     f"{ind}   result = z2*10^{2 * m} + z1*10^{m} + z0 = {short(r)}")
        return r

    product = rec(int(a), int(b), 0)
    steps.append(f"DONE.  The product has {len(str(product))} digits.\n{product}")
    return steps, product


#window
class App:
    def __init__(self, root):
        self.root = root
        root.title("DAA Project - Divide and Conquer Visualiser")
        root.geometry("1180x760")

        style = ttk.Style()
        style.theme_use("clam")
        style.configure("Run.TButton", font=("Segoe UI", 10, "bold"), foreground="white",
                        background="#2f6fdd", padding=6)
        style.map("Run.TButton", background=[("active", "#1f56b5"), ("disabled", "#9bb6e8")])

        # state
        self.mode = None            # "cp_steps", "cp_large", "im_steps", "im_large"
        self.steps = []
        self.idx = 0
        self.sortedP = []
        self.pts = []
        self.pair = None
        self.playing = False
        self.job = None
        self.queue = queue.Queue()

        #top bar
        top = ttk.Frame(root, padding=8)
        top.pack(fill="x")
        ttk.Label(top, text="Input file:", font=("Segoe UI", 10, "bold")).pack(side="left")
        files = list_input_files()
        self.fileVar = tk.StringVar(value="cp10.txt" if "cp10.txt" in files else (files[0] if files else ""))
        ttk.Combobox(top, textvariable=self.fileVar, values=files, width=30).pack(side="left", padx=6)
        ttk.Button(top, text="Browse...", command=self.browse).pack(side="left")
        self.runBtn = ttk.Button(top, text="Run", style="Run.TButton", command=self.on_run)
        self.runBtn.pack(side="left", padx=10)
        self.prog = ttk.Progressbar(top, mode="indeterminate", length=130)
        self.prog.pack(side="left")
        self.status = ttk.Label(top, text="")
        self.status.pack(side="left", padx=10)

        #middle: view (left) and result (right)
        mid = ttk.Frame(root)
        mid.pack(fill="both", expand=True, padx=8)
        self.left = ttk.Frame(mid)
        self.left.pack(side="left", fill="both", expand=True)
        self.canvas = tk.Canvas(self.left, bg="white", highlightthickness=1, highlightbackground="#cfd6df")
        self.text = tk.Text(self.left, font=("Consolas", 10), wrap="word", bg="#fbfbfd", state="disabled")
        self.text.tag_configure("now", background="#fff3b0")
        self.canvas.bind("<Configure>", lambda e: self.cp_draw())

        right = ttk.LabelFrame(mid, text="Result (from your C++ program)", padding=8, width=360)
        right.pack(side="right", fill="y", padx=(8, 0))
        right.pack_propagate(False)
        ttk.Label(right, text="Running time", font=("Segoe UI", 9)).pack(anchor="w")
        self.timeLbl = ttk.Label(right, text="-", font=("Segoe UI", 22, "bold"), foreground="#2f6fdd")
        self.timeLbl.pack(anchor="w")
        self.checkLbl = ttk.Label(right, text="", font=("Segoe UI", 11, "bold"))
        self.checkLbl.pack(anchor="w", pady=(0, 6))
        self.resText = tk.Text(right, font=("Consolas", 9), wrap="char", relief="flat", bg="#f4f6fa", height=20)
        self.resText.pack(fill="both", expand=True)

        #bottom: description + controls
        self.descLbl = ttk.Label(root, text="Pick a file and press Run.  Small files (up to 200 points / 200 digits) are "
                                            "shown step by step; large files show the result and the running time.",
                                 wraplength=1120, font=("Segoe UI", 11), padding=(10, 6))
        self.descLbl.pack(fill="x")
        bot = ttk.Frame(root, padding=(8, 0, 8, 8))
        bot.pack(fill="x")
        ttk.Button(bot, text="|<", width=4, command=lambda: self.goto(0)).pack(side="left")
        ttk.Button(bot, text="<  Prev", command=lambda: self.goto(self.idx - 1)).pack(side="left", padx=2)
        self.playBtn = ttk.Button(bot, text="Play", width=8, command=self.toggle_play)
        self.playBtn.pack(side="left", padx=2)
        ttk.Button(bot, text="Next  >", command=lambda: self.goto(self.idx + 1)).pack(side="left", padx=2)
        ttk.Button(bot, text=">|", width=4, command=lambda: self.goto(len(self.steps) - 1)).pack(side="left")
        self.slider = tk.Scale(bot, from_=0, to=0, orient="horizontal", showvalue=0, command=self.on_slider)
        self.slider.pack(side="left", fill="x", expand=True, padx=10)
        self.stepLbl = ttk.Label(bot, text="", width=16)
        self.stepLbl.pack(side="left")
        ttk.Label(bot, text="Speed").pack(side="left", padx=(10, 2))
        self.speed = tk.Scale(bot, from_=1, to=10, orient="horizontal", length=90, showvalue=0)
        self.speed.set(4)
        self.speed.pack(side="left")

        root.bind("<Left>", lambda e: self.goto(self.idx - 1))
        root.bind("<Right>", lambda e: self.goto(self.idx + 1))
        root.bind("<space>", lambda e: self.toggle_play())
        self.show_view("canvas")

    # file + run
    def browse(self):
        path = filedialog.askopenfilename(initialdir=FOLDER, filetypes=[("Text files", "*.txt")])
        if path:
            self.fileVar.set(path)

    def resolve_path(self):
        name = self.fileVar.get().strip()
        return name if os.path.isabs(name) else os.path.join(FOLDER, name)

    def on_run(self):
        path = self.resolve_path()
        if not os.path.isfile(path):
            messagebox.showerror("File not found", path + "\n\nRun generator.exe to create the big files again.")
            return
        self.stop_play()
        self.runBtn.state(["disabled"])
        self.prog.start(12)
        self.status.config(text="Running your C++ program (big files take a few seconds)...")
        threading.Thread(target=self.worker, args=(path,), daemon=True).start()
        self.root.after(100, self.poll)

    def worker(self, path):
        """Runs in a background thread so the window never freezes."""
        try:
            if detect_kind(path) == "cp":
                out = run_cpp("closest_pair", path)
                n, pts = read_points(path)
                self.queue.put(("cp", path, out, n, pts))
            else:
                out = run_cpp("karatsuba", path)
                a, b = read_numbers(path)
                self.queue.put(("im", path, out, a, b))
        except Exception as e:# show any problem in a message box
            self.queue.put(("err", str(e)))

    def poll(self):
        try:
            msg = self.queue.get_nowait()
        except queue.Empty:
            self.root.after(100, self.poll)
            return
        self.prog.stop()
        self.runBtn.state(["!disabled"])
        self.status.config(text="")
        if msg[0] == "err":
            messagebox.showerror("Error", msg[1])
        elif msg[0] == "cp":
            self.show_cp(*msg[1:])
        else:
            self.show_im(*msg[1:])

    #results
    def set_result(self, out, extra=""):
        info = parse_output(out)
        self.timeLbl.config(text=info.get("Time", "-"))
        ok = info.get("Check", "").upper().startswith("PASS")
        self.checkLbl.config(text=("Verified: PASS" if ok else "Check: " + info.get("Check", "?")),
                             foreground="#1b8a3a" if ok else "#c62828")
        self.resText.config(state="normal")
        self.resText.delete("1.0", "end")
        self.resText.insert("end", out + ("\n\n" + extra if extra else ""))
        self.resText.config(state="disabled")
        return info

    def show_view(self, which):
        self.canvas.pack_forget()
        self.text.pack_forget()
        (self.canvas if which == "canvas" else self.text).pack(fill="both", expand=True)

    def set_steps(self, steps):
        self.steps = steps
        self.idx = 0
        self.slider.config(to=max(0, len(steps) - 1))
        self.slider.set(0)

    #closest pair
    def show_cp(self, path, out, n, pts):
        info = parse_output(out)
        coords = re.findall(r"\(\s*([-\d.eE+]+)\s*,\s*([-\d.eE+]+)\s*\)", info.get("Closest pair", ""))
        cpp_pair = [(float(a), float(b)) for a, b in coords] if len(coords) == 2 else None
        self.show_view("canvas")
        if n <= STEP_LIMIT:
            steps, self.sortedP, dist = closest_pair_steps(pts)
            self.pts = pts
            self.mode = "cp_steps"
            cpp_dist = float(info.get("Distance", "nan"))
            same = abs(dist - cpp_dist) <= 1e-5 * max(1.0, cpp_dist)
            self.set_result(out, "Step-by-step replay " + ("agrees with" if same else "DIFFERS from") +
                            " the C++ result.")
            self.set_steps(steps)
            self.goto(0, force=True)
        else:
            self.mode = "cp_large"
            self.pts = pts
            self.pair = tuple(cpp_pair) if cpp_pair else None
            self.set_steps([])
            self.stepLbl.config(text="")
            self.set_result(out)
            self.descLbl.config(text=f"Large input (n = {n:,}).  Showing a sample of {len(pts):,} points (grey). "
                                     f"The red rings mark the closest pair found by your C++ program. "
                                     f"Final result and running time are on the right.")
            self.cp_draw()

    def cp_draw(self):
        c = self.canvas
        c.delete("all")
        if self.mode not in ("cp_steps", "cp_large") or not self.pts:
            return
        W, H, pad = max(c.winfo_width(), 300), max(c.winfo_height(), 300), 36
        step = self.steps[self.idx] if self.mode == "cp_steps" else None
        pair = step["pair"] if step else self.pair

        xs = [p[0] for p in self.pts]
        ys = [p[1] for p in self.pts]
        if pair:
            xs += [pair[0][0], pair[1][0]]
            ys += [pair[0][1], pair[1][1]]
        x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
        sx = (W - 2 * pad) / ((x1 - x0) or 1)
        sy = (H - 2 * pad) / ((y1 - y0) or 1)
        X = lambda x: W / 2 if x1 == x0 else pad + (x - x0) * sx
        Y = lambda y: H / 2 if y1 == y0 else H - pad - (y - y0) * sy
        top, bottom = pad - 14, H - pad + 14

        lo, hi, strip = 0, 0, set()
        if step:
            P = self.sortedP
            lo, hi = step["lo"], step["hi"]
            c.create_rectangle(X(P[lo][0]) - 6, top, X(P[hi - 1][0]) + 6, bottom,
                               fill="#eaf2ff", outline="#bcd3ff")              # current sub-problem
            if step["delta"] is not None and step["xstar"] is not None and step["kind"] in ("merge", "strip", "best"):
                c.create_rectangle(X(step["xstar"] - step["delta"]), top, X(step["xstar"] + step["delta"]), bottom,
                                   fill="#fff1d6", outline="#f5c26b")          # the strip
            strip = set(step["strip"])
            points = [(p, lo <= i < hi) for i, p in enumerate(P)]
        else:
            points = [(p, False) for p in self.pts]

        for p, inside in points:
            colour = "#f08c00" if p in strip else ("#2f6fdd" if inside else "#9aa5b1")
            r = 4 if (p in strip or inside) else 3
            c.create_oval(X(p[0]) - r, Y(p[1]) - r, X(p[0]) + r, Y(p[1]) + r, fill=colour, outline="")

        if step and step["xstar"] is not None:
            c.create_line(X(step["xstar"]), top, X(step["xstar"]), bottom, dash=(6, 4), fill="#333", width=2)
        if pair and pair[0]:
            (ax, ay), (bx, by) = pair
            c.create_line(X(ax), Y(ay), X(bx), Y(by), fill="#d32f2f", width=3)
            for px, py in ((ax, ay), (bx, by)):
                c.create_oval(X(px) - 9, Y(py) - 9, X(px) + 9, Y(py) + 9, outline="#d32f2f", width=3)
        if step and step["delta"] is not None:
            c.create_text(10, 8, anchor="nw", font=("Segoe UI", 11, "bold"),
                          text=f"best distance so far (delta) = {step['delta']:.3f}")
        legend = "blue = current sub-problem   dashed line = split   orange = strip   red = best pair so far" \
            if step else "grey = sample of the points   red = closest pair"
        c.create_text(10, H - 8, anchor="sw", font=("Segoe UI", 9), fill="#555", text=legend)

    #integer multiplication
    def show_im(self, path, out, a, b):
        info = parse_output(out)
        self.show_view("text")
        self.pair = None
        if len(a) <= STEP_LIMIT and len(b) <= STEP_LIMIT:
            steps, product = karatsuba_steps(a, b)
            self.mode = "im_steps"
            shown = info.get("Product", "")
            if "..." in shown:
                head, _, tail = shown.partition("...")
                same = str(product).startswith(head.strip()) and str(product).endswith(tail.strip())
            else:
                same = str(product) == shown
            self.set_result(out, "Step-by-step replay " + ("agrees with" if same else "DIFFERS from") +
                            " the C++ product.")
            self.set_steps(steps)
            self.goto(0, force=True)
        else:
            self.mode = "im_large"
            self.set_steps([])
            self.stepLbl.config(text="")
            self.set_result(out)
            self.descLbl.config(text="Large input: the final result and the running time are shown (step by step is "
                                     "only for small inputs).")
            self.text.config(state="normal")
            self.text.delete("1.0", "end")
            self.text.insert("end", "Integer multiplication (Karatsuba)\n\n"
                                    f"File           : {os.path.basename(path)}\n"
                                    f"Digits         : {info.get('Digits', '')}\n"
                                    f"Product digits : {info.get('Product digits', '')}\n"
                                    f"Product        : {info.get('Product', '')}\n"
                                    f"Running time   : {info.get('Time', '')}\n"
                                    f"Check          : {info.get('Check', '')}\n")
            self.text.config(state="disabled")

    def show_kara(self, i):
        t = self.text
        t.config(state="normal")
        t.delete("1.0", "end")
        for k in range(i + 1):
            t.insert("end", self.steps[k] + "\n\n", "now" if k == i else "")
        t.see("end")
        t.config(state="disabled")

    # step controls
    def goto(self, i, force=False):
        if not self.steps:
            return
        i = max(0, min(i, len(self.steps) - 1))
        if i == self.idx and not force:
            return
        self.idx = i
        self.slider.set(i)
        self.stepLbl.config(text=f"Step {i + 1} / {len(self.steps)}")
        if self.mode == "cp_steps":
            self.descLbl.config(text=self.steps[i]["text"])
            self.cp_draw()
        elif self.mode == "im_steps":
            self.descLbl.config(text=self.steps[i].splitlines()[0])
            self.show_kara(i)

    def on_slider(self, value):
        i = int(float(value))
        if self.steps and i != self.idx:
            self.goto(i)

    def toggle_play(self):
        if self.playing:
            self.stop_play()
        elif self.steps:
            if self.idx >= len(self.steps) - 1:
                self.goto(0)
            self.playing = True
            self.playBtn.config(text="Pause")
            self.tick()

    def tick(self):
        if not self.playing:
            return
        if self.idx >= len(self.steps) - 1:
            self.stop_play()
            return
        self.goto(self.idx + 1)
        self.job = self.root.after(int(1800 / self.speed.get()), self.tick)

    def stop_play(self):
        self.playing = False
        self.playBtn.config(text="Play")
        if self.job:
            self.root.after_cancel(self.job)
            self.job = None


if __name__ == "__main__":
    window = tk.Tk()
    App(window)
    window.mainloop()
