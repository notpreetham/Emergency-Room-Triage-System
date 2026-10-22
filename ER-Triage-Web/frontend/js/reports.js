document.addEventListener('DOMContentLoaded', () => {
    loadReportsData();
});

let triageChart = null;
let symptomChart = null;
let compositionChart = null;
let severityDistChart = null;

async function loadReportsData() {
    const reportContainer = document.getElementById('report-dashboard-view');
    if (!reportContainer) return;
    
    try {
        const [queueRes, reportsRes] = await Promise.all([
            window.API.getQueue(),
            window.API.getReports()
        ]);
        
        if (queueRes.status === 'success' && reportsRes.status === 'success') {
            renderSummaryCards(reportsRes);
            renderCharts(queueRes.queue || [], reportsRes);
        } else {
            window.showToast('Failed to compile clinical reports', 'error');
        }
    } catch (error) {
        console.error('Error generating reports:', error);
        window.showToast('Error connecting to ER Triage server', 'error');
    }
}

function renderSummaryCards(report) {
    document.getElementById('rep-total-registered').textContent = report.total_patients || 0;
    document.getElementById('rep-total-treated').textContent = report.total_treated || 0;
    document.getElementById('rep-active-waiting').textContent = report.queue_length || 0;
    document.getElementById('rep-avg-severity').textContent = (report.average_severity || 0).toFixed(2);
}

function renderCharts(queue, report) {
    // Check if Chart.js is loaded
    if (typeof Chart === 'undefined') {
        console.error('Chart.js library not loaded');
        window.showToast('Chart.js library missing. Cannot render graphics.', 'warning');
        return;
    }
    
    // --- Chart 1: Patients by Triage Category ---
    const ctxTriage = document.getElementById('chart-triage-category').getContext('2d');
    if (triageChart) triageChart.destroy();
    
    triageChart = new Chart(ctxTriage, {
        type: 'doughnut',
        data: {
            labels: ['Critical', 'Serious', 'Moderate', 'Stable'],
            datasets: [{
                data: [
                    report.critical_count || 0,
                    report.serious_count || 0,
                    report.moderate_count || 0,
                    report.stable_count || 0
                ],
                backgroundColor: ['#ef4444', '#f97316', '#eab308', '#10b981'],
                borderColor: 'rgba(255, 255, 255, 0.08)',
                borderWidth: 1
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    position: 'bottom',
                    labels: { color: '#f3f4f6', font: { family: 'Plus Jakarta Sans', size: 11 } }
                }
            }
        }
    });

    // --- Chart 2: Top Common Symptoms ---
    const ctxSymptom = document.getElementById('chart-symptom-frequency').getContext('2d');
    if (symptomChart) symptomChart.destroy();
    
    const symptomsLabels = report.common_symptoms ? report.common_symptoms.map(s => s.symptom || s) : ['None', 'None', 'None'];
    const symptomsData = report.common_symptoms_counts ? report.common_symptoms_counts : (report.common_symptoms ? report.common_symptoms.map(s => s.count || 0) : [0, 0, 0]);
    
    symptomChart = new Chart(ctxSymptom, {
        type: 'bar',
        data: {
            labels: symptomsLabels,
            datasets: [{
                label: 'Patient Count',
                data: symptomsData,
                backgroundColor: 'rgba(59, 130, 246, 0.65)',
                borderColor: '#3b82f6',
                borderWidth: 1.5,
                borderRadius: 4
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: { display: false }
            },
            scales: {
                y: {
                    grid: { color: 'rgba(255, 255, 255, 0.06)' },
                    ticks: { color: '#9ca3af', stepSize: 1 }
                },
                x: {
                    grid: { display: false },
                    ticks: { color: '#9ca3af' }
                }
            }
        }
    });

    // --- Chart 3: Queue Composition (Active vs Treated) ---
    const ctxComposition = document.getElementById('chart-queue-composition').getContext('2d');
    if (compositionChart) compositionChart.destroy();
    
    compositionChart = new Chart(ctxComposition, {
        type: 'pie',
        data: {
            labels: ['Active Waiting', 'Treated Hist.'],
            datasets: [{
                data: [report.queue_length || 0, report.total_treated || 0],
                backgroundColor: ['#3b82f6', 'rgba(255, 255, 255, 0.15)'],
                borderColor: 'rgba(255, 255, 255, 0.08)',
                borderWidth: 1
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: {
                    position: 'bottom',
                    labels: { color: '#f3f4f6', font: { family: 'Plus Jakarta Sans', size: 11 } }
                }
            }
        }
    });

    // --- Chart 4: Severity Deciles Distribution ---
    const ctxSeverityDist = document.getElementById('chart-severity-distribution').getContext('2d');
    if (severityDistChart) severityDistChart.destroy();
    
    // Group active queue severity scores in ranges: 0-19, 20-39, 40-59, 60-79, 80-100
    const deciles = [0, 0, 0, 0, 0];
    queue.forEach(p => {
        const score = p.severity_score;
        if (score >= 80) deciles[4]++;
        else if (score >= 60) deciles[3]++;
        else if (score >= 40) deciles[2]++;
        else if (score >= 20) deciles[1]++;
        else deciles[0]++;
    });

    severityDistChart = new Chart(ctxSeverityDist, {
        type: 'line',
        data: {
            labels: ['0-19 (Stable)', '20-39 (Moderate)', '40-59 (Serious)', '60-79 (Urgent)', '80+ (Critical)'],
            datasets: [{
                label: 'Active Queue Load',
                data: deciles,
                borderColor: '#10b981',
                backgroundColor: 'rgba(16, 185, 129, 0.15)',
                borderWidth: 2,
                fill: true,
                tension: 0.35,
                pointBackgroundColor: '#10b981'
            }]
        },
        options: {
            responsive: true,
            maintainAspectRatio: false,
            plugins: {
                legend: { display: false }
            },
            scales: {
                y: {
                    grid: { color: 'rgba(255, 255, 255, 0.06)' },
                    ticks: { color: '#9ca3af', stepSize: 1 }
                },
                x: {
                    grid: { display: false },
                    ticks: { color: '#9ca3af' }
                }
            }
        }
    });
}
