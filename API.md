# CoreMetrics API reference

The public interface of the CoreMetrics GUI library. CoreMetrics is a small C++23 UI
toolkit built directly on raw SDL3 surfaces: a software rasterizer (`Screen`), a widget
hierarchy (`GUIElement` and friends), a layout tree, an event system, and a cross-platform
metrics facade. This document covers every class a consumer of the library would use.

For a guided tour of the repo see [DOCS.md](DOCS.md); for the project overview see
[README.md](README.md). Every signature below is copied verbatim from the headers in
[`include/`](include/).

## Contents

- [Math](#math): `Tvec2<T>`, `Tvec3<T>`, `Matrix`, `linear.hpp`
- [Graphics](#graphics): `Screen`, `ThreadPool`
- [GUI elements](#gui-elements): `GUIElement`, `Cloneable<Derived>`, `GUIElementFactory`, `GUIElementType`, `Point`, `Line`, `Box`, `Label`, `Selection`, `Image`, `Button`, `Bar`, `Row`
- [Layout](#layout): `Tree<T>`, `Layout`, `LayoutManager`, `LayoutUtils`, `GUIFile`
- [Events](#events): `Event`, `ClickEvent`, `ShowEvent`, `SoundEvent`, `EventManager`, `SoundPlayer`, `Font`
- [System](#system): `ProcessInfo`, `SystemMetrics`, `ProcessUtils`, `ProcParsers`
- [Charting](#charting): `RingBuffer<T>`, `Sparkline`

---

## Math

### Tvec2\<T\>
Generic 2-component vector. Template parameter `T` is the component type (`float` and `int` are aliased as `vec2` and `ivec2`). Exposes public reference members `T &x, &y` that alias an internal `components[2]` array. The `int` specialization redefines `magnitude()`/`unit()` to use L1 (Manhattan) distance and integer division.

- `Tvec2()`: default-constructs the zero vector `(0,0)`.
- `Tvec2(const Tvec2<T> &copy)`: copy constructor.
- `Tvec2(T _x, T _y)`: constructs from explicit components.
- `template <typename U> Tvec2(const Tvec2<U> &other)`: converting constructor; static_casts components from another `Tvec2<U>` (e.g. `ivec2`/`vec2`).
- `Tvec2<T> &operator=(const Tvec2<T> &copy)`: assignment.
- `bool operator==(const Tvec2<T> &rhs) const`: component-wise equality.
- `Tvec2<T> operator+(const Tvec2<T> &rhs) const`: vector addition.
- `Tvec2<T> operator-(const Tvec2<T> &rhs) const`: vector subtraction.
- `Tvec2<T> operator*(T scalar) const`: scalar multiplication.
- `T dot(const Tvec2<T> &rhs) const`: dot product.
- `T magnitude() const`: Euclidean length (float types); L1 norm for `ivec2`.
- `Tvec2<T> unit() const`: normalized vector; returns `(0,0)` for the zero vector.
- `T &operator[](int index)`: mutable indexed component access.
- `Tvec2<T> &operator+=(const Tvec2<T> &rhs)`: in-place addition.
- `Tvec2<T> &operator-=(const Tvec2<T> &rhs)`: in-place subtraction.
- `Tvec2<T> &operator*=(T scalar)`: in-place scalar multiplication.

Aliases: `typedef Tvec2<float> vec2;` `typedef Tvec2<int> ivec2;`

### Tvec3\<T\>
Generic 3-component vector. Template parameter `T` is the component type (aliased `vec3` for float, `ivec3` for int). Exposes public reference members `T &x, &y, &z` aliasing an internal `components[3]`. The `int` specialization uses L1 magnitude and integer-division `unit()`.

- `Tvec3()`: default-constructs the zero vector `(0,0,0)`.
- `Tvec3(const Tvec3<T> &copy)`: copy constructor.
- `Tvec3(T _x, T _y, T _z)`: constructs from explicit components.
- `template <typename U> Tvec3(const Tvec3<U> &other)`: converting constructor across component types.
- `Tvec3<T> operator=(const Tvec3<T> &copy)`: assignment (returns by value).
- `Tvec3<T> &operator+=(const Tvec3<T> &rhs)`: in-place addition.
- `Tvec3<T> &operator-=(const Tvec3<T> &rhs)`: in-place subtraction.
- `Tvec3<T> &operator*=(T scalar)`: in-place scalar multiplication.
- `bool operator==(const Tvec3<T> &rhs) const`: component-wise equality.
- `Tvec3<T> operator+(const Tvec3<T> &rhs) const`: vector addition.
- `Tvec3<T> operator-(const Tvec3<T> &rhs) const`: vector subtraction.
- `Tvec3<T> operator*(T scalar) const`: scalar multiplication.
- `T dot(const Tvec3<T> &rhs) const`: dot product.
- `T magnitude() const`: Euclidean length (float types); L1 norm for `ivec3`.
- `Tvec3<T> unit() const`: normalized vector; returns `(0,0,0)` for the zero vector.
- `Tvec3<T> cross(const Tvec3<T> &rhs) const`: cross product.
- `T &operator[](int index)`: mutable indexed component access.

Aliases: `typedef Tvec3<float> vec3;` `typedef Tvec3<int> ivec3;`

### Matrix
Fixed 3x3 single-precision matrix (`SIZE = 3`), used for 2D transforms.

- `Matrix() = default`: zero-initialized 3x3 matrix.
- `Matrix(float data[SIZE][SIZE])`: constructs from a 3x3 C-array of floats.
- `Matrix(const Matrix &copy)`: copy constructor.
- `Matrix &operator=(const Matrix &copy)`: assignment.
- `Matrix operator*(const Matrix &rhs) const`: matrix multiplication.
- `bool operator==(const Matrix &rhs) const`: element-wise equality.
- `Matrix toTranspose() const`: returns the transpose.

### linear.hpp
Convenience umbrella header. No types of its own; includes `vec2.hpp`, `vec3.hpp`, and `matrix.hpp`. Include this to pull in the whole math layer.

---

## Graphics

### Screen
Software framebuffer over a raw SDL3 surface; the from-scratch 2D rasterizer (pixels, lines, boxes, triangles, text). Owns an `SDL_Surface*` and is cleaned up in its destructor (RAII). Line-plotting helpers and edge tests are private.

- `Screen(unsigned int w, unsigned int h)`: allocates a `w`x`h` drawing surface.
- `~Screen()`: frees the underlying SDL surface.
- `void clear()`: clears the surface to background.
- `void blitTo(SDL_Surface *target)`: blits this screen onto a target SDL surface (e.g. the window).
- `void blitSurface(SDL_Surface *src, const Tvec2<int> &pos)`: blits a source surface onto this screen at `pos`.
- `void drawPixel(const Tvec2<int> &pos, const Tvec3<float> &color)`: sets a single pixel.
- `void drawLine(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<float> &color)`: draws a line (float color, 0–1 channels).
- `void drawLine(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<int> &color)`: overload taking an integer (0–255) color.
- `void drawBox(const Tvec2<int> &a, const Tvec2<int> &b, const Tvec3<float> &color)`: draws a box between two corners.
- `void drawTriangle(const Tvec2<int> &v1, const Tvec2<int> &v2, const Tvec2<int> &v3, const Tvec3<float> &color)`: rasterizes a triangle.
- `void drawText(const Tvec2<int> &pos, const Tvec3<float> &color, std::string text)` renders text via SDL_RenderDebugText (SDL3 built-in debug font). The bundled TTF is used by Font::drawText, which Label calls.

### ThreadPool
Singleton worker-thread pool for offloading work. Construction is private; copy and assignment are deleted. Tasks are submitted as callables and dispatched to worker threads.

- `static ThreadPool& getInstance()`: returns the process-wide singleton.
- `~ThreadPool()`: joins workers and tears down the pool.
- `ThreadPool(const ThreadPool&) = delete`: non-copyable.
- `ThreadPool& operator=(const ThreadPool&) = delete`: non-assignable.
- `size_t threadCount() const`: number of worker threads.
- `template<typename F> std::future<void> submit(F&& f)`: enqueues a callable, wakes a worker, and returns a `std::future<void>`; throws `std::runtime_error` if the pool has been stopped.

---

## GUI elements

### GUIElement
Abstract base for every drawable/interactive widget. Pure-virtual interface; the CRTP `Cloneable` mixin supplies `clone()`.

- `virtual ~GUIElement()`: virtual destructor.
- `virtual void draw(Screen& screen) = 0`: renders the element to a `Screen`.
- `virtual bool operator()(Event* event)`: handles an event; returns `false` by default (not consumed).
- `virtual GUIElement* clone() const = 0`: polymorphic deep copy.

### Cloneable\<Derived\>
CRTP mixin that auto-implements `clone()` for a derived widget. Template parameter `Derived` is the concrete subclass; it must be copy-constructible. Widgets inherit `public Cloneable<Self>` instead of `GUIElement` directly.

- `GUIElement* clone() const override`: returns `new Derived(*this)` (copy of the concrete type).
- `Derived* cloneDerived() const`: covariant-return clone yielding the exact derived pointer.

Free function in the same header:
- `template <typename T> std::unique_ptr<T> cloneUnique(const T& obj)`: clones `obj` and wraps the result in a `std::unique_ptr<T>`.

### GUIElementFactory
Static factory that builds widgets and returns them as `std::unique_ptr<GUIElement>` (owning). All methods are `static`; there is no instance.

- `static std::unique_ptr<GUIElement> createPoint(vec2 pos, vec3 color)`: builds a `Point`.
- `static std::unique_ptr<GUIElement> createLine(vec2 start, vec2 end, vec3 color)`: builds a `Line`.
- `static std::unique_ptr<GUIElement> createBox(vec2 minPos, vec2 maxPos, vec3 color)`: builds a `Box`.
- `static std::unique_ptr<GUIElement> createBar(ivec2 minPos, ivec2 maxPos, vec3 fillColor, vec3 bgColor, float minVal, float maxVal, std::string metricName)`: builds a `Bar`.
- `static std::unique_ptr<GUIElement> createRow(ivec2 minPos, ivec2 maxPos, std::vector<std::string> cells, std::vector<float> columnWeights, vec3 textColor)`: builds a `Row`.
- `static std::unique_ptr<GUIElement> createLabel(std::string text, ivec2 pos, vec3 color)`: builds a `Label`.
- `static std::unique_ptr<GUIElement> createButton(ivec2 minPos, ivec2 maxPos, vec3 color, std::string soundFile, std::string targetLayout, std::string targetLayoutHide)`: builds a `Button`.
- `static std::unique_ptr<GUIElement> create(GUIElementType type, vec2 pos1, vec2 pos2, vec3 color)`: generic dispatch for the simple geometric types (POINT/LINE/BOX); returns `nullptr` for BAR/ROW/LABEL/BUTTON (which require their specialized overloads) and for unknown types.

### GUIElementType
Scoped enum (`enum class`) tagging element kinds for the factory: `POINT, LINE, BOX, BAR, ROW, LABEL, BUTTON, IMAGE`. Declared in `GUIElements.hpp`, which is an umbrella include of all widget headers.

### Point
A single colored pixel widget. Inherits `Cloneable<Point>`. Public data members `vec2 pos; vec3 color;`.

- `Point()`: defaults position and color to zero.
- `Point(vec2 pos, vec3 color)`: constructs at `pos` with `color`.
- `void draw(Screen &screen) override`: plots the pixel.

### Line
A colored line segment widget. Inherits `Cloneable<Line>`. Public members `vec2 start; vec2 end; vec3 color;`.

- `Line()`: zero-initialized.
- `Line(vec2 start, vec2 end, vec3 color)`: constructs the segment.
- `void draw(Screen &screen) override`: draws the line.

### Box
A colored rectangle widget defined by two corners. Inherits `Cloneable<Box>`. Public members `vec2 minPos; vec2 maxPos; vec3 color;`.

- `Box()`: zero-initialized.
- `Box(vec2 minPos, vec2 maxPos, vec3 color)`: constructs the rectangle.
- `void draw(Screen &screen) override`: draws the box.

### Label
A text-string widget at a fixed position. Inherits `Cloneable<Label>`. Text/position/color are private.

- `Label(std::string text, ivec2 position, vec3 color)`: constructs the label.
- `virtual ~Label() = default`: destructor.
- `virtual void draw(Screen& screen) override`: renders the text.
- `std::string getText() const`: returns the current text.
- `void setText(std::string text)`: replaces the text.
- `void setPos(ivec2 pos)`: moves the label.

### Selection
A toggleable checkbox/selection widget with a text label. Inherits `Cloneable<Selection>`. State and label are private.

- `Selection(ivec2 position, std::string label, bool checked = false)`: constructs at `position`, optionally pre-checked.
- `virtual ~Selection() = default`: destructor.
- `virtual void draw(Screen& screen) override`: renders box + label.
- `void toggle()`: flips the selected state.
- `bool isSelected() const`: returns whether currently selected.

### Image
A widget that draws an image loaded from a file path at a position. Inherits `Cloneable<Image>`. Path and position are private.

- `Image(std::string filePath, ivec2 position)`: constructs from a file path and position.
- `virtual ~Image() = default`: destructor.
- `virtual void draw(Screen& screen) override`: blits the image.
- `std::string getFilePath() const`: returns the source path.

### Button
A clickable rectangular widget that, on click, can play a sound and show/hide target layouts. Inherits `Cloneable<Button>`; overrides `operator()` to consume `ClickEvent`s. Geometry and target strings are private.

- `Button(ivec2 minP, ivec2 maxP, vec3 col, std::string soundFile = "", std::string targetLayout = "", std::string targetLayoutHide = "")`: constructs the button with optional sound and layout-show/hide targets.
- `~Button()`: destructor.
- `void draw(Screen& screen) override`: draws the button rectangle.
- `bool checkToggle(int mouseX, int mouseY)`: returns whether the given coordinates fall inside the button.
- `bool operator()(Event* event) override`: handles a click; emits sound/show events and returns whether it was consumed.

### Bar
A horizontal progress/metric bar mapping a value within `[minVal, maxVal]` to a filled fraction. Inherits `Cloneable<Bar>`. Geometry, colors, range, value, and metric name are private.

- `Bar(ivec2 minPos, ivec2 maxPos, vec3 fillColor, vec3 bgColor, float minVal = 0.0f, float maxVal = 100.0f, std::string metricName = "")`: constructs the bar.
- `void setValue(float value)`: sets the current value.
- `float getValue() const`: returns the current value.
- `const std::string& getMetricName() const`: returns the metric name.
- `ivec2 getMinPos() const`: returns the top-left corner.
- `ivec2 getMaxPos() const`: returns the bottom-right corner.
- `void draw(Screen &screen) override`: renders background then fill.

### Row
A multi-column text row that lays out cells across its width using per-column weights. Inherits `Cloneable<Row>`. Cells, weights, geometry, and color are private.

- `Row(ivec2 minPos, ivec2 maxPos, std::vector<std::string> cells, std::vector<float> columnWeights, vec3 textColor)`: constructs the row.
- `void setCells(std::vector<std::string> cells)`: replaces the cell text.
- `const std::vector<std::string>& getCells() const`: returns the cell strings.
- `const std::vector<float>& getColumnWeights() const`: returns the column weights.
- `ivec2 getMinPos() const`: returns the top-left corner.
- `ivec2 getMaxPos() const`: returns the bottom-right corner.
- `void draw(Screen &screen) override`: renders each cell in its weighted column.

---

## Layout

### Tree\<T\>
Generic ownership tree node. Template parameter `T` is the payload type. Each node holds a `T`, a raw back-pointer to its parent, and a vector of `std::unique_ptr<Tree<T>>` children (children owned; parent non-owning). `LayoutManager` instantiates it as `Tree<Layout>`.

- `Tree(T data)`: constructs a root node holding `data` (moved in), no parent.
- `T& getData()` / `const T& getData() const`: accessors for the payload.
- `Tree<T>* getParent()`: returns the parent node (`nullptr` at root).
- `bool isRoot() const`: true if no parent.
- `bool isLeaf() const`: true if no children.
- `std::vector<std::unique_ptr<Tree<T>>>& getChildren()` / `const ... getChildren() const`: accessors for the owned child vector.
- `Tree<T>* addChild(T&& childData)`: creates an owned child holding `childData`, links its parent, and returns a non-owning pointer to it.

### Layout
A named, positioned container of GUI elements that can be active/hidden; positions resolve relative to a parent rectangle. Copyable (custom copy ctor/assign deep-copy the cloned elements) and movable.

- `Layout(vec2 start, vec2 end, bool active = true, std::string name = "")`: constructs a layout spanning `start`→`end`.
- `Layout(const Layout& other)`: deep copy (clones contained elements).
- `Layout& operator=(const Layout& other)`: deep-copy assignment.
- `Layout(Layout&& other) noexcept = default`: move constructor.
- `Layout& operator=(Layout&& other) noexcept = default`: move assignment.
- `~Layout() = default`: destructor.
- `void addElement(std::unique_ptr<GUIElement> element)`: takes ownership of an element and appends it.
- `void setActive(bool active)`: shows/hides the layout.
- `bool isActive() const`: returns active state.
- `vec2 getStart() const` / `vec2 getEnd() const`: relative bounds accessors.
- `void setName(std::string name)` / `std::string getName() const`: name accessors.
- `ivec2 resolveAbsStart(ivec2 parentStart, ivec2 parentEnd) const`: computes absolute top-left within the parent rect.
- `ivec2 resolveAbsEnd(ivec2 parentStart, ivec2 parentEnd) const`: computes absolute bottom-right within the parent rect.
- `void draw(Screen& screen, ivec2 parentStart, ivec2 parentEnd) const`: draws all elements at resolved absolute coordinates.
- Public member: `std::vector<std::unique_ptr<GUIElement>> elements`: the owned elements.

### LayoutManager
Singleton owning the root `Tree<Layout>` and driving recursive rendering of the layout hierarchy. Constructor is private; copy/assign deleted.

- `static LayoutManager& getInstance()`: returns the singleton.
- `Tree<Layout>& getRoot()`: returns the root layout node.
- `Tree<Layout>* addChild(Tree<Layout>* parent, Layout layout)`: adds `layout` as a child of `parent`; returns the new node.
- `void render(Screen& screen, ivec2 screenStart, ivec2 screenEnd) const`: recursively renders the active layout tree.
- `void clear()`: resets the tree.

### LayoutUtils (free functions)
Helpers for querying widgets inside a `Tree<Layout>` hierarchy. Declared in `LayoutUtils.hpp`; not a class.

- `Bar *findBarByMetric(Tree<Layout> &node, const std::string &metric)`: returns the first `Bar` whose metric name matches, or `nullptr`.
- `Label *nthLabelInLayout(Tree<Layout> &layoutNode, std::size_t index)`: returns the `index`-th `Label` in a layout node, or `nullptr`.
- `void collectRows(Tree<Layout> &node, std::vector<Row *> &out)`: appends pointers to all `Row` widgets under `node` into `out`.

### GUIFile
Reads and writes the GUI layout file format, populating/serializing a `LayoutManager`'s tree. Parsing helpers are private; two public entry points.

- `void readFile(const std::string &fileName, LayoutManager& manager)`: parses a layout file and builds the manager's layout tree.
- `void writeFile(std::string fileName, LayoutManager& manager)`: serializes the manager's layout tree to a file.

---

## Events

### Event
Base class for all events; carries an `EventType` tag. Subtypes dispatch on `getType()`.

- `Event(EventType type)`: constructs with a type tag.
- `virtual ~Event() = default`: virtual destructor.
- `EventType getType() const`: returns the event's type.

`EventType` enum: `EVENT_CLICK, EVENT_SHOW, EVENT_SOUND`.

### ClickEvent
Mouse-click event carrying screen coordinates. Inherits `Event` (`EVENT_CLICK`).

- `ClickEvent(int mouseX, int mouseY)`: constructs at the click position.
- `int getMouseX() const`: returns the click X.
- `int getMouseY() const`: returns the click Y.

### ShowEvent
Request to show or hide a named layout. Inherits `Event` (`EVENT_SHOW`).

- `ShowEvent(const std::string& layoutName, bool show)`: constructs targeting `layoutName` with desired visibility.
- `const std::string& getLayoutName() const`: returns the target layout name.
- `bool getShow() const`: true to show, false to hide.

### SoundEvent
Request to play a sound file. Inherits `Event` (`EVENT_SOUND`).

- `SoundEvent(const std::string& filePath)`: constructs for the given audio file.
- `const std::string& getFilePath() const`: returns the file path.

### EventManager
Singleton event queue that dispatches (trickles) events down the layout tree. Constructor private; copy/assign deleted.

- `static EventManager& getInstance()`: returns the singleton.
- `void pushEvent(std::unique_ptr<Event> event)`: enqueues an owned event.
- `void processEvents(ivec2 screenStart, ivec2 screenEnd)`: drains the queue, dispatching each event through the layout hierarchy.
- `static Tree<Layout>* findLayoutByName(Tree<Layout>& node, const std::string& name)`: searches the tree for a layout node by name; returns `nullptr` if absent.

### SoundPlayer
Singleton SDL3 audio playback wrapper. Constructor and copy/assign private/deleted; owns an `SDL_AudioStream*` (RAII).

- `static SoundPlayer& getInstance()`: returns the singleton.
- `~SoundPlayer()`: releases the audio stream.
- `void play(const std::string& filePath)`: plays the given audio file.
- `void shutdown()`: tears down the audio stream/subsystem.

### Font (namespace)
Free-function text-rendering facade over the font system. Not a class.

- `void Font::drawText(Screen &screen, const std::string &text, ivec2 pos, vec3 color)`: renders `text` onto a screen at `pos`.
- `void Font::shutdown()`: releases font resources.

---

## System

### ProcessInfo (struct)
Plain data record for one process row. Public fields: `int pid; int parentPid; std::string name; float cpuPct; float memPct; unsigned long long diskReadKbPerSec; unsigned long long diskWriteKbPerSec;`.

### MemBreakdown (struct)
Memory split into four htop-style segments. All fields in kilobytes. `unsigned long long totalKb; activeKb; wiredKb; cachedKb; freeKb;`. Segments cover the total: any unaccounted bytes fold into `freeKb`.

### DiskUsage (struct)
Root volume capacity. `unsigned long long totalKb; freeKb;`. Both zero when the platform call fails.

### SystemMetrics
Static facade exposing cross-platform system metrics (CPU / memory / GPU / disk / process list). All methods are `static`; platform implementations live in `SystemMetrics_{linux,mac,win}.cpp`.

- `static float readCpuPercent()`: current total CPU utilization (%).
- `static float readMemPercent()`: current memory utilization (%).
- `static float readGpuPercent()`: current GPU utilization (%).
- `static std::vector<float> readPerCoreCpu()`: per-logical-CPU utilization since the previous call. First call returns all zeros (no prior sample to diff). Empty vector on Windows (backend not implemented).
- `static MemBreakdown readMemBreakdown()`: active / wired / cached / free memory segments. Zeroed on failure.
- `static unsigned long long readUptimeSeconds()`: seconds since boot. Zero on failure.
- `static std::vector<float> readLoadAverages()`: 1, 5, 15-minute load averages. Always three elements; `{0, 0, 0}` on Windows.
- `static DiskUsage readDiskUsage()`: root-volume total and free capacity. Linux + macOS use `statvfs("/")`; Windows uses `GetDiskFreeSpaceExA("C:\\")`.
- `static std::vector<ProcessInfo> topProcesses(std::size_t n = 20)`: returns up to `n` top processes by memory usage. Each `ProcessInfo` carries CPU% (delta-based), MEM%, parent PID, and disk-I/O throughput in KB/sec.

### ProcessUtils
`ProcessUtils.hpp` exposes a sort-column enum and pure helpers used by every platform backend and by the in-app sort handler. Extracting the math out of the platform `topProcesses` bodies and the in-app `compareProcesses` wrapper makes them testable on any platform without `/proc` or a running window.

- `enum SortColumn { SORT_PID = 0, SORT_NAME = 1, SORT_CPU = 2, SORT_MEM = 3, SORT_DISK = 4 }`: column selector for sorting the process table.
- `std::string formatPct(float value)`: formats a percentage float as a display string with one decimal place.
- `float computeCpuPercentDelta(std::uint64_t procCurrent, std::uint64_t procPrevious, std::uint64_t denom)`: returns the per-process CPU% as `((procCurrent - procPrevious) / denom) * 100`, clamped to `[0, 100]`. Returns `0.0f` when `denom == 0` or when `procCurrent < procPrevious` (counter reset, pid reuse). Used by the macOS, Linux, and Windows backends.
- `std::uint64_t computeIoKbPerSec(std::uint64_t prev, std::uint64_t curr, double elapsedSec)`: returns `((curr - prev) / 1024) / elapsedSec` in KB/sec. Returns 0 when `elapsedSec <= 0` or when `curr < prev` (counter reset).
- `bool processNameMatchesFilter(const std::string &name, const std::string &needle)`: case-insensitive substring match. Empty needle always matches; empty name with non-empty needle never matches.
- `std::string formatUptimeString(unsigned long long seconds)`: formats seconds as `Up Xd Yh Zm` / `Up Yh Zm` / `Up Zm`. Returns `Up --` for 0.
- `std::string formatDiskIo(unsigned long long readKbPerSec, unsigned long long writeKbPerSec)`: formats `read + write` as `X.X MB/s` for >= 1 MB/s, `XXX KB/s` for 1..1023, empty string for 0.
- `bool compareProcessByColumn(const ProcessInfo &a, const ProcessInfo &b, SortColumn column, bool ascending)`: pure comparator for the Processes table. Numerical compare on PID / CPU% / MEM% / DISK (read + write), lexicographic on NAME. `ascending = true` puts smaller values first.

### ProcParsers
`ProcParsers.hpp` exposes platform-neutral string parsers for the three `/proc` files the Linux backend reads. Each takes file contents as a `std::string` and writes the result through an out-parameter so the same parser can be exercised with synthetic fixtures on any OS.

- `bool ProcParsers::parseProcStat(const std::string &content, unsigned long long &totalOut, unsigned long long &idleOut)`: parses the first `cpu` line of `/proc/stat`. Returns `false` on empty input, non-`cpu` label, or a truncated line.
- `bool ProcParsers::parseProcPidStatCpuTicks(const std::string &content, unsigned long long &ticksOut)`: parses `/proc/[pid]/stat` and returns `utime + stime` in jiffies. Uses `rfind(')')` so process names containing spaces or nested parens are handled correctly.
- `bool ProcParsers::parseProcStatusVmRssKb(const std::string &content, unsigned long long &kbOut)`: scans `/proc/[pid]/status` for `VmRSS:` and returns the value in kB.

## Charting

### RingBuffer\<T\>
Header-only fixed-capacity circular buffer (`include/RingBuffer.hpp`), mirroring the style of `Tree<T>`. Newest sample at the back, oldest at the front. When full, `push()` drops the oldest entry. Intended for rolling time-series windows (e.g. the 64-sample backing store of each `Sparkline`).

- `explicit RingBuffer(std::size_t cap)`: constructs with the given capacity (clamped to at least 1).
- `std::size_t getCapacity() const` / `std::size_t getSize() const`: capacity and current population.
- `bool isEmpty() const` / `bool isFull() const`.
- `void clear()`: drops every sample.
- `void push(const T &value)`: appends a sample; drops the oldest one if already full.
- `T at(std::size_t index) const`: index 0 is the oldest sample, `getSize() - 1` is the newest. Returns a default-constructed `T` if `index >= getSize()`.
- `T newest() const` / `T oldest() const`: convenience accessors. Return `T()` when the buffer is empty.

### Sparkline
Inline time-series chart widget (`include/Sparkline.hpp`). Stores its samples in a `RingBuffer<float>` and plots N-1 line segments through `Screen::drawLine` (the Bresenham path). Right-justified so the newest sample sits at the right edge of the bounding box, matching every other live monitor.

- `Sparkline(ivec2 minPos, ivec2 maxPos, vec3 color, float minValue, float maxValue, std::size_t capacity)`: constructs a chart inside the given pixel rectangle. `[minValue, maxValue]` defines the vertical scale; values outside are clamped, not stretched.
- `void push(float value)`: clamps `value` into the configured range and appends it to the rolling window.
- `void clear()`: drops every sample.
- `std::size_t getCapacity() const` / `std::size_t getSize() const`: capacity and current population.
- `ivec2 getMinPos() const` / `ivec2 getMaxPos() const`: the chart's bounding box.
- `void draw(Screen &screen) const`: paints the polyline. Does nothing while `getSize() < 2`.
