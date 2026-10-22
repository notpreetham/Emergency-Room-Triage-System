#include "queue.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Helper to compute dynamic escalated severity score based on waiting time.
// Adds 5 points to raw severity score for every 15 minutes (900 seconds) waited.
static int get_effective_severity(const Patient *p, long long current_time) {
    if (strcmp(p->status, "treated") == 0) {
        return p->severity_score; // Treated patients do not escalate
    }
    long long seconds_waited = current_time - p->arrival_time;
    if (seconds_waited < 0) seconds_waited = 0;
    
    int escalation_intervals = (int)(seconds_waited / 900); // 15-minute intervals
    int escalation_points = escalation_intervals * 5;
    
    return p->severity_score + escalation_points;
}

// Comparison function for sorting/prioritizing patients.
// Returns:
//   < 0 if a has higher priority than b (should come before b)
//   > 0 if a has lower priority than b (should come after b)
//   0 if they have equal priority
static int compare_patients(const Patient *a, const Patient *b) {
    // 1. Status priority: "waiting" patients always come before "treated" patients
    int a_waiting = (strcmp(a->status, "waiting") == 0);
    int b_waiting = (strcmp(b->status, "waiting") == 0);
    if (a_waiting != b_waiting) {
        return b_waiting - a_waiting; // waiting (1) comes before treated (0) -> returns negative
    }
    
    // 2. If both are waiting, prioritize by Effective Severity Score (raw + waiting-time escalation)
    if (a_waiting) {
        long long now = get_current_timestamp();
        int a_eff = get_effective_severity(a, now);
        int b_eff = get_effective_severity(b, now);
        
        if (a_eff != b_eff) {
            return b_eff - a_eff; // higher effective score comes first -> returns negative
        }
        // 3. Tie-breaker: Arrival Time (ascending - earlier time first)
        if (a->arrival_time != b->arrival_time) {
            return (a->arrival_time < b->arrival_time) ? -1 : 1;
        }
    } else {
        // For treated patients, sort by arrival time descending (newest treated at the end of the file)
        if (a->arrival_time != b->arrival_time) {
            return (b->arrival_time < a->arrival_time) ? -1 : 1;
        }
    }
    return 0;
}

int add_patient(PatientNode **head, Patient patient) {
    if (!head) return 0;
    
    // Recalculate score and level for consistency
    patient.severity_score = calculate_severity(&patient);
    assign_triage_level(patient.severity_score, patient.triage_level, sizeof(patient.triage_level));
    
    // Default status if empty
    if (patient.status[0] == '\0') {
        safe_strcpy(patient.status, "waiting", sizeof(patient.status));
    }
    
    // Double check if patient already exists
    PatientNode *existing = search_patient(*head, patient.id);
    if (existing) {
        return 0; // Prevent duplicate IDs in add mode
    }

    PatientNode *new_node = (PatientNode *)malloc(sizeof(PatientNode));
    if (!new_node) return 0;
    
    new_node->patient = patient;
    new_node->next = NULL;

    // Insert in sorted order
    if (!*head || compare_patients(&new_node->patient, &(*head)->patient) < 0) {
        new_node->next = *head;
        *head = new_node;
    } else {
        PatientNode *curr = *head;
        while (curr->next && compare_patients(&new_node->patient, &curr->next->patient) >= 0) {
            curr = curr->next;
        }
        new_node->next = curr->next;
        curr->next = new_node;
    }

    recalculate_wait_times(*head);
    return 1;
}

int remove_patient(PatientNode **head, const char *id) {
    if (!head || !*head || !id) return 0;

    PatientNode *prev = NULL;
    PatientNode *curr = *head;

    while (curr && strcmp(curr->patient.id, id) != 0) {
        prev = curr;
        curr = curr->next;
    }

    if (!curr) return 0; // Patient not found

    if (prev) {
        prev->next = curr->next;
    } else {
        *head = curr->next;
    }

    free(curr);
    recalculate_wait_times(*head);
    return 1;
}

PatientNode* pop_next_patient(PatientNode **head) {
    if (!head || !*head) return NULL;

    // Find the first patient with status "waiting"
    PatientNode *prev = NULL;
    PatientNode *curr = *head;

    while (curr && strcmp(curr->patient.status, "waiting") != 0) {
        prev = curr;
        curr = curr->next;
    }

    if (!curr) return NULL; // No waiting patients

    // Unlink the node
    if (prev) {
        prev->next = curr->next;
    } else {
        *head = curr->next;
    }
    curr->next = NULL;

    recalculate_wait_times(*head);
    return curr;
}

PatientNode* search_patient(PatientNode *head, const char *id) {
    if (!id) return NULL;
    
    PatientNode *curr = head;
    while (curr) {
        if (strcmp(curr->patient.id, id) == 0) {
            return curr;
        }
        curr = curr->next;
    }
    return NULL;
}

int search_patients_substring(PatientNode *head, const char *query, PatientNode **results, int max_results) {
    if (!query || !results || max_results <= 0) return 0;

    int count = 0;
    PatientNode *curr = head;

    while (curr && count < max_results) {
        if (strcasestr_custom(curr->patient.id, query) || 
            strcasestr_custom(curr->patient.name, query)) {
            results[count++] = curr;
        }
        curr = curr->next;
    }
    return count;
}

