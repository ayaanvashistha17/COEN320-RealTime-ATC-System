Here is a comprehensive, production-grade `README.md` designed for your repository. It balances a high-level conceptual overview for non-technical readers (like recruiters) with deep architectural mechanics for technical observers (like engineers and professors).

---

# COEN 320: Real-Time Air Traffic Control (ATC) System

An industrial-grade, multi-threaded Air Traffic Control (ATC) simulator built to track, monitor, and manage commercial aircraft navigating through simulated en-route airspace. Developed as a core design project for **COEN 320: Introduction to Real-Time Systems** at **Concordia University**.

The system leverages the native architectural components of the **QNX Real-Time Operating System (RTOS)** to guarantee predictable execution, deterministic timing constraints, and safe communication between concurrent components.

---

## 🌍 Executive Summary (Non-Technical Overview)

In aviation, the most critical phase of flight safety happens when aircraft leave airport radar towers and enter high-altitude cruise sectors. This project acts as the "brains" of an en-route Air Traffic Control center.

The system reads an active flight manifest file, introduces planes into a virtual 3D sky container, updates their positions in real-time, and constantly projects their flight paths forward. If two aircraft fly too close to one another—or are projected to cross paths dangerously within the next 3 minutes—the system throws an immediate collision alarm so a human controller can intervene and alter their speed or altitude trajectories.

---

## 🛠️ Tech Stack & Environment

* **Language:** C/C++ (Object-Oriented Design) 


* **Operating System:** QNX Neutrino RTOS (7.0+ x86 / AArch64 Targets) 


* **Development Environment:** QNX Momentics IDE (Eclipse-based platform) 


* **Low-Level Primitives:** QNX Kernel Channels, Native Pulsing Timers, POSIX Shared Memory (`mmap`), Standard Threads (`pthread`)

---

## 🏗️ System Architecture & Subsystems

The project workspace is cleanly divided into three decoupled operational subsystems that communicate via real-time Inter-Process Communication (IPC):

```
                       +---------------------------------------+
                       |              Lab4_ATC                 |
                       | (Aircraft Threads & Radar Subsystem)  |
                       +---------------------------------------+
                                           |
                                           | Flushes Telemetry Frames via
                                           v POSIX Shared Memory Map
                       +---------------------------------------+
                       |           Lab5_Computer               |
                       |  (Predictive Brains & Console Input)  |
                       +---------------------------------------+
                                           |
                                           | Emits Rendering Stream
                                           v
                       +---------------------------------------+
                       |          ProjectDisplay               |
                       |    (5-Second Space Visualization)     |
                       +---------------------------------------+

```

### 1. `Lab4_ATC` (Core Simulation & Telemetry Caching)

* **Concurrent Aircraft Actors:** Every airplane inside the boundaries is instantiated as an independent, periodic execution thread. Every 1 second, it updates its internal coordinates based on velocity vectors ($SpeedX, SpeedY, SpeedZ$).


* **Double-Buffered Radar Scanning:** Implements a background Radar polling thread that queries every active aircraft server at a strict 1Hz frequency. It stores telemetry data into an inactive vector buffer and performs an atomic pointer swap, guaranteeing that external systems can read consistent tracking data without causing threading race conditions.

### 2. `Lab5_Computer` (The Analytical Engine)

* **Predictive Collision Avoidance:** Scans the active airspace matrix and projects spatial trajectories forward by an adjustable $n$-second look-ahead window.


* **Operator Console Overrides:** Hosts a console input loop enabling an operator to query extended telemetry options or manually issue command adjustments (`send(R, m)`) to modify a targeted plane's course parameters.



### 3. `ProjectDisplay` (Visualizer Component)

* **Plan View Rasterization:** A dedicated rendering thread that wakes up precisely every 5 seconds to generate an scannable, flat top-down grid map showing the positional locations of all planes currently traveling through the tracking zone.



---

## 📊 Airspace Operational Parameters

The software actively enforces the following geometric boundaries and strict safety rules:

| Parameter | Operational Specification | Source Reference |
| --- | --- | --- |
| **Airspace Floor** | 15,000 feet above sea level

 |  |
| **Horizontal Area** | <br>$100000 \times 100000$ spatial units |  |
| **Vertical Ceiling** | 25,000 units total height

 |  |
| **Minimum Horizontal Buffer** | 3,000 distance units between aircraft

 |  |
| **Minimum Vertical Buffer** | 1,000 distance units between aircraft

 |  |
| **Predictive Alert Warning** | Triggered if collision is imminent within 3 minutes

 |  |
| **Airspace Logging Lifecycle** | Complete state telemetry saved to history files every 30 seconds

 |  |

---

## ⚡ Real-Time Implementation Mechanics

### Deterministic Heartbeat (`ATCTimer`)

Instead of standard non-deterministic system utilities (like `sleep`), this project implements high-precision kernel timers utilizing QNX Pulse Notifications (`timer_create` and `SIGEV_PULSE_INIT`). The calling thread blocks on a `MsgReceive` primitive, yielding execution control back to the OS CPU manager until the system clock fires a precise hardware timer pulse to unblock it.

### High-Speed IPC Data Highways

* **QNX Microkernel Messaging:** Native `MsgSend` / `MsgReceive` / `MsgReply` structures are used for synchronous telemetry collection and critical message-passing overrides.
* **Shared Memory Maps:** The transition data path between the Radar scanner and the analytical computer core utilizes high-throughput POSIX shared memory allocations (`shm_open`, `ftruncate`, and `mmap`), bypassing kernel-space context switches for memory transfers.

### Schedulability & Benchmarking

* **Priority Assignment:** Task execution hierarchies are calculated using **Rate Monotonic Scheduling (RMS)**, assigning higher priority execution tracks to threads with tighter cyclic periods.


* **Execution Profiling:** The code wraps processing blocks with high-resolution clock cycle measurements (`ClockCycles()`) to track and analyze **Best-Case Execution Time (BCET)** and **Worst-Case Execution Time (WCET)** under stress conditions.



---

## 🚀 Setup and Compilation inside QNX Momentics

Since this architecture requires the native QNX Neutrino RTOS microkernel headers (`sys/neutrino.h`, `sys/dispatch.h`), it must be built inside the official **QNX Momentics IDE** targeting an x86 or AArch64 target environment.

1. Open **QNX Momentics IDE**.
2. File -> Import -> **Existing Projects into Workspace**.
3. Set the root directory to your cloned repository folder and select `Lab4_ATC`, `Lab5_Computer`, and `ProjectDisplay`.
4. Right-click on each project directory -> **Build Project** using your target platform's toolchain (e.g., `aarch64le-debug`).
5. Ensure your simulation configuration path contains a valid `planes.txt` data layout structured as follows:


```text
Time   ID   X      Y      Z      SpeedX  SpeedY  SpeedZ
1      101  20000  25000  18000  250     0       0
3      102  80000  25000  18500  -200    0       0

```


6. Run the compiled system binaries across your QNX evaluation targets.
