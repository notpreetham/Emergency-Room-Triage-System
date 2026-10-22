const express = require('express');
const cors = require('cors');
const { execFile } = require('child_process');
const fs = require('fs').promises;
const path = require('path');

const app = express();
const PORT = process.env.PORT || 3000;

app.use(cors());
app.use(express.json());

// Resolve paths to the C backend executable and directory
const TRIAGE_EXE = path.resolve(__dirname, '../../ER-Triage-Backend/build/triage.exe');
const TRIAGE_CWD = path.resolve(__dirname, '../../ER-Triage-Backend');

// Serve static frontend files
app.use(express.static(path.resolve(__dirname, '../frontend')));

// Helper function to execute the C backend binary
function runBackend(args) {
    return new Promise((resolve, reject) => {
        execFile(TRIAGE_EXE, args, { cwd: TRIAGE_CWD }, (error, stdout, stderr) => {
            if (error && stdout === '') {
                return reject(new Error(`Failed to execute C backend: ${error.message}. Stderr: ${stderr}`));
            }
            
            try {
                const json = JSON.parse(stdout);
                resolve(json);
            } catch (parseError) {
                if (error) {
                    reject(new Error(`C Backend error (code ${error.code}): ${stderr || stdout}`));
                } else {
                    reject(new Error(`Failed to parse C backend output as JSON: ${stdout}`));
                }
            }
        });
    });
}

// Helper to write temporary JSON files for add/update operations
async function writeTempJson(data) {
    const tempDir = path.join(TRIAGE_CWD, 'data');
    // Ensure data directory exists
    await fs.mkdir(tempDir, { recursive: true });
    
    const filename = `temp_${Date.now()}_${Math.random().toString(36).substr(2, 5)}.json`;
    const filepath = path.join(tempDir, filename);
    
    await fs.writeFile(filepath, JSON.stringify(data, null, 2), 'utf8');
    return filepath;
}

// ==========================================
// API ROUTE ENDPOINTS
// ==========================================

// GET /api/queue -> Get priority queue
app.get('/api/queue', async (req, res) => {
    try {
        const data = await runBackend(['queue']);
        res.json(data);
    } catch (error) {
        res.status(500).json({ status: 'error', message: error.message });
    }
});

// POST /api/patient -> Register new patient
app.post('/api/patient', async (req, res) => {
    let tempFile = null;
    try {
        // Write the request body to a temp JSON file
        tempFile = await writeTempJson(req.body);
        
        // Execute the C backend: triage add <tempFile>
        // Resolve tempFile path relative to the process cwd of C program or use absolute path
        const result = await runBackend(['add', tempFile]);
        res.status(201).json(result);
    } catch (error) {
        res.status(500).json({ status: 'error', message: error.message });
    } finally {
        if (tempFile) {
            try {
                await fs.unlink(tempFile);
            } catch (err) {
                console.error(`Failed to delete temp file ${tempFile}:`, err);
            }
        }
    }
});

// GET /api/patient/:id -> Search patient by ID
app.get('/api/patient/:id', async (req, res) => {
    try {
        const result = await runBackend(['search', req.params.id]);
        res.json(result);
    } catch (error) {
        res.status(500).json({ status: 'error', message: error.message });
    }
});

// PUT /api/patient/:id -> Update patient vitals/symptoms
app.put('/api/patient/:id', async (req, res) => {
    let tempFile = null;
    try {
        // Merge the ID from the URL path parameter into the update body
        const updateData = { ...req.body, id: req.params.id };
        tempFile = await writeTempJson(updateData);
        
        // Execute: triage update <tempFile>
        const result = await runBackend(['update', tempFile]);
        res.json(result);
    } catch (error) {
        res.status(500).json({ status: 'error', message: error.message });
    } finally {
        if (tempFile) {
            try {
                await fs.unlink(tempFile);
            } catch (err) {
                console.error(`Failed to delete temp file ${tempFile}:`, err);
            }
        }
    }
});

// DELETE /api/patient/next -> Treat next patient
app.delete('/api/patient/next', async (req, res) => {
    try {
        const result = await runBackend(['treat']);
        res.json(result);
    } catch (error) {
        res.status(500).json({ status: 'error', message: error.message });
    }
});

// GET /api/reports -> View analytics and statistics
app.get('/api/reports', async (req, res) => {
    try {
        const result = await runBackend(['report']);
        res.json(result);
    } catch (error) {
        res.status(500).json({ status: 'error', message: error.message });
    }
});

// POST /api/save -> Trigger database save (Auto-saved by default, acts as check)
app.post('/api/save', (req, res) => {
    res.json({ status: 'success', message: 'Data is automatically persistent in pure C text database.' });
});

// POST /api/load -> Trigger reload / backup check
app.post('/api/load', async (req, res) => {
    try {
        // In our C implementation, load is called on every run. We run backup as a sync action here
        const result = await runBackend(['backup']);
        res.json({
            status: 'success',
            message: 'Patient database synchronized and current state backed up.',
            backup: result
        });
    } catch (error) {
        res.status(500).json({ status: 'error', message: error.message });
    }
});

// Start express server
app.listen(PORT, () => {
    console.log(`==================================================`);
    console.log(` ER Triage System Express Server Running!`);
    console.log(` Port: ${PORT}`);
    console.log(` Local URL: http://localhost:${PORT}`);
    console.log(`==================================================`);
});
