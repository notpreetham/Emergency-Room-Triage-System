#include "fileio.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Custom tokenizer that mimics standard strsep behavior to support empty tokens
static char *split_token(char **stringp, const char *delim) {
    char *s;
    const char *spanp;
    int c, sc;
    char *tok;

    if ((s = *stringp) == NULL)
        return NULL;

    tok = s;
    for (;;) {
        c = *s++;
        spanp = delim;
        do {
            if ((sc = *spanp++) == c) {
                if (c == 0)
                    s = NULL;
                else
                    s[-1] = 0;
                *stringp = s;
                return tok;
            }
        } while (sc != 0);
    }
}

// Sanitizes a field by replacing any pipe '|' characters or newlines with spaces
static void sanitize_field(char *dest, const char *src, int max_len) {
    if (!src || !dest || max_len <= 0) return;
    int d = 0;
    for (int s = 0; src[s] && d < max_len - 1; s++) {
        if (src[s] == '|' || src[s] == '\n' || src[s] == '\r') {
            dest[d++] = ' ';
        } else {
            dest[d++] = src[s];
        }
    }
    dest[d] = '\0';
}

int load_patients(PatientNode **head, const char *filename) {
    FILE *fp = fopen(filename, "r");
    if (!fp) {
        // Return 0 if file does not exist (meaning DB starts empty)
        return 0;
    }
    
    char line[1024];
    int loaded_count = 0;
    
    // Clear list to prevent merging duplicate datasets
    free_queue(*head);
    *head = NULL;
    
    while (fgets(line, sizeof(line), fp)) {
        // Trim line-breaks and leading/trailing whitespace
        trim_whitespace(line);
        if (line[0] == '\0') continue;
        
        Patient p = {0};
        char *running = line;
        char *token;
        
        // 1. Patient ID
        token = split_token(&running, "|");
        if (!token) continue;
        safe_strcpy(p.id, token, sizeof(p.id));
        
        // 2. Name
        token = split_token(&running, "|");
        if (!token) continue;
        safe_strcpy(p.name, token, sizeof(p.name));
        
        // 3. Age
        token = split_token(&running, "|");
        if (!token) continue;
        p.age = atoi(token);
        
        // 4. Gender
        token = split_token(&running, "|");
        if (!token) continue;
        safe_strcpy(p.gender, token, sizeof(p.gender));
        
        // 5. Symptoms
        token = split_token(&running, "|");
        if (!token) continue;
        safe_strcpy(p.symptoms, token, sizeof(p.symptoms));
        
        // 6. Heart Rate
        token = split_token(&running, "|");
        if (!token) continue;
        p.heart_rate = atoi(token);
        
        // 7. Oxygen Level
        token = split_token(&running, "|");
        if (!token) continue;
        p.oxygen_level = atoi(token);
        
        // 8. Temperature
        token = split_token(&running, "|");
        if (!token) continue;
        p.temperature = atof(token);
        
        // 9. Blood Pressure
        token = split_token(&running, "|");
        if (!token) continue;
        safe_strcpy(p.blood_pressure, token, sizeof(p.blood_pressure));
        
        // 10. Arrival Time
        token = split_token(&running, "|");
        if (!token) continue;
        p.arrival_time = strtoll(token, NULL, 10);
        
        // 11. Arrival Time String
        token = split_token(&running, "|");
        if (!token) continue;
        safe_strcpy(p.arrival_time_str, token, sizeof(p.arrival_time_str));
        
        // 12. Ambulance Arrival
        token = split_token(&running, "|");
        if (!token) continue;
        p.ambulance_arrival = atoi(token);
        
        // 13. Severity Score
        token = split_token(&running, "|");
        if (!token) continue;
        p.severity_score = atoi(token);
        
        // 14. Triage Level
        token = split_token(&running, "|");
        if (!token) continue;
        safe_strcpy(p.triage_level, token, sizeof(p.triage_level));
        
        // 15. Estimated Wait Time
        token = split_token(&running, "|");
        if (!token) continue;
        p.estimated_wait_time = atoi(token);
        
        // 16. Status
        token = split_token(&running, "|");
        if (!token) continue;
        safe_strcpy(p.status, token, sizeof(p.status));
        
        // Insert nodes at the head. We'll run a single sort pass afterwards.
        PatientNode *new_node = (PatientNode *)malloc(sizeof(PatientNode));
        if (new_node) {
            new_node->patient = p;
            new_node->next = *head;
            *head = new_node;
            loaded_count++;
        }
    }
    
    fclose(fp);
    
    // Sort and calculate wait times in a single operation
    sort_queue(head);
    
    return loaded_count;
}

int save_patients(PatientNode *head, const char *filename) {
    FILE *fp = fopen(filename, "w");
    if (!fp) {
        // Automatically attempt directory creation (supports Windows command style)
        system("mkdir data 2> NUL");
        fp = fopen(filename, "w");
        if (!fp) {
            return 0;
        }
    }
    
    PatientNode *curr = head;
    while (curr) {
        char s_name[100];
        char s_symptoms[256];
        char s_gender[16];
        char s_bp[16];
        
        // Strip out delimiters
        sanitize_field(s_name, curr->patient.name, sizeof(s_name));
        sanitize_field(s_symptoms, curr->patient.symptoms, sizeof(s_symptoms));
        sanitize_field(s_gender, curr->patient.gender, sizeof(s_gender));
        sanitize_field(s_bp, curr->patient.blood_pressure, sizeof(s_bp));
        
        fprintf(fp, "%s|%s|%d|%s|%s|%d|%d|%.2f|%s|%lld|%s|%d|%d|%s|%d|%s\n",
                curr->patient.id,
                s_name,
                curr->patient.age,
                s_gender,
                s_symptoms,
                curr->patient.heart_rate,
                curr->patient.oxygen_level,
                curr->patient.temperature,
                s_bp,
                curr->patient.arrival_time,
                curr->patient.arrival_time_str,
                curr->patient.ambulance_arrival,
                curr->patient.severity_score,
                curr->patient.triage_level,
                curr->patient.estimated_wait_time,
                curr->patient.status);
        curr = curr->next;
    }
    
    fclose(fp);
    return 1;
}

int backup_patients(const char *filename) {
    FILE *src = fopen(filename, "r");
    if (!src) return 0; // Nothing to backup
    
    char backup_name[256];
    long long now = get_current_timestamp();
    snprintf(backup_name, sizeof(backup_name), "data/patients_backup_%lld.txt", now);
    
    FILE *dest = fopen(backup_name, "w");
    if (!dest) {
        fclose(src);
        return 0;
    }
    
    char ch;
    while ((ch = fgetc(src)) != EOF) {
        fputc(ch, dest);
    }
    
    fclose(src);
    fclose(dest);
    return 1;
}
