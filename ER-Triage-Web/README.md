# Emergency Room (ER) Triage System - Web Application

A professional, modern hospital control dashboard built using HTML5, CSS3, and Vanilla JavaScript. It connects via a minimal Node.js + Express communication server to a compiled C triage engine.

---

## 1. Architecture Overview

The application utilizes a strict decoupled multi-tier architecture to separate business rules from visual rendering:

```
[Browser Frontend (HTML5/CSS3/Vanilla JS)]
                 ↓ (REST JSON API)
[Node.js + Express Communication Layer]
                 ↓ (child_process.execFile execution)
[Compiled C Triage Engine (triage.exe)]
                 ↓ (File System I/O)
     [Database (data/patients.txt)]
```

### The Separation Rule
To guarantee clinical safety and strict architecture separation:
1. **The Node.js server does NOT contain any medical calculations.** It acts solely as an orchestrator, executing the compiled C program and relaying outputs.
2. **The C backend remains the single source of truth** for triage level classification, severity scoring, wait-time calculations, priority sorting, backup creation, and database persistence.

---

## 2. Technologies Used
- **Frontend**: HTML5 (Semantic Structure), CSS3 (Modern Glassmorphism Theme, HSL Color System, responsive Grid layouts), Vanilla JavaScript (DOM manipulation, asynchronous Fetch API).
- **Visualization**: Chart.js via CDN (Pie, Doughnut, Line, and Bar charts).
- **Communication Layer**: Node.js & Express.

---

## 3. Express REST API Endpoints

The communication server (`server/server.js`) exposes the following endpoints, mapping them directly to commands in the C backend:

| Method | Endpoint | Description | C Executable Map |
|---|---|---|---|
| **GET** | `/api/queue` | Fetches active patients waiting in the queue | `triage queue` |
| **POST** | `/api/patient` | Registers a new patient | `triage add <temp_file.json>` |
| **GET** | `/api/patient/:id` | Searches for a specific patient by ID | `triage search <id>` |
| **PUT** | `/api/patient/:id` | Updates vitals/symptoms of a patient | `triage update <temp_file.json>` |
| **DELETE**| `/api/patient/next`| Treats the next patient (highest priority) | `triage treat` |
| **GET** | `/api/reports` | Fetches aggregate analytics summaries | `triage report` |
| **POST** | `/api/save` | Explicit database sync confirmation | *(C auto-saves on mutation)* |
| **POST** | `/api/load` | Synchronizes database and triggers backup | `triage backup` |

---

## 4. Run Instructions

### Step 1: Compile the C Backend
Before starting the web server, make sure the C backend is compiled. Navigate to the `ER-Triage-Backend` directory and compile:
```bash
# In ER-Triage-Backend/
make
```
This will compile and generate `build/triage.exe` (or `build/triage`).

### Step 2: Install Web Dependencies
Navigate to the `ER-Triage-Web/server/` directory and run:
```bash
# In ER-Triage-Web/server/
npm install
```
This installs the required dependencies (`express` and `cors`).

### Step 3: Launch the Server
Start the Express server by executing:
```bash
npm start
```
The server will start on port `3000`. Open your browser and navigate to:
```
http://localhost:3000
```

---

## 5. Web Interface Sections

1. **Dashboard** (`index.html`): Displays status counters (KPIs) showing active, total, and treated patients, alongside a color-coded priority view of the top 5 waiting patients and a real-time warning ribbon.
2. **Registration Board** (`register.html`): A clean clinical registration intake form that features an **On-the-Fly Triage Prediction Simulator** that runs the clinical decision algorithm in JavaScript to preview scores and levels in real time.
3. **Queue Board** (`queue.html`): Displays a responsive grid of all active waiting patients, color-coded by urgency (Red = Critical, Orange = Serious, Yellow = Moderate, Green = Stable), with controls to View Details, Edit Vitals, or Treat.
4. **Patient Profile** (`patient.html`): A clinical chart page that displays complete patient parameters and lets clinicians modify vitals/symptoms and trigger dynamic queue reprioritization.
5. **Reports Board** (`reports.html`): Graphically renders hospital load metrics using Chart.js charts:
   - Urgency category percentages (Doughnut).
   - Common symptoms frequency (Bar).
   - Treated historical ratio (Pie).
   - Queue urgency loading curve (Line).
