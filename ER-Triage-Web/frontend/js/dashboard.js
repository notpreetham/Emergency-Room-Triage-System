document.addEventListener('DOMContentLoaded', () => {
    loadDashboardData();
    
    // Bind treat next patient button
    const treatBtn = document.getElementById('btn-treat-next');
    if (treatBtn) {
        treatBtn.addEventListener('click', handleTreatNext);
    }
    
    // Bind backup button
    const backupBtn = document.getElementById('btn-backup');
    if (backupBtn) {
        backupBtn.addEventListener('click', handleBackup);
    }
});

async function loadDashboardData() {
    try {
        // Fetch queue and reports in parallel
        const [queueRes, reportsRes] = await Promise.all([
            window.API.getQueue(),
            window.API.getReports()
        ]);
        
        if (queueRes.status === 'success' && reportsRes.status === 'success') {
            updateKpiCards(reportsRes);
            populateTriageTicker(queueRes.queue);
            populatePriorityList(queueRes.queue);
        } else {
            window.showToast('Failed to fetch dashboard data', 'error');
        }
    } catch (error) {
        console.error('Error loading dashboard:', error);
        window.showToast('Error communicating with ER backend', 'error');
    }
}

function updateKpiCards(report) {
    document.getElementById('kpi-total').textContent = report.total_patients || 0;
    document.getElementById('kpi-queue').textContent = report.queue_length || 0;
    document.getElementById('kpi-treated').textContent = report.total_treated || 0;
    document.getElementById('kpi-severity').textContent = (report.average_severity || 0).toFixed(1);
    
    document.getElementById('kpi-critical').textContent = report.critical_count || 0;
    document.getElementById('kpi-serious').textContent = report.serious_count || 0;
    document.getElementById('kpi-moderate').textContent = report.moderate_count || 0;
    document.getElementById('kpi-stable').textContent = report.stable_count || 0;
}

function populateTriageTicker(queue) {
    const tickerContainer = document.getElementById('alert-ticker-container');
    if (!tickerContainer) return;
    
    // Find if there is any critical patient waiting
    const criticalPatients = queue.filter(p => p.triage_level === 'CRITICAL');
    
    if (criticalPatients.length > 0) {
        const topCritical = criticalPatients[0];
        tickerContainer.innerHTML = `
            <div class="alert-ticker">
                <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round">
                    <path d="m21.73 18-8-14a2 2 0 0 0-3.48 0l-8 14A2 2 0 0 0 4 21h16a2 2 0 0 0 1.73-3Z"/>
                    <line x1="12" y1="9" x2="12" y2="13"/>
                    <line x1="12" y1="17" x2="12.01" y2="17"/>
                </svg>
                <span><strong>CRITICAL ALERT:</strong> Patient ${topCritical.name} (${topCritical.id}) is waiting with a severity score of ${topCritical.severity_score}. Action required.</span>
            </div>
        `;
    } else {
        tickerContainer.innerHTML = `
            <div class="alert-ticker" style="background: linear-gradient(90deg, rgba(16, 185, 129, 0.15) 0%, rgba(16, 185, 129, 0.02) 100%); border-color: rgba(16, 185, 129, 0.2);">
                <svg xmlns="http://www.w3.org/2000/svg" width="20" height="20" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2.5" stroke-linecap="round" stroke-linejoin="round" style="color: var(--stable); animation: none;">
                    <circle cx="12" cy="12" r="10"/><path d="m9 12 2 2 4-4"/>
                </svg>
                <span>ER status stable. No critical patients currently in queue.</span>
            </div>
        `;
    }
}

function populatePriorityList(queue) {
    const listBody = document.getElementById('priority-list-body');
    if (!listBody) return;
    
    listBody.innerHTML = '';
    
    // Filter waiting patients and take top 5
    const activeQueue = queue.filter(p => p.status === 'waiting').slice(0, 5);
    
    if (activeQueue.length === 0) {
        listBody.innerHTML = '<div style="padding: 24px; text-align: center; color: var(--text-muted);">Queue is currently empty.</div>';
        return;
    }
    
    activeQueue.forEach(p => {
        const item = document.createElement('div');
        item.className = 'patient-item';
        
        let badgeClass = '';
        if (p.triage_level === 'CRITICAL') badgeClass = 'badge-critical';
        else if (p.triage_level === 'SERIOUS') badgeClass = 'badge-serious';
        else if (p.triage_level === 'MODERATE') badgeClass = 'badge-moderate';
        else badgeClass = 'badge-stable';
        
        item.innerHTML = `
            <div class="patient-item-info">
                <span class="patient-item-title">${p.name} <span style="font-size: 0.8rem; color: var(--text-muted)">(${p.id})</span></span>
                <span class="patient-item-subtitle">${p.symptoms}</span>
            </div>
            <span class="badge ${badgeClass}">${p.triage_level}</span>
        `;
        listBody.appendChild(item);
    });
}

async function handleTreatNext() {
    if (!confirm('Are you sure you want to treat the next highest priority patient?')) return;
    
    try {
        const result = await window.API.treatNextPatient();
        if (result.status === 'success') {
            window.showToast(`Successfully treated patient: ${result.patient.name} (${result.patient.id})`, 'success');
            loadDashboardData();
        } else {
            window.showToast(result.message || 'No patients in the queue to treat', 'warning');
        }
    } catch (error) {
        console.error('Error treating next patient:', error);
        window.showToast('Error executing treatment', 'error');
    }
}

async function handleBackup() {
    try {
        const result = await window.API.triggerBackup();
        if (result.status === 'success') {
            window.showToast('Database backup created successfully!', 'success');
        } else {
            window.showToast('Failed to create database backup', 'error');
        }
    } catch (error) {
        console.error('Error creating backup:', error);
        window.showToast('Error connecting to backend for backup', 'error');
    }
}
