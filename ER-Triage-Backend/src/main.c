#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "triage.h"
#include "queue.h"
#include "reports.h"
#include "fileio.h"
#include "json.h"
#include "utils.h"

// Constants for DB path
#define PATIENTS_DB_PATH "data/patients.txt"

// ANSI Color Escape Sequences for Terminal Mode
#define CLR_RESET   "\x1b[0m"
#define CLR_RED     "\x1b[31m"
#define CLR_GREEN   "\x1b[32m"
#define CLR_YELLOW  "\x1b[33m"
#define CLR_ORANGE  "\x1b[38;5;208m"
#define CLR_BLUE    "\x1b[34m"
#define CLR_CYAN    "\x1b[36m"
#define CLR_BOLD    "\x1b[1m"

// Function Declarations
static void run_interactive_menu(void);
static void print_patient_json(const Patient *p);
static int parse_patient_json(const char *json, Patient *p);
static char *read_file_content(const char *filepath);
static void generate_unique_id(PatientNode *head, char *dest, int max_len);

// CLI Command handlers
static void handle_cli_add(const char *json_file);
static void handle_cli_update(const char *json_file);
static void handle_cli_queue(void);
static void handle_cli_report(void);
static void handle_cli_search(const char *query);
static void handle_cli_treat(void);
static void handle_cli_backup(void);

int main(int argc, char *argv[]) {
    // If no arguments provided, enter interactive terminal mode
    if (argc < 2) {
        run_interactive_menu();
        return 0;
    }

    const char *cmd = argv[1];

    if (strcmp(cmd, "add") == 0) {
        if (argc < 3) {
            printf("{\n  \"status\": \"error\",\n  \"message\": \"Missing JSON file path\"\n}\n");
            return 1;
        }
        handle_cli_add(argv[2]);
    } else if (strcmp(cmd, "update") == 0) {
        if (argc < 3) {
            printf("{\n  \"status\": \"error\",\n  \"message\": \"Missing JSON file path\"\n}\n");
            return 1;
        }
        handle_cli_update(argv[2]);
    } else if (strcmp(cmd, "queue") == 0) {
        handle_cli_queue();
    } else if (strcmp(cmd, "report") == 0) {
        handle_cli_report();
    } else if (strcmp(cmd, "search") == 0) {
        if (argc < 3) {
            printf("{\n  \"status\": \"error\",\n  \"message\": \"Missing search query\"\n}\n");
            return 1;
        }
        handle_cli_search(argv[2]);
    } else if (strcmp(cmd, "treat") == 0) {
        handle_cli_treat();
    } else if (strcmp(cmd, "backup") == 0) {
        handle_cli_backup();
    } else {
        printf("{\n  \"status\": \"error\",\n  \"message\": \"Unknown command '%s'\"\n}\n", cmd);
        return 1;
    }

    return 0;
}

// ==========================================
// CLI COMMAND MODE IMPLEMENTATION
// ==========================================

static char *read_file_content(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *buf = (char *)malloc(size + 1);
    if (!buf) {
        fclose(fp);
        return NULL;
    }

    size_t read_bytes = fread(buf, 1, size, fp);
    buf[read_bytes] = '\0';

    fclose(fp);
    return buf;
}

static int parse_patient_json(const char *json, Patient *p) {
    memset(p, 0, sizeof(Patient));

    // Extract ID if exists (might be auto-generated later if empty)
    json_get_string(json, "id", p->id, sizeof(p->id));
    json_get_string(json, "name", p->name, sizeof(p->name));
    json_get_int(json, "age", &p->age);
    json_get_string(json, "gender", p->gender, sizeof(p->gender));
    json_get_string(json, "symptoms", p->symptoms, sizeof(p->symptoms));
    
    // Set vitals defaults to handle missing fields
    p->heart_rate = -1;
    p->oxygen_level = -1;
    p->temperature = -1.0f;
    
    json_get_int(json, "heart_rate", &p->heart_rate);
    json_get_int(json, "oxygen_level", &p->oxygen_level);
    json_get_float(json, "temperature", &p->temperature);
    json_get_string(json, "blood_pressure", p->blood_pressure, sizeof(p->blood_pressure));
    
    char arr_time[64] = {0};
    if (json_get_string(json, "arrival_time", arr_time, sizeof(arr_time))) {
        p->arrival_time = parse_timestamp(arr_time);
    } else {
        p->arrival_time = 0; // Not provided
    }

    p->ambulance_arrival = -1;
    json_get_int(json, "ambulance_arrival", &p->ambulance_arrival);
    json_get_string(json, "status", p->status, sizeof(p->status));

    return 1;
}

