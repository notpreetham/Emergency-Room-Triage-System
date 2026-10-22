#ifndef QUEUE_H
#define QUEUE_H

#include "triage.h"

typedef struct PatientNode {
    Patient patient;
    struct PatientNode *next;
} PatientNode;

// Add a patient to the queue, keeping it sorted by priority
// (Severity Score descending, then Arrival Time ascending)
int add_patient(PatientNode **head, Patient patient);

// Remove a patient from the queue by ID
int remove_patient(PatientNode **head, const char *id);

// Pop the highest priority patient from the queue
PatientNode* pop_next_patient(PatientNode **head);

// Search for a patient in the queue by ID
PatientNode* search_patient(PatientNode *head, const char *id);

// Search for patients by ID or Name substring
// Output matches to a preallocated array of Patient pointers
int search_patients_substring(PatientNode *head, const char *query, PatientNode **results, int max_results);

// Update a patient's vitals/symptoms and dynamically re-prioritize
int update_patient(PatientNode **head, const char *id, const Patient *updated_data);

// Recalculate estimated wait times for all patients in the queue
void recalculate_wait_times(PatientNode *head);

// Free all memory allocated for the queue
void free_queue(PatientNode *head);

// Sort the queue in-place (used if loaded in bulk)
void sort_queue(PatientNode **head);

#endif // QUEUE_H
