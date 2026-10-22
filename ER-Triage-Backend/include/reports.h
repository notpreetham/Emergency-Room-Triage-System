#ifndef REPORTS_H
#define REPORTS_H

#include "queue.h"

typedef struct {
    int total_patients;
    int total_treated;
    int critical_count;
    int serious_count;
    int moderate_count;
    int stable_count;
    float average_severity;
    int queue_length;
    char common_symptoms[3][64]; // Top 3 common symptoms
    int common_symptoms_counts[3];
} TriageReport;

// Generate a summary report from the patient linked list
void generate_report(PatientNode *head, TriageReport *report);

// Output the report as a JSON string to stdout
void print_report_json(const TriageReport *report);

// Output the report in a professional, human-readable terminal format
void print_report_human(const TriageReport *report);

#endif // REPORTS_H