static void print_patient_json(const Patient *p) {
    char name_esc[128];
    char sym_esc[300];
    json_escape_string(name_esc, p->name, sizeof(name_esc));
    json_escape_string(sym_esc, p->symptoms, sizeof(sym_esc));

    printf("  {\n");
    printf("    \"id\": \"%s\",\n", p->id);
    printf("    \"name\": \"%s\",\n", name_esc);
    printf("    \"age\": %d,\n", p->age);
    printf("    \"gender\": \"%s\",\n", p->gender);
    printf("    \"symptoms\": \"%s\",\n", sym_esc);
    printf("    \"heart_rate\": %d,\n", p->heart_rate);
    printf("    \"oxygen_level\": %d,\n", p->oxygen_level);
    printf("    \"temperature\": %.2f,\n", p->temperature);
    printf("    \"blood_pressure\": \"%s\",\n", p->blood_pressure);
    printf("    \"arrival_time\": \"%s\",\n", p->arrival_time_str);
    printf("    \"ambulance_arrival\": %d,\n", p->ambulance_arrival);
    printf("    \"severity_score\": %d,\n", p->severity_score);
    printf("    \"triage_level\": \"%s\",\n", p->triage_level);
    printf("    \"estimated_wait_time\": %d,\n", p->estimated_wait_time);
    printf("    \"status\": \"%s\"\n", p->status);
    printf("  }");
}

static void generate_unique_id(PatientNode *head, char *dest, int max_len) {
    int max_num = 100; // IDs start at P101
    PatientNode *curr = head;
    while (curr) {
        int num = 0;
        const char *p = curr->patient.id;
        while (*p && !isdigit((unsigned char)*p)) p++;
        if (*p) {
            num = atoi(p);
            if (num > max_num) {
                max_num = num;
            }
        }
        curr = curr->next;
    }
    snprintf(dest, max_len, "P%d", max_num + 1);
}

static void handle_cli_add(const char *json_file) {
    char *json_content = read_file_content(json_file);
    if (!json_content) {
        printf("{\n  \"status\": \"error\",\n  \"message\": \"Could not read input JSON file\"\n}\n");
        return;
    }

    PatientNode *head = NULL;
    load_patients(&head, PATIENTS_DB_PATH);

    Patient p = {0};
    parse_patient_json(json_content, &p);
    free(json_content);

    // Auto-generate ID if empty or null
    if (p.id[0] == '\0') {
        generate_unique_id(head, p.id, sizeof(p.id));
    } else {
        // Validate uniqueness
        if (search_patient(head, p.id) != NULL) {
            printf("{\n  \"status\": \"error\",\n  \"message\": \"Patient ID '%s' already exists\"\n}\n", p.id);
            free_queue(head);
            return;
        }
    }

    // Default status, arrival time, gender, symptoms
    if (p.status[0] == '\0') {
        safe_strcpy(p.status, "waiting", sizeof(p.status));
    }
    if (p.arrival_time == 0) {
        p.arrival_time = get_current_timestamp();
    }
    format_timestamp(p.arrival_time, p.arrival_time_str, sizeof(p.arrival_time_str));
    
    if (p.gender[0] == '\0') safe_strcpy(p.gender, "Unknown", sizeof(p.gender));
    if (p.blood_pressure[0] == '\0') safe_strcpy(p.blood_pressure, "N/A", sizeof(p.blood_pressure));
    if (p.ambulance_arrival < 0) p.ambulance_arrival = 0;
    if (p.heart_rate < 0) p.heart_rate = 75;
    if (p.oxygen_level < 0) p.oxygen_level = 98;
    if (p.temperature < 0.0f) p.temperature = 98.6f;

    // Triage classification
    p.severity_score = calculate_severity(&p);
    assign_triage_level(p.severity_score, p.triage_level, sizeof(p.triage_level));

    if (add_patient(&head, p)) {
        save_patients(head, PATIENTS_DB_PATH);
        
        // Find saved record to retrieve wait times
        PatientNode *saved = search_patient(head, p.id);
        printf("{\n  \"status\": \"success\",\n");
        printf("  \"patient\":\n");
        print_patient_json(&saved->patient);
        printf("\n}\n");
    } else {
        printf("{\n  \"status\": \"error\",\n  \"message\": \"Failed to register patient in database\"\n}\n");
    }

    free_queue(head);
}

