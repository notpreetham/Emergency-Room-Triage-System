#include "triage.h"
#include "utils.h"
#include <string.h>

int calculate_severity(const Patient *p) {
    int score = 0;

    // 1. Oxygen level below 90
    // Check oxygen_level is greater than 0 to handle valid input only
    if (p->oxygen_level > 0 && p->oxygen_level < 90) {
        score += 50;
    }

    // 2. Heart rate above 120
    if (p->heart_rate > 120) {
        score += 30;
    }

    // 3. Age above 60
    if (p->age > 60) {
        score += 10;
    }

    // 4. Temperature above 102.0 Fahrenheit
    if (p->temperature > 102.0f) {
        score += 15;
    }

    // 5. Symptom string matching
    if (p->symptoms[0] != '\0') {
        if (strcasestr_custom(p->symptoms, "chest pain")) {
            score += 25;
        }
        if (strcasestr_custom(p->symptoms, "difficulty breathing") || 
            strcasestr_custom(p->symptoms, "shortness of breath") ||
            strcasestr_custom(p->symptoms, "difficulty in breathing") ||
            strcasestr_custom(p->symptoms, "breathing difficulty")) {
            score += 30;
        }
        if (strcasestr_custom(p->symptoms, "unconscious") || 
            strcasestr_custom(p->symptoms, "fainted") ||
            strcasestr_custom(p->symptoms, "passed out") ||
            strcasestr_custom(p->symptoms, "loss of consciousness")) {
            score += 50;
        }
        if (strcasestr_custom(p->symptoms, "heavy bleeding") || 
            strcasestr_custom(p->symptoms, "hemorrhage") ||
            strcasestr_custom(p->symptoms, "bleeding heavily")) {
            score += 40;
        }
        if (strcasestr_custom(p->symptoms, "stroke") || 
            strcasestr_custom(p->symptoms, "numbness") || 
            strcasestr_custom(p->symptoms, "slurred speech") ||
            strcasestr_custom(p->symptoms, "facial droop") ||
            strcasestr_custom(p->symptoms, "weakness on one side")) {
            score += 50;
        }
    }

    // 6. Ambulance arrival
    if (p->ambulance_arrival == 1) {
        score += 20;
    }

    return score;
}

void assign_triage_level(int severity_score, char *dest, int max_len) {
    if (!dest || max_len <= 0) return;
    
    if (severity_score >= 80) {
        safe_strcpy(dest, "CRITICAL", max_len);
    } else if (severity_score >= 50) {
        safe_strcpy(dest, "SERIOUS", max_len);
    } else if (severity_score >= 20) {
        safe_strcpy(dest, "MODERATE", max_len);
    } else {
        safe_strcpy(dest, "STABLE", max_len);
    }
}
