# Emergency Room (ER) Triage System - Pure C Backend

A high-performance, production-quality Emergency Room Triage System implemented in pure C. The system manages patient records, prioritizes the care queue using clinical vitals and symptom criteria, and maintains persistent storage through a pipe-delimited data file.

---

## 1. Overview
In a busy hospital Emergency Department, patients must be treated based on clinical urgency rather than their arrival order. This pure C backend provides the underlying business logic, a rule-based triage scoring engine, and a priority queue to ensure critical patients receive immediate care. 

It supports two modes of operation:
1. **Interactive Terminal Mode**: A menu-driven dashboard console for on-site clinicians.
2. **Command Line Mode**: A headless utility designed to integrate with web servers (like Node.js) by consuming JSON inputs and returning JSON outputs to standard output (`stdout`).

---

## 2. Folder Structure
```
ER-Triage-Backend/
│
├── src/
│   ├── main.c           # Command routing & interactive console loop
│   ├── triage.c         # Clinical severity scoring logic
│   ├── queue.c          # Priority queue linked list operations
│   ├── reports.c        # Tally metrics & symptom analytics
│   ├── fileio.c         # Pipe-delimited database persistence
│   ├── utils.c          # String, time, and general helpers
│   └── json.c           # Lightweight manual JSON parser
│
├── include/
│   ├── triage.h         # Patient structural definitions & scoring headers
│   ├── queue.h          # Priority queue management headers
│   ├── reports.h        # Reporting structural definitions
│   ├── fileio.h         # Persistence and backup headers
│   ├── utils.h          # Helper function headers
│   └── json.h           # JSON parsing function headers
│
├── data/
│   └── patients.txt     # Main database file (pipe-delimited)
│
├── build/               # Directory for compiled object files and binaries
│
├── sample_data/         # JSON templates representing different clinical states
│   ├── patient_critical.json
│   └── patient_stable.json
│
├── Makefile             # Compiles backend using GCC
└── README.md            # Backend Documentation
```

---

## 3. Data Structures & Algorithms

### Data Structures
1. **`Patient` Struct**: Represents complete patient records including demographics, vitals (heart rate, temperature, oxygen level, BP), symptoms, ambulance indicator, severity score, triage level, wait times, and status.
2. **Priority Queue (`PatientNode`)**: Implemented as a singly linked list that is kept sorted by clinical priority at all times.
   - Singly linked list nodes support dynamic registration and removal with $O(1)$ pointer manipulations after locating insertion indices.

### Sorting & Priority Rules
Queue sorting is governed by the comparison algorithm:
1. **Primary Key**: **Severity Score** (descending order - higher scores come first).
2. **Secondary Key**: **Arrival Time** (ascending order - earlier arrivals break ties).
3. **Status Filter**: Active waiting patients are sorted and positioned at the head of the queue, while already treated patients are positioned at the tail.

---

## 4. Triage Engine Scoring Rules

The clinical severity score starts at $0$ and aggregates points based on these rules:

| Category | Indicator / Condition | Points |
|---|---|---|
| **Oxygenation** | Oxygen level $< 90\%$ | `+50` |
| **Circulation** | Heart rate $> 120\text{ bpm}$ | `+30` |
| **Temperature** | Body temperature $> 102.0^\circ\text{F}$ | `+15` |
| **Age** | Age $> 60\text{ years}$ | `+10` |
| **Transport** | Arrived via ambulance | `+20` |
| **Symptom Match** | Chest Pain | `+25` |
| **Symptom Match** | Difficulty breathing / Shortness of breath | `+30` |
| **Symptom Match** | Unconscious / Fainted / Passed out | `+50` |
| **Symptom Match** | Heavy bleeding / Hemorrhage | `+40` |
| **Symptom Match** | Stroke symptoms (numbness, slurred speech) | `+50` |

### Triage Classifications
- **CRITICAL** (Score $\ge 80$): Immediate resuscitation.
- **SERIOUS** (Score $50 \text{ to } 79$): High priority; urgent care.
- **MODERATE** (Score $20 \text{ to } 49$): Moderate priority.
- **STABLE** (Score $< 20$): Low priority; non-urgent.

---

## 5. Time Complexity Analysis

| Operation | Algorithm | Time Complexity | Rationale |
|---|---|---|---|
| **Registration (Insert)** | Priority Sorted Insertion | $O(N)$ | Traverses list to locate correct priority spot. |
| **Treat Patient (Pop)** | Head extraction | $O(1)$ | Extracts the first waiting patient (already at the head). |
| **Search Patient** | Substring / ID Match | $O(N)$ | Performs linear traversal of all active and treated patients. |
| **Update Patient** | Extract, Update, Re-insert | $O(N)$ | Unlinks the node, updates clinical fields, and re-inserts in sorted order. |
| **Report Generation** | Single pass aggregation | $O(N)$ | Iterates through all patients to compute totals and rank symptoms. |
| **Save / Load** | Disk Read / Write | $O(N)$ | Performs line-by-line reading or writing of patient files. |

---

## 6. Compilation & Execution

### Prerequisites
- GCC Compiler (MinGW for Windows)
- Make Utility

### Compiling
Navigate to the `ER-Triage-Backend` directory and run:
```bash
make
```
This creates the executable file inside the `build/` directory (`build/triage.exe` or `build/triage`).

To clean object files and binaries:
```bash
make clean
```

### Running Interactive Mode
Run the executable with no arguments to start the menu:
```bash
./build/triage
```

### Running Command Mode (JSON Interface)
Run the executable with one of the following commands:

* **Register Patient**:
  ```bash
  ./build/triage add sample_data/patient_critical.json
  ```
* **View Queue**:
  ```bash
  ./build/triage queue
  ```
* **Treat Next Patient**:
  ```bash
  ./build/triage treat
  ```
* **Update Patient**:
  ```bash
  ./build/triage update sample_data/patient_update.json
  ```
* **Search Patient**:
  ```bash
  ./build/triage search P101
  ```
* **Generate Report**:
  ```bash
  ./build/triage report
  ```
* **Backup Database**:
  ```bash
  ./build/triage backup
  ```

---

## 7. Future Scope
- **Direct Socket Interface**: Establish direct IPC socket servers inside C to handle request streams instead of spawning short-lived process instances.
- **Real-Time Heart Rate Monitoring**: Integrate IoT pulse-oximeter feeds to update scores and re-prioritize the queue dynamically without human intervention.
- **Natural Language Processing**: Implement symptom keyword extraction using embedded machine learning libraries rather than case-insensitive substring comparisons.
