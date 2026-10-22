document.addEventListener('DOMContentLoaded', () => {
    const form = document.getElementById('register-form');
    if (form) {
        form.addEventListener('submit', handleRegisterSubmit);
        
        // Add event listeners to vitals and inputs for on-the-fly simulation
        const inputs = [
            'age', 'symptoms', 'heart_rate', 'oxygen_level', 
            'temperature', 'ambulance_arrival'
        ];
        
        inputs.forEach(id => {
            const el = document.getElementById(id);
            if (el) {
                // Input and change event listeners for instant reactivity
                el.addEventListener('input', runSimulation);
                el.addEventListener('change', runSimulation);
            }
        });
        
        // Run initial simulation
        runSimulation();
    }
});

// Client-Side Triage Rules Simulator
function simulateTriage(data) {
    let score = 0;
    
    const age = parseInt(data.age) || 0;
    const hr = parseInt(data.heart_rate) || 0;
    const o2 = parseInt(data.oxygen_level) || 0;
    const temp = parseFloat(data.temperature) || 0.0;
    const ambulance = parseInt(data.ambulance_arrival) || 0;
    const symptoms = (data.symptoms || '').toLowerCase();
    
    // 1. Oxygen
    if (o2 > 0 && o2 < 90) score += 50;
    
    // 2. Heart Rate
    if (hr > 120) score += 30;
    
    // 3. Age
    if (age > 60) score += 10;
    
    // 4. Temperature
    if (temp > 102.0) score += 15;
    
    // 5. Symptoms
    if (symptoms) {
        if (symptoms.includes('chest pain')) score += 25;
        
        if (symptoms.includes('difficulty breathing') || 
            symptoms.includes('shortness of breath') || 
            symptoms.includes('difficulty in breathing') || 
            symptoms.includes('breathing difficulty')) {
            score += 30;
        }
        
        if (symptoms.includes('unconscious') || 
            symptoms.includes('fainted') || 
            symptoms.includes('passed out') || 
            symptoms.includes('loss of consciousness')) {
            score += 50;
        }
        
        if (symptoms.includes('heavy bleeding') || 
            symptoms.includes('hemorrhage') || 
            symptoms.includes('bleeding heavily')) {
            score += 40;
        }
        
        if (symptoms.includes('stroke') || 
            symptoms.includes('numbness') || 
            symptoms.includes('slurred speech') || 
            symptoms.includes('facial droop') || 
            symptoms.includes('weakness on one side')) {
            score += 50;
        }
    }
    
    // 6. Transport
    if (ambulance === 1) score += 20;
    
    // Triage Level
    let level = 'STABLE';
    let waitTime = 120;
    if (score >= 80) {
        level = 'CRITICAL';
        waitTime = 0;
    } else if (score >= 50) {
        level = 'SERIOUS';
        waitTime = 15; // base wait time
    } else if (score >= 20) {
        level = 'MODERATE';
        waitTime = 60; // base wait time
    }
    
    return { score, level, waitTime };
}

function runSimulation() {
    const ageVal = document.getElementById('age').value;
    const hrVal = document.getElementById('heart_rate').value;
    const o2Val = document.getElementById('oxygen_level').value;
    const tempVal = document.getElementById('temperature').value;
    const symVal = document.getElementById('symptoms').value;
    const ambVal = document.getElementById('ambulance_arrival').value;
    
    const simResult = simulateTriage({
        age: ageVal,
        heart_rate: hrVal,
        oxygen_level: o2Val,
        temperature: tempVal,
        symptoms: symVal,
        ambulance_arrival: ambVal
    });
    
    // Update Triage Simulation Card
    const ringElement = document.getElementById('sim-score-ring');
    const scoreElement = document.getElementById('sim-score-value');
    const levelElement = document.getElementById('sim-triage-level');
    const waitElement = document.getElementById('sim-wait-time');
    
    if (scoreElement) scoreElement.textContent = simResult.score;
    
    if (levelElement) {
        levelElement.textContent = simResult.level;
        
        // Remove old classes and apply current triage colors
        levelElement.style.color = '';
        ringElement.style.borderColor = '';
        ringElement.style.boxShadow = '';
        
        if (simResult.level === 'CRITICAL') {
            levelElement.style.color = 'var(--critical)';
            ringElement.style.borderColor = 'var(--critical)';
            ringElement.style.boxShadow = '0 0 15px var(--critical-glow)';
        } else if (simResult.level === 'SERIOUS') {
            levelElement.style.color = 'var(--serious)';
            ringElement.style.borderColor = 'var(--serious)';
            ringElement.style.boxShadow = '0 0 15px var(--serious-glow)';
        } else if (simResult.level === 'MODERATE') {
            levelElement.style.color = 'var(--moderate)';
            ringElement.style.borderColor = 'var(--moderate)';
            ringElement.style.boxShadow = '0 0 15px var(--moderate-glow)';
        } else {
            levelElement.style.color = 'var(--stable)';
            ringElement.style.borderColor = 'var(--stable)';
            ringElement.style.boxShadow = '0 0 15px var(--stable-glow)';
        }
    }
    
    if (waitElement) {
        waitElement.textContent = simResult.level === 'CRITICAL' ? 'Immediate' : `${simResult.waitTime}+ mins`;
    }
}

async function handleRegisterSubmit(e) {
    e.preventDefault();
    
    const submitBtn = document.getElementById('btn-submit-register');
    if (submitBtn) {
        submitBtn.disabled = true;
        submitBtn.textContent = 'Registering...';
    }
    
    // Build patient data object
    const pData = {
        id: document.getElementById('patient_id').value.trim() || undefined,
        name: document.getElementById('name').value.trim(),
        age: parseInt(document.getElementById('age').value),
        gender: document.getElementById('gender').value,
        symptoms: document.getElementById('symptoms').value.trim(),
        heart_rate: parseInt(document.getElementById('heart_rate').value),
        oxygen_level: parseInt(document.getElementById('oxygen_level').value),
        temperature: parseFloat(document.getElementById('temperature').value),
        blood_pressure: document.getElementById('blood_pressure').value.trim() || '120/80',
        ambulance_arrival: parseInt(document.getElementById('ambulance_arrival').value)
    };
    
    try {
        const result = await window.API.registerPatient(pData);
        if (result.status === 'success') {
            window.showToast(`Registered successfully! Assigned ID: ${result.patient.id}`, 'success');
            
            // Wait 1.5 seconds and redirect to queue board
            setTimeout(() => {
                window.location.href = 'queue.html';
            }, 1500);
        } else {
            window.showToast(result.message || 'Registration failed', 'error');
            if (submitBtn) {
                submitBtn.disabled = false;
                submitBtn.textContent = 'Register Patient';
            }
        }
    } catch (error) {
        console.error('Error submitting patient registration:', error);
        window.showToast('Server communications error', 'error');
        if (submitBtn) {
            submitBtn.disabled = false;
            submitBtn.textContent = 'Register Patient';
        }
    }
}
