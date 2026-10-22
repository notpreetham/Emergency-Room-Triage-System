# Emergency Room (ER) Triage & Patient Management System

A high-performance, full-stack clinical triage management system designed for hospital Emergency Departments. The application pairs a robust **Pure C** rule-based clinical scoring engine and priority queue with an intuitive, glassmorphic **Web Dashboard** powered by Node.js, Express, and Vanilla JavaScript.

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [System Features](#system-features)
- [Clinical Triage Scoring Model](#clinical-triage-scoring-model)
- [Directory Structure](#directory-structure)
- [Getting Started](#getting-started)
  - [Prerequisites](#prerequisites)
  - [1. Compile the C Engine](#1-compile-the-c-engine)
  - [2. Start the Web Dashboard](#2-start-the-web-dashboard)
  - [3. (Optional) Run C Terminal Interactive Mode](#3-optional-run-c-terminal-interactive-mode)
- [REST API Reference](#rest-api-reference)
- [Technologies Used](#technologies-used)
- [Data Storage & Persistence](#data-storage--persistence)

---

## Overview

In an emergency department, care must be delivered based on medical urgency rather than first-come, first-served order. This system automates the triage intake process:
1. Intake nurses record patient demographics, symptoms, and physiological vital signs.
2. The core C engine analyzes clinical indicators (oxygen saturation, heart rate, body temperature, age, transport modality, and critical symptom keywords) to compute an objective **Severity Score** and assign an acuity classification (**CRITICAL**, **SERIOUS**, **MODERATE**, **STABLE**).
3. Patients enter a dynamically sorted **Priority Queue** with wait-time escalation rules to prevent clinical deterioration while waiting.
4. Clinicians and triage staff monitor the live queue, treat next-in-line critical patients, adjust vitals, and review aggregated hospital capacity metrics.

---

## Architecture

The project enforces a strict separation between medical business logic and graphical presentation:

```
┌────────────────────────────────────────────────────────┐
│             Web Client (HTML5 / CSS3 / JS)            │
│  - Control Dashboard (KPIs, Active Ticker, Top 5)     │
│  - Triage Queue Board (Priority sorting, filters)     │
│  - Registration Form (Live Pre-Triage Simulator)      │
│  - Patient Profile (Vitals adjustment)                │
│  - Reports & Analytics (Chart.js visualizations)      │
└───────────────────────────┬────────────────────────────┘
                            │ REST API (JSON)
                            ▼
┌────────────────────────────────────────────────────────┐
│            Node.js + Express API Gateway               │
│  - Endpoint routing (/api/queue, /api/patient, etc.)   │
│  - Headless process orchestration (execFile)           │
│  - Zero medical calculations (pure orchestration)      │
└───────────────────────────┬────────────────────────────┘
                            │ Child Process Execution
                            ▼
┌────────────────────────────────────────────────────────┐
│            Compiled C Triage Engine (triage.exe)       │
│  - Single source of clinical truth                     │
│  - Priority Queue Linked List management               │
│  - Severity scoring & dynamic wait-time escalation     │
│  - Analytics aggregation & report generation           │
└───────────────────────────┬────────────────────────────┘
                            │ Direct Disk File I/O
                            ▼
┌────────────────────────────────────────────────────────┐
│          Persistent Storage (data/patients.txt)        │
│  - Pipe-delimited flat-file database                   │
│  - Timestamped snapshot backups                        │
└────────────────────────────────────────────────────────┘
```

---

## System Features

- **Decoupled Multi-Tier Design**: Medical decision algorithms are isolated inside compiled C for predictable execution, low overhead, and integrity.
- **Priority Queue Linked List**: Dynamically positions patients based on severity score and arrival time ($O(1)$ head extraction for treatments, $O(N)$ priority insertion).
- **Dynamic Deterioration Prevention**: Escalates severity score (+5 points for every 15 minutes of wait time) to ensure waiting patients are not starved by newer arrivals.
- **Dual Backend Modes**:
  - **Headless JSON CLI**: Accepts arguments and JSON payloads to seamlessly communicate with the Node.js API server.
  - **Interactive Terminal Console**: Standalone ANSI-colored terminal application with interactive menus for offline or terminal-only operation.
- **Interactive Web Interface**:
  - **Dashboard**: High-level hospital overview with active statistics, severity averages, top-5 queue preview, and a critical alert ticker.
  - **Live Intake Simulator**: Calculates predicted triage levels and wait times on-the-fly inside the client browser as vitals are entered.
  - **Triage Queue**: Color-coded queue grid with search filtering, direct status updates, and single-click treatment actions.
  - **Patient Profile**: Full vital signs inspection and editing form that automatically triggers dynamic queue re-sorting.
  - **Analytics & Visualizations**: Four interactive charts (Doughnut, Bar, Pie, Line) powered by Chart.js displaying acuity distribution, symptom frequencies, throughput ratios, and load curves.
- **Persistent Flat-File Database**: All patient records and treatment statuses survive process restarts via a pipe-delimited format with one-click snapshot backups.

---

## Clinical Triage Scoring Model

The severity scoring engine evaluates vital signs and clinical presentation according to the following point breakdown:

| Parameter | Clinical Criteria | Points Added |
|---|---|---|
| **Oxygen Saturation** | $\text{SpO}_2 < 90\%$ | `+50` |
| **Circulation** | $\text{Heart Rate} > 120\text{ bpm}$ | `+30` |
| **Temperature** | $\text{Temperature} > 102.0^\circ\text{F}$ | `+15` |
| **Age** | $\text{Age} > 60\text{ years}$ | `+10` |
| **Transport** | Arrival via Ambulance | `+20` |
| **Symptom Criteria** | Chest Pain | `+25` |
| **Symptom Criteria** | Difficulty Breathing / Shortness of Breath | `+30` |
| **Symptom Criteria** | Unconscious / Fainted / Syncope | `+50` |
| **Symptom Criteria** | Heavy Bleeding / Hemorrhage | `+40` |
| **Symptom Criteria** | Stroke Symptoms (Facial Droop, Slurred Speech, Numbness) | `+50` |

### Triage Levels & Estimated Wait Times

| Triage Level | Score Threshold | Target Clinical Action | Base Wait Time Formula |
|---|---|---|---|
| **CRITICAL** | $\text{Score} \ge 80$ | Immediate resuscitation | $0\text{ mins}$ |
| **SERIOUS** | $50 \le \text{Score} < 80$ | Urgent assessment | $15\text{ mins} + (10 \times \text{Critical/Serious ahead})$ |
| **MODERATE** | $20 \le \text{Score} < 50$ | Standard emergency care | $60\text{ mins} + (15 \times \text{Moderate+ ahead})$ |
| **STABLE** | $\text{Score} < 20$ | Non-urgent treatment | $120\text{ mins} + (20 \times \text{Total ahead})$ |

---

## Directory Structure

```text
Emergency-Room-Triage-System/
├── ER-Triage-Backend/                 # Pure C Core Engine
│   ├── include/
│   │   ├── triage.h                   # Patient struct & severity scoring definitions
│   │   ├── queue.h                    # Priority queue linked list declarations
│   │   ├── fileio.h                   # Pipe-delimited file persistence headers
│   │   ├── reports.h                  # Analytics and summary structures
│   │   ├── json.h                     # JSON serializer and parser headers
│   │   └── utils.h                    # String manipulation and time helpers
│   ├── src/
│   │   ├── main.c                     # CLI router & interactive terminal loop
│   │   ├── triage.c                   # Clinical scoring algorithms
│   │   ├── queue.c                    # Priority insertion, wait-time recalculation
│   │   ├── fileio.c                   # Disk read/write & backup routines
│   │   ├── reports.c                  # Analytics generation & symptom rankings
│   │   ├── json.c                     # Lightweight custom JSON tokenizer
│   │   └── utils.c                    # Timestamp formatting and string sanitization
│   ├── data/
│   │   └── patients.txt               # Main pipe-delimited database file
│   ├── sample_data/                   # Sample JSON files for headless execution
│   ├── Makefile                       # GCC compilation script
│   └── README.md                      # Backend-specific documentation
│
├── ER-Triage-Web/                     # Node.js + Express Web Layer
│   ├── server/
│   │   ├── package.json               # Express and dependencies configuration
│   │   └── server.js                  # Express API server & C backend bridge
│   ├── frontend/
│   │   ├── index.html                 # Main Hospital Control Dashboard
│   │   ├── queue.html                 # Active Triage Queue Board
│   │   ├── register.html              # Patient Registration & Intake Simulator
│   │   ├── patient.html               # Patient Profile & Vitals Editor
│   │   ├── reports.html               # Graphical Analytics & Reports
│   │   ├── css/
│   │   │   └── style.css              # Glassmorphic CSS stylesheet
│   │   └── js/
│   │       ├── api.js                 # Frontend API client & toast notifications
│   │       ├── dashboard.js           # Dashboard metrics & priority list logic
│   │       ├── queue.js               # Queue table rendering and search filters
│   │       ├── register.js            # Intake validation & client-side simulation
│   │       └── reports.js             # Chart.js initialization and data binding
│   └── README.md                      # Web application documentation
│
└── README.md                          # Main project documentation (this file)
```

---

## Getting Started

### Prerequisites

- **GCC Compiler**: MinGW / MSYS2 for Windows, or standard GCC on Linux/macOS.
- **Make Utility**: `make` or `mingw32-make` on Windows.
- **Node.js**: Version 16.x or newer (npm included).

---

### 1. Compile the C Engine

Open your terminal, navigate to the `ER-Triage-Backend` directory, and compile the executable:

```bash
cd ER-Triage-Backend
mingw32-make
# (or 'make' on Linux / macOS)
```

This will produce the binary inside `ER-Triage-Backend/build/triage.exe` (or `build/triage`).

---

### 2. Start the Web Dashboard

1. Navigate to the web server directory and install dependencies:
   ```bash
   cd ../ER-Triage-Web/server
   npm install
   ```

2. Launch the Express server:
   ```bash
   npm start
   ```

3. Open your browser and visit:
   ```
   http://localhost:3000
   ```

You can now register patients, view the prioritized queue, update vitals, and monitor real-time hospital analytics through the web interface.

---

### 3. (Optional) Run C Terminal Interactive Mode

You can also use the C backend directly from the command line without the web server. Run the compiled binary without arguments:

```bash
cd ER-Triage-Backend
./build/triage.exe
```

This launches the interactive terminal menu:
```text
==================================================
       ER TRIAGE CLINICAL CONTROL SYSTEM          
==================================================
 Queue Size: 4 | Treated: 12 | Avg Severity: 42.5
 Critical Patients: 1 | Serious: 1
--------------------------------------------------
1. Register Patient
2. View Priority Queue
3. Treat Next Patient
4. Update Patient Vitals/Symptoms
5. Search Patient Records
6. View Comprehensive Analytics Report
7. Trigger Database Backup
8. Exit
Enter choice (1-8):
```

---

## REST API Reference

The Node.js server (`server/server.js`) exposes the following endpoints:

| Method | Endpoint | Description | Backend Action |
|---|---|---|---|
| `GET` | `/api/queue` | Retrieve active waiting queue | `triage queue` |
| `POST` | `/api/patient` | Register a new patient record | `triage add <temp_json>` |
| `GET` | `/api/patient/:id` | Search for a patient by ID | `triage search <id>` |
| `PUT` | `/api/patient/:id` | Update clinical vitals or symptoms | `triage update <temp_json>` |
| `DELETE` | `/api/patient/next` | Treat and discharge highest-priority patient | `triage treat` |
| `GET` | `/api/reports` | Summary metrics and symptom tallies | `triage report` |
| `POST` | `/api/load` | Trigger timestamped database snapshot backup | `triage backup` |

---

## Technologies Used

- **Core Engine**: C (C99 Standard, GNU GCC)
- **Data Structures**: Priority Queue using Singly Linked Lists, Custom Hash/Token String Parsers
- **Backend Bridge**: Node.js, Express.js (`child_process.execFile`)
- **Frontend**: HTML5, CSS3 (Modern Glassmorphism, CSS Custom Properties, Flexbox & CSS Grid), Vanilla JavaScript (ES6+ Asynchronous Fetch)
- **Data Visualization**: Chart.js (Doughnut, Bar, Pie, Line charts)
- **Persistence**: Flat-File Pipe-Delimited (`|`) Storage

---

## Data Storage & Persistence

Patient records are stored in `ER-Triage-Backend/data/patients.txt` with pipe-delimited fields:

```text
ID|Name|Age|Gender|Symptoms|HeartRate|OxygenLevel|Temperature|BloodPressure|ArrivalTime|ArrivalTimeStr|Ambulance|SeverityScore|TriageLevel|WaitTime|Status
```

- **Automatic Persistence**: Every addition, update, or treatment modification automatically writes through to disk.
- **Snapshot Backups**: The backup command generates timestamped files (`data/patients_backup_<timestamp>.txt`) preserving historical state before significant shifts or audits.