static void handle_cli_update(const char *json_file) {
    char *json_content = read_file_content(json_file);
    if (!json_content) {
        printf("{\n  \"status\": \"error\",\n  \"message\": \"Could not read input JSON file\"\n}\n");
        return;
    }

    PatientNode *head = NULL;
    load_patients(&head, PATIENTS_DB_PATH);

    Patient update_data = {0};
    parse_patient_json(json_content, &update_data);
    free(json_content);

    if (update_data.id[0] == '\0') {
        printf("{\n  \"status\": \"error\",\n  \"message\": \"Missing required field 'id' for update\"\n}\n");
        free_queue(head);
        return;
    }

    PatientNode *existing = search_patient(head, update_data.id);
    if (!existing) {
        printf("{\n  \"status\": \"error\",\n  \"message\": \"Patient ID '%s' not found\"\n}\n", update_data.id);
        free_queue(head);
        return;
    }

    if (update_patient(&head, update_data.id, &update_data)) {
        save_patients(head, PATIENTS_DB_PATH);
        
        // Retrieve updated patient record
        PatientNode *updated = search_patient(head, update_data.id);
        printf("{\n  \"status\": \"success\",\n");
        printf("  \"patient\":\n");
        print_patient_json(&updated->patient);
        printf("\n}\n");
    } else {
        printf("{\n  \"status\": \"error\",\n  \"message\": \"Failed to update patient\"\n}\n");
    }

    free_queue(head);
}

static void handle_cli_queue(void) {
    PatientNode *head = NULL;
    load_patients(&head, PATIENTS_DB_PATH);

    printf("{\n  \"status\": \"success\",\n");
    printf("  \"queue\": [\n");

    int count = 0;
    PatientNode *curr = head;
    while (curr) {
        if (strcmp(curr->patient.status, "waiting") == 0) {
            if (count > 0) printf(",\n");
            print_patient_json(&curr->patient);
            count++;
        }
        curr = curr->next;
    }

    printf("\n  ]\n}\n");
    free_queue(head);
}

static void handle_cli_report(void) {
    PatientNode *head = NULL;
    load_patients(&head, PATIENTS_DB_PATH);

    TriageReport report;
    generate_report(head, &report);
    print_report_json(&report);

    free_queue(head);
}

static void handle_cli_search(const char *query) {
    PatientNode *head = NULL;
    load_patients(&head, PATIENTS_DB_PATH);

    PatientNode *results[20];
    int count = search_patients_substring(head, query, results, 20);

    printf("{\n  \"status\": \"success\",\n");
    printf("  \"results\": [\n");

    for (int i = 0; i < count; i++) {
        print_patient_json(&results[i]->patient);
        if (i < count - 1) printf(",\n");
    }

    printf("\n  ]\n}\n");
    free_queue(head);
}

static void handle_cli_treat(void) {
    PatientNode *head = NULL;
    load_patients(&head, PATIENTS_DB_PATH);

    PatientNode *popped = pop_next_patient(&head);
    if (!popped) {
        printf("{\n  \"status\": \"error\",\n  \"message\": \"No patients waiting in queue\"\n}\n");
        free_queue(head);
        return;
    }

    // Set status to treated
    safe_strcpy(popped->patient.status, "treated", sizeof(popped->patient.status));
    popped->patient.estimated_wait_time = 0;

    // Append back to head. add_patient will place treated patients at the tail
    add_patient(&head, popped->patient);
    save_patients(head, PATIENTS_DB_PATH);

    printf("{\n  \"status\": \"success\",\n");
    printf("  \"message\": \"Patient successfully treated and moved out of priority queue\",\n");
    printf("  \"patient\":\n");
    print_patient_json(&popped->patient);
    printf("\n}\n");

    free(popped);
    free_queue(head);
}