int update_patient(PatientNode **head, const char *id, const Patient *updated_data) {
    if (!head || !*head || !id || !updated_data) return 0;

    PatientNode *prev = NULL;
    PatientNode *curr = *head;

    while (curr && strcmp(curr->patient.id, id) != 0) {
        prev = curr;
        curr = curr->next;
    }

    if (!curr) return 0; // Patient not found

    // Update patient clinical fields
    if (updated_data->name[0] != '\0') {
        safe_strcpy(curr->patient.name, updated_data->name, sizeof(curr->patient.name));
    }
    if (updated_data->age > 0) {
        curr->patient.age = updated_data->age;
    }
    if (updated_data->gender[0] != '\0') {
        safe_strcpy(curr->patient.gender, updated_data->gender, sizeof(curr->patient.gender));
    }
    if (updated_data->symptoms[0] != '\0') {
        safe_strcpy(curr->patient.symptoms, updated_data->symptoms, sizeof(curr->patient.symptoms));
    }
    if (updated_data->heart_rate > 0) {
        curr->patient.heart_rate = updated_data->heart_rate;
    }
    if (updated_data->oxygen_level > 0) {
        curr->patient.oxygen_level = updated_data->oxygen_level;
    }
    if (updated_data->temperature > 0.0f) {
        curr->patient.temperature = updated_data->temperature;
    }
    if (updated_data->blood_pressure[0] != '\0') {
        safe_strcpy(curr->patient.blood_pressure, updated_data->blood_pressure, sizeof(curr->patient.blood_pressure));
    }
    if (updated_data->arrival_time > 0) {
        curr->patient.arrival_time = updated_data->arrival_time;
        format_timestamp(curr->patient.arrival_time, curr->patient.arrival_time_str, sizeof(curr->patient.arrival_time_str));
    }
    if (updated_data->ambulance_arrival >= 0) {
        curr->patient.ambulance_arrival = updated_data->ambulance_arrival;
    }
    if (updated_data->status[0] != '\0') {
        safe_strcpy(curr->patient.status, updated_data->status, sizeof(curr->patient.status));
    }

    // Recalculate score and level
    curr->patient.severity_score = calculate_severity(&curr->patient);
    assign_triage_level(curr->patient.severity_score, curr->patient.triage_level, sizeof(curr->patient.triage_level));

    // Remove node from its current spot
    if (prev) {
        prev->next = curr->next;
    } else {
        *head = curr->next;
    }
    curr->next = NULL;

    // Re-insert node in its newly sorted position
    PatientNode *new_head = *head;
    if (!new_head || compare_patients(&curr->patient, &new_head->patient) < 0) {
        curr->next = new_head;
        *head = curr;
    } else {
        PatientNode *temp = new_head;
        while (temp->next && compare_patients(&curr->patient, &temp->next->patient) >= 0) {
            temp = temp->next;
        }
        curr->next = temp->next;
        temp->next = curr;
    }

    // Recalculate wait times for all patients
    recalculate_wait_times(*head);
    return 1;
}

void recalculate_wait_times(PatientNode *head) {
    int critical_serious_ahead = 0;
    int moderate_or_higher_ahead = 0;
    int total_ahead = 0;

    PatientNode *curr = head;
    while (curr) {
        // Skip treated patients (who reside at the end)
        if (strcmp(curr->patient.status, "waiting") != 0) {
            curr->patient.estimated_wait_time = 0;
            curr = curr->next;
            continue;
        }

        // Apply formula based on triage category
        if (strcmp(curr->patient.triage_level, "CRITICAL") == 0) {
            curr->patient.estimated_wait_time = 0;
        } else if (strcmp(curr->patient.triage_level, "SERIOUS") == 0) {
            curr->patient.estimated_wait_time = 15 + 10 * critical_serious_ahead;
        } else if (strcmp(curr->patient.triage_level, "MODERATE") == 0) {
            curr->patient.estimated_wait_time = 60 + 15 * moderate_or_higher_ahead;
        } else { // STABLE
            curr->patient.estimated_wait_time = 120 + 20 * total_ahead;
        }

        // Update statistics for patients behind
        total_ahead++;
        if (strcmp(curr->patient.triage_level, "CRITICAL") == 0 || 
            strcmp(curr->patient.triage_level, "SERIOUS") == 0) {
            critical_serious_ahead++;
        }
        if (strcmp(curr->patient.triage_level, "STABLE") != 0) {
            moderate_or_higher_ahead++;
        }

        curr = curr->next;
    }
}

void sort_queue(PatientNode **head) {
    if (!head || !*head || !(*head)->next) return;

    PatientNode *sorted = NULL;
    PatientNode *curr = *head;

    while (curr) {
        PatientNode *next = curr->next;
        curr->next = NULL;

        // Insert curr into the sorted list
        if (!sorted || compare_patients(&curr->patient, &sorted->patient) < 0) {
            curr->next = sorted;
            sorted = curr;
        } else {
            PatientNode *temp = sorted;
            while (temp->next && compare_patients(&curr->patient, &temp->next->patient) >= 0) {
                temp = temp->next;
            }
            curr->next = temp->next;
            temp->next = curr;
        }
        curr = next;
    }
    *head = sorted;
    recalculate_wait_times(*head);
}

void free_queue(PatientNode *head) {
    PatientNode *curr = head;
    while (curr) {
        PatientNode *next = curr->next;
        free(curr);
        curr = next;
    }
}
