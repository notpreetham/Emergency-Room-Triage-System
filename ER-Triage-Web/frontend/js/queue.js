document.addEventListener('DOMContentLoaded', () => {
    loadQueue();
    
    // Bind search input filter
    const searchInput = document.getElementById('queue-search');
    if (searchInput) {
        searchInput.addEventListener('input', filterQueue);
    }
});

let allPatients = [];

async function loadQueue() {
    const tableBody = document.getElementById('queue-table-body');
    if (!tableBody) return;
    
    // Show spinner
    tableBody.innerHTML = `
        <tr>
            <td colspan="7" style="text-align: center;">
                <div class="spinner"></div>
                <span>Loading active patient queue...</span>
            </td>
        </tr>
    `;
    
    try {
        const result = await window.API.getQueue();
        if (result.status === 'success') {
            allPatients = result.queue || [];
            renderQueue(allPatients);
        } else {
            tableBody.innerHTML = `
                <tr>
                    <td colspan="7" style="text-align: center; color: var(--critical);">
                        Failed to load queue: ${result.message}
                    </td>
                </tr>
            `;
        }
    } catch (error) {
        console.error('Error loading queue:', error);
        tableBody.innerHTML = `
            <tr>
                <td colspan="7" style="text-align: center; color: var(--critical);">
                    Error connecting to ER Triage server.
                </td>
            </tr>
        `;
    }
}

function renderQueue(patients) {
    const tableBody = document.getElementById('queue-table-body');
    if (!tableBody) return;
    
    tableBody.innerHTML = '';
    
    if (patients.length === 0) {
        tableBody.innerHTML = `
            <tr>
                <td colspan="7" style="text-align: center; padding: 30px; color: var(--text-secondary);">
                    No patients currently waiting in the triage queue.
                </td>
            </tr>
        `;
        return;
    }
    
    patients.forEach(p => {
        const tr = document.createElement('tr');
        
        let badgeClass = '';
        if (p.triage_level === 'CRITICAL') badgeClass = 'badge-critical';
        else if (p.triage_level === 'SERIOUS') badgeClass = 'badge-serious';
        else if (p.triage_level === 'MODERATE') badgeClass = 'badge-moderate';
        else badgeClass = 'badge-stable';
        
        // Color row left border based on triage severity
        let leftBorderColor = 'transparent';
        if (p.triage_level === 'CRITICAL') leftBorderColor = 'var(--critical)';
        else if (p.triage_level === 'SERIOUS') leftBorderColor = 'var(--serious)';
        else if (p.triage_level === 'MODERATE') leftBorderColor = 'var(--moderate)';
        else leftBorderColor = 'var(--stable)';
        
        tr.style.borderLeft = `3px solid ${leftBorderColor}`;
        
        tr.innerHTML = `
            <td><strong>${p.id}</strong></td>
            <td><strong>${p.name}</strong></td>
            <td>${p.age} yrs (${p.gender})</td>
            <td><span class="badge ${badgeClass}">${p.triage_level}</span></td>
            <td><strong>${p.severity_score}</strong></td>
            <td><strong>${p.estimated_wait_time} mins</strong></td>
            <td>
                <div class="table-actions">
                    <button class="btn-icon icon-view" title="View Patient Details" onclick="viewPatient('${p.id}')">
                        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                            <path d="M2.062 12.348a1 1 0 0 1 0-.696 10.75 10.75 0 0 1 19.876 0 1 1 0 0 1 0 .696 10.75 10.75 0 0 1-19.876 0z"/>
                            <circle cx="12" cy="12" r="3"/>
                        </svg>
                    </button>
                    <button class="btn-icon icon-edit" title="Update Vitals" onclick="editPatient('${p.id}')">
                        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                            <path d="M12 20h9"/><path d="M16.5 3.5a2.12 2.12 0 0 1 3 3L7 19l-4 1 1-4Z"/>
                        </svg>
                    </button>
                    <button class="btn-icon icon-treat" title="Mark Treated" onclick="treatPatient('${p.id}', '${p.name}')">
                        <svg xmlns="http://www.w3.org/2000/svg" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round">
                            <path d="M19 14c1.49-1.46 3-3.21 3-5.5A5.5 5.5 0 0 0 16.5 3c-1.76 0-3 .5-4.5 2-1.5-1.5-2.74-2-4.5-2A5.5 5.5 0 0 0 2 8.5c0 2.3 1.5 4.05 3 5.5l7 7Z"/>
                        </svg>
                    </button>
                </div>
            </td>
        `;
        tableBody.appendChild(tr);
    });
}

function filterQueue(e) {
    const query = e.target.value.toLowerCase().trim();
    if (query === '') {
        renderQueue(allPatients);
        return;
    }
    
    const filtered = allPatients.filter(p => 
        p.id.toLowerCase().includes(query) || 
        p.name.toLowerCase().includes(query) ||
        p.symptoms.toLowerCase().includes(query) ||
        p.triage_level.toLowerCase().includes(query)
    );
    renderQueue(filtered);
}

function viewPatient(id) {
    window.location.href = `patient.html?id=${id}`;
}

function editPatient(id) {
    window.location.href = `patient.html?id=${id}&edit=true`;
}

async function treatPatient(id, name) {
    // If patient is at index 0 (highest priority), we can use the DELETE endpoint.
    // Otherwise, we can treat them by sending a PUT request to update their status.
    const isFirst = allPatients.length > 0 && allPatients[0].id === id;
    
    if (!confirm(`Mark patient ${name} (${id}) as treated and remove from queue?`)) return;
    
    try {
        let result;
        if (isFirst) {
            result = await window.API.treatNextPatient();
        } else {
            // Treat specific patient via update route
            result = await window.API.updatePatient(id, { status: 'treated' });
        }
        
        if (result.status === 'success') {
            window.showToast(`Patient ${name} treated successfully!`, 'success');
            loadQueue();
        } else {
            window.showToast(result.message || 'Failed to treat patient', 'error');
        }
    } catch (error) {
        console.error('Error treating patient:', error);
        window.showToast('Communication error during treatment', 'error');
    }
}

// Make functions globally accessible for inline onclick triggers
window.viewPatient = viewPatient;
window.editPatient = editPatient;
window.treatPatient = treatPatient;