static void handle_cli_backup(void) {
    if (backup_patients(PATIENTS_DB_PATH)) {
        printf("{\n  \"status\": \"success\",\n  \"message\": \"Database backup created successfully\"\n}\n");
    } else {
        printf("{\n  \"status\": \"error\",\n  \"message\": \"Failed to create database backup\"\n}\n");
    }
}

// ==========================================
// INTERACTIVE MODE TERMINAL LOOP
// ==========================================

static void clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

static void read_string_input(const char *prompt, char *dest, int max_len) {
    printf("%s", prompt);
    if (fgets(dest, max_len, stdin)) {
        trim_whitespace(dest);
    }
}

static void run_interactive_menu(void) {
    PatientNode *head = NULL;
    load_patients(&head, PATIENTS_DB_PATH);

    while (1) {
        printf("\n" CLR_BOLD CLR_BLUE "==================================================\n");
        printf("       ER TRIAGE CLINICAL CONTROL SYSTEM          \n");
        printf("==================================================\n" CLR_RESET);
        
        // Brief status card
        TriageReport rep;
        generate_report(head, &rep);
        printf(" Queue Size: " CLR_BOLD CLR_YELLOW "%d" CLR_RESET " | Treated: " CLR_BOLD CLR_GREEN "%d" CLR_RESET " | Avg Severity: " CLR_BOLD CLR_CYAN "%.1f" CLR_RESET "\n", 
               rep.queue_length, rep.total_treated, rep.average_severity);
        printf(" Critical Patients: " CLR_BOLD CLR_RED "%d" CLR_RESET " | Serious: " CLR_BOLD CLR_ORANGE "%d" CLR_RESET "\n",
               rep.critical_count, rep.serious_count);
        printf("--------------------------------------------------\n");
        
        printf("1. " CLR_BOLD "Register Patient" CLR_RESET "\n");
        printf("2. " CLR_BOLD "View Priority Queue" CLR_RESET "\n");
        printf("3. " CLR_BOLD "Treat Next Patient" CLR_RESET "\n");
        printf("4. " CLR_BOLD "Update Patient Vitals/Symptoms" CLR_RESET "\n");
        printf("5. " CLR_BOLD "Search Patient Records" CLR_RESET "\n");
        printf("6. " CLR_BOLD "View Comprehensive Analytics Report" CLR_RESET "\n");
        printf("7. " CLR_BOLD "Trigger Database Backup" CLR_RESET "\n");
        printf("8. " CLR_BOLD "Exit" CLR_RESET "\n");
        printf("Enter choice (1-8): ");

        int choice = 0;
        if (scanf("%d", &choice) != 1) {
            clear_input_buffer();
            printf(CLR_RED "Invalid selection. Please enter a number between 1 and 8.\n" CLR_RESET);
            continue;
        }
        clear_input_buffer();

        if (choice == 8) {
            printf("\nExiting. Thank you for using ER Triage System.\n");
            break;
        }

        switch (choice) {
            case 1: { // Register Patient
                Patient p = {0};

                printf("\n--- REGISTER PATIENT ---\n");
                
                // Patient ID (Optional)
                read_string_input("Patient ID (Enter to auto-generate): ", p.id, sizeof(p.id));
                if (p.id[0] == '\0') {
                    generate_unique_id(head, p.id, sizeof(p.id));
                    printf("Auto-generated ID: %s\n", p.id);
                } else {
                    if (search_patient(head, p.id) != NULL) {
                        printf(CLR_RED "Error: Patient ID '%s' already exists.\n" CLR_RESET, p.id);
                        break;
                    }
                }

                // Name
                do {
                    read_string_input("Patient Name (Required): ", p.name, sizeof(p.name));
                    if (p.name[0] == '\0') {
                        printf(CLR_RED "Name cannot be empty!\n" CLR_RESET);
                    }
                } while (p.name[0] == '\0');

                // Age
                printf("Age: ");
                while (scanf("%d", &p.age) != 1 || p.age < 0 || p.age > 150) {
                    clear_input_buffer();
                    printf(CLR_RED "Invalid age. Enter age: " CLR_RESET);
                }
                clear_input_buffer();

                // Gender
                read_string_input("Gender (Male/Female/Other): ", p.gender, sizeof(p.gender));
                if (p.gender[0] == '\0') safe_strcpy(p.gender, "Unknown", sizeof(p.gender));

                // Symptoms
                read_string_input("Symptoms description (e.g. Chest pain, Fever): ", p.symptoms, sizeof(p.symptoms));

                // Heart rate
                printf("Heart Rate (bpm): ");
                while (scanf("%d", &p.heart_rate) != 1 || p.heart_rate < 0) {
                    clear_input_buffer();
                    printf(CLR_RED "Invalid heart rate. Enter heart rate: " CLR_RESET);
                }
                clear_input_buffer();

                // Oxygen level
                printf("Oxygen Level (%%): ");
                while (scanf("%d", &p.oxygen_level) != 1 || p.oxygen_level < 0 || p.oxygen_level > 100) {
                    clear_input_buffer();
                    printf(CLR_RED "Invalid oxygen level (0-100). Enter oxygen: " CLR_RESET);
                }
                clear_input_buffer();

                // Temperature
                printf("Temperature (°F): ");
                while (scanf("%f", &p.temperature) != 1 || p.temperature < 50.0f || p.temperature > 115.0f) {
                    clear_input_buffer();
                    printf(CLR_RED "Invalid temperature. Enter temperature: " CLR_RESET);
                }
                clear_input_buffer();

                // BP
                read_string_input("Blood Pressure (mmHg, e.g. 120/80): ", p.blood_pressure, sizeof(p.blood_pressure));
                if (p.blood_pressure[0] == '\0') safe_strcpy(p.blood_pressure, "N/A", sizeof(p.blood_pressure));

                // Ambulance Arrival
                printf("Ambulance Arrival? (1 = Yes, 0 = No): ");
                while (scanf("%d", &p.ambulance_arrival) != 1 || (p.ambulance_arrival != 0 && p.ambulance_arrival != 1)) {
                    clear_input_buffer();
                    printf(CLR_RED "Invalid entry. Enter 1 (Yes) or 0 (No): " CLR_RESET);
                }
                clear_input_buffer();

                // System initializers
                p.arrival_time = get_current_timestamp();
                format_timestamp(p.arrival_time, p.arrival_time_str, sizeof(p.arrival_time_str));
                safe_strcpy(p.status, "waiting", sizeof(p.status));

                // Calculations
                p.severity_score = calculate_severity(&p);
                assign_triage_level(p.severity_score, p.triage_level, sizeof(p.triage_level));

                if (add_patient(&head, p)) {
                    save_patients(head, PATIENTS_DB_PATH);
                    
                    // Retrieve wait time
                    PatientNode *saved = search_patient(head, p.id);
                    printf("\n" CLR_GREEN "✔ Patient Registered Successfully!" CLR_RESET "\n");
                    printf("  - Severity Score : %d\n", saved->patient.severity_score);
                    printf("  - Triage Level   : ");
                    if (strcmp(saved->patient.triage_level, "CRITICAL") == 0) printf(CLR_RED CLR_BOLD "%s\n" CLR_RESET, saved->patient.triage_level);
                    else if (strcmp(saved->patient.triage_level, "SERIOUS") == 0) printf(CLR_ORANGE CLR_BOLD "%s\n" CLR_RESET, saved->patient.triage_level);
                    else if (strcmp(saved->patient.triage_level, "MODERATE") == 0) printf(CLR_YELLOW CLR_BOLD "%s\n" CLR_RESET, saved->patient.triage_level);
                    else printf(CLR_GREEN CLR_BOLD "%s\n" CLR_RESET, saved->patient.triage_level);
                    
                    printf("  - Est. Wait Time : %d minutes\n", saved->patient.estimated_wait_time);
                } else {
                    printf(CLR_RED "Error adding patient to queue.\n" CLR_RESET);
                }
                break;
            }

            case 2: { // View Priority Queue
                printf("\n" CLR_BOLD "------------------- ACTIVE TRIAGE QUEUE -------------------\n" CLR_RESET);
                printf("%-6s | %-16s | %-4s | %-13s | %-8s | %-9s\n", "ID", "Name", "Age", "Triage", "Severity", "Wait Time");
                printf("-----------------------------------------------------------\n");
                
                int count = 0;
                PatientNode *curr = head;
                while (curr) {
                    if (strcmp(curr->patient.status, "waiting") == 0) {
                        const char *color = CLR_RESET;
                        if (strcmp(curr->patient.triage_level, "CRITICAL") == 0) color = CLR_RED;
                        else if (strcmp(curr->patient.triage_level, "SERIOUS") == 0) color = CLR_ORANGE;
                        else if (strcmp(curr->patient.triage_level, "MODERATE") == 0) color = CLR_YELLOW;
                        else color = CLR_GREEN;

                        printf("%-6s | %-16.16s | %-4d | %s%-13.13s" CLR_RESET " | %-8d | %d mins\n",
                               curr->patient.id,
                               curr->patient.name,
                               curr->patient.age,
                               color,
                               curr->patient.triage_level,
                               curr->patient.severity_score,
                               curr->patient.estimated_wait_time);
                        count++;
                    }
                    curr = curr->next;
                }
                if (count == 0) {
                    printf("  No waiting patients in the queue.\n");
                }
                printf("-----------------------------------------------------------\n");
                break;
            }

            case 3: { // Treat Next Patient
                PatientNode *next_p = pop_next_patient(&head);
                if (!next_p) {
                    printf("\n" CLR_YELLOW "No active patients in the queue." CLR_RESET "\n");
                    break;
                }
                
                printf("\n" CLR_BOLD CLR_GREEN "Treating Next Priority Patient:" CLR_RESET "\n");
                printf("  - ID: %s | Name: %s | Age: %d\n", next_p->patient.id, next_p->patient.name, next_p->patient.age);
                printf("  - Symptoms: %s\n", next_p->patient.symptoms);
                printf("  - Vitals: HR %d bpm | SpO2 %d%% | Temp %.1f°F | BP %s\n", 
                       next_p->patient.heart_rate, next_p->patient.oxygen_level, next_p->patient.temperature, next_p->patient.blood_pressure);
                printf("  - Severity: %d | Level: %s\n", next_p->patient.severity_score, next_p->patient.triage_level);
                
                // Mark treated and re-insert at bottom
                safe_strcpy(next_p->patient.status, "treated", sizeof(next_p->patient.status));
                next_p->patient.estimated_wait_time = 0;
                add_patient(&head, next_p->patient);
                save_patients(head, PATIENTS_DB_PATH);
                
                printf(CLR_GREEN "\n✔ Patient status updated to 'treated' and recorded.\n" CLR_RESET);
                free(next_p);
                break;
            }

            case 4: { // Update Patient Vitals/Symptoms
                char id_to_find[32];
                read_string_input("\nEnter Patient ID to update: ", id_to_find, sizeof(id_to_find));
                
                PatientNode *target = search_patient(head, id_to_find);
                if (!target) {
                    printf(CLR_RED "Patient ID '%s' not found.\n" CLR_RESET, id_to_find);
                    break;
                }

                if (strcmp(target->patient.status, "treated") == 0) {
                    printf(CLR_YELLOW "Note: Patient has already been treated.\n" CLR_RESET);
                }

                Patient update_v = {0};
                char val_str[128];
                
                printf("Updating patient %s (%s). Press Enter to keep current value.\n", target->patient.id, target->patient.name);
                
                // New Symptoms
                printf("Current symptoms: %s\n", target->patient.symptoms);
                read_string_input("Enter new symptoms: ", update_v.symptoms, sizeof(update_v.symptoms));
                
                // New Heart Rate
                printf("Current HR: %d bpm\n", target->patient.heart_rate);
                printf("Enter new HR: ");
                if (fgets(val_str, sizeof(val_str), stdin) && val_str[0] != '\n') {
                    update_v.heart_rate = atoi(val_str);
                } else {
                    update_v.heart_rate = target->patient.heart_rate;
                }

                // New Oxygen Level
                printf("Current SpO2: %d%%\n", target->patient.oxygen_level);
                printf("Enter new SpO2: ");
                if (fgets(val_str, sizeof(val_str), stdin) && val_str[0] != '\n') {
                    update_v.oxygen_level = atoi(val_str);
                } else {
                    update_v.oxygen_level = target->patient.oxygen_level;
                }

                // New Temperature
                printf("Current Temp: %.1f°F\n", target->patient.temperature);
                printf("Enter new Temp: ");
                if (fgets(val_str, sizeof(val_str), stdin) && val_str[0] != '\n') {
                    update_v.temperature = atof(val_str);
                } else {
                    update_v.temperature = target->patient.temperature;
                }

                // New BP
                printf("Current BP: %s\n", target->patient.blood_pressure);
                read_string_input("Enter new BP: ", update_v.blood_pressure, sizeof(update_v.blood_pressure));
                if (update_v.blood_pressure[0] == '\0') {
                    safe_strcpy(update_v.blood_pressure, target->patient.blood_pressure, sizeof(update_v.blood_pressure));
                }

                // Preserve status and meta
                safe_strcpy(update_v.id, target->patient.id, sizeof(update_v.id));
                safe_strcpy(update_v.name, target->patient.name, sizeof(update_v.name));
                update_v.age = target->patient.age;
                safe_strcpy(update_v.gender, target->patient.gender, sizeof(update_v.gender));
                update_v.arrival_time = target->patient.arrival_time;
                update_v.ambulance_arrival = target->patient.ambulance_arrival;
                safe_strcpy(update_v.status, target->patient.status, sizeof(update_v.status));

                if (update_patient(&head, target->patient.id, &update_v)) {
                    save_patients(head, PATIENTS_DB_PATH);
                    printf(CLR_GREEN "\n✔ Patient vitals updated and priority re-evaluated.\n" CLR_RESET);
                    
                    // Retrieve wait time & severity
                    PatientNode *updated_p = search_patient(head, id_to_find);
                    printf("  - New Severity Score: %d | Level: %s\n", updated_p->patient.severity_score, updated_p->patient.triage_level);
                    printf("  - New Est. Wait Time: %d mins\n", updated_p->patient.estimated_wait_time);
                } else {
                    printf(CLR_RED "Failed to update patient.\n" CLR_RESET);
                }
                break;
            }

            case 5: { // Search Patient
                char query[128];
                read_string_input("\nEnter ID or Name substring to search: ", query, sizeof(query));
                
                PatientNode *results[20];
                int count = search_patients_substring(head, query, results, 20);
                
                printf("\nSearch Results (Found %d matches):\n", count);
                printf("------------------------------------------------------------------------------------\n");
                printf("%-6s | %-16s | %-4s | %-12s | %-8s | %-8s | %-8s\n", "ID", "Name", "Age", "Triage", "Score", "WaitTime", "Status");
                printf("------------------------------------------------------------------------------------\n");
                for (int i = 0; i < count; i++) {
                    const char *color = CLR_RESET;
                    if (strcmp(results[i]->patient.triage_level, "CRITICAL") == 0) color = CLR_RED;
                    else if (strcmp(results[i]->patient.triage_level, "SERIOUS") == 0) color = CLR_ORANGE;
                    else if (strcmp(results[i]->patient.triage_level, "MODERATE") == 0) color = CLR_YELLOW;
                    else color = CLR_GREEN;

                    printf("%-6s | %-16.16s | %-4d | %s%-12.12s" CLR_RESET " | %-8d | %-8d | %-8s\n",
                           results[i]->patient.id,
                           results[i]->patient.name,
                           results[i]->patient.age,
                           color,
                           results[i]->patient.triage_level,
                           results[i]->patient.severity_score,
                           results[i]->patient.estimated_wait_time,
                           results[i]->patient.status);
                    printf("  Symptoms: %s | Vitals: HR %d | SpO2 %d%% | Temp %.1f | BP %s | Arrived: %s\n\n",
                           results[i]->patient.symptoms,
                           results[i]->patient.heart_rate,
                           results[i]->patient.oxygen_level,
                           results[i]->patient.temperature,
                           results[i]->patient.blood_pressure,
                           results[i]->patient.arrival_time_str);
                }
                printf("------------------------------------------------------------------------------------\n");
                break;
            }

            case 6: { // View Comprehensive Analytics Report
                TriageReport r;
                generate_report(head, &r);
                print_report_human(&r);
                break;
            }

            case 7: { // Trigger Database Backup
                if (backup_patients(PATIENTS_DB_PATH)) {
                    printf("\n" CLR_GREEN "✔ Patient database successfully backed up in 'data/' folder!" CLR_RESET "\n");
                } else {
                    printf("\n" CLR_RED "Failed to create database backup." CLR_RESET "\n");
                }
                break;
            }
        }
    }

    free_queue(head);
}
