#include "reports.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

void generate_report(PatientNode *head, TriageReport *report) {
    memset(report, 0, sizeof(TriageReport));
    
    int severity_sum = 0;
    int active_count = 0;
    
    // Predefined common emergency symptoms to track and rank
    const char *symptom_keys[] = {
        "Chest pain", "Difficulty breathing", "Unconscious", "Heavy bleeding",
        "Stroke symptoms", "Fever", "Cough", "Headache", "Nausea",
        "Dizziness", "Abdominal pain", "Fracture"
    };
    int symptom_counts[12] = {0};
    int num_symptoms = 12;
    
    PatientNode *curr = head;
    while (curr) {
        report->total_patients++;
        
        if (strcmp(curr->patient.status, "treated") == 0) {
            report->total_treated++;
        } else if (strcmp(curr->patient.status, "waiting") == 0) {
            report->queue_length++;
            severity_sum += curr->patient.severity_score;
            active_count++;
            
            if (strcmp(curr->patient.triage_level, "CRITICAL") == 0) {
                report->critical_count++;
            } else if (strcmp(curr->patient.triage_level, "SERIOUS") == 0) {
                report->serious_count++;
            } else if (strcmp(curr->patient.triage_level, "MODERATE") == 0) {
                report->moderate_count++;
            } else if (strcmp(curr->patient.triage_level, "STABLE") == 0) {
                report->stable_count++;
            }
        }
        
        // Scan symptoms (both active and treated for complete clinical stats)
        for (int i = 0; i < num_symptoms; i++) {
            if (strcasestr_custom(curr->patient.symptoms, symptom_keys[i])) {
                symptom_counts[i]++;
            }
        }
        
        curr = curr->next;
    }
    
    if (active_count > 0) {
        report->average_severity = (float)severity_sum / active_count;
    } else {
        report->average_severity = 0.0f;
    }
    
    // Extract top 3 symptoms by frequency
    for (int top = 0; top < 3; top++) {
        int max_idx = -1;
        int max_val = -1;
        for (int i = 0; i < num_symptoms; i++) {
            if (symptom_counts[i] > max_val) {
                max_val = symptom_counts[i];
                max_idx = i;
            }
        }
        if (max_idx != -1 && max_val > 0) {
            safe_strcpy(report->common_symptoms[top], symptom_keys[max_idx], sizeof(report->common_symptoms[top]));
            report->common_symptoms_counts[top] = max_val;
            symptom_counts[max_idx] = -1; // Remove from next search iteration
        } else {
            safe_strcpy(report->common_symptoms[top], "None", sizeof(report->common_symptoms[top]));
            report->common_symptoms_counts[top] = 0;
        }
    }
}

void print_report_json(const TriageReport *report) {
    printf("{\n");
    printf("  \"status\": \"success\",\n");
    printf("  \"total_patients\": %d,\n", report->total_patients);
    printf("  \"total_treated\": %d,\n", report->total_treated);
    printf("  \"queue_length\": %d,\n", report->queue_length);
    printf("  \"critical_count\": %d,\n", report->critical_count);
    printf("  \"serious_count\": %d,\n", report->serious_count);
    printf("  \"moderate_count\": %d,\n", report->moderate_count);
    printf("  \"stable_count\": %d,\n", report->stable_count);
    printf("  \"average_severity\": %.2f,\n", report->average_severity);
    printf("  \"common_symptoms\": [\n");
    for (int i = 0; i < 3; i++) {
        printf("    {\n");
        printf("      \"symptom\": \"%s\",\n", report->common_symptoms[i]);
        printf("      \"count\": %d\n", report->common_symptoms_counts[i]);
        printf("    }%s\n", (i < 2) ? "," : "");
    }
    printf("  ]\n");
    printf("}\n");
}

void print_report_human(const TriageReport *report) {
    printf("\n============================================\n");
    printf("       EMERGENCY ROOM STATUS REPORT         \n");
    printf("============================================\n");
    printf("Total Patients Registered : %d\n", report->total_patients);
    printf("Patients Treated          : %d\n", report->total_treated);
    printf("Patients in Queue         : %d\n", report->queue_length);
    printf("Average Severity Score    : %.2f\n", report->average_severity);
    printf("\nQueue Composition:\n");
    printf("  - CRITICAL (Score 80+)  : %d\n", report->critical_count);
    printf("  - SERIOUS  (Score 50-79): %d\n", report->serious_count);
    printf("  - MODERATE (Score 20-49): %d\n", report->moderate_count);
    printf("  - STABLE   (Score <20)  : %d\n", report->stable_count);
    printf("\nTop 3 Most Common Symptoms:\n");
    for (int i = 0; i < 3; i++) {
        if (report->common_symptoms_counts[i] > 0) {
            printf("  %d. %s (%d patient%s)\n", i + 1, 
                   report->common_symptoms[i], 
                   report->common_symptoms_counts[i],
                   (report->common_symptoms_counts[i] > 1) ? "s" : "");
        } else {
            printf("  %d. N/A\n", i + 1);
        }
    }
    printf("============================================\n\n");
}
