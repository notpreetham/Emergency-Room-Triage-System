#ifndef TRIAGE_H
#define TRIAGE_H

typedef struct {
    char id[32];
    char name[100];
    int age;
    char gender[16];
    char symptoms[256];
    int heart_rate;
    int oxygen_level;
    float temperature;
    char blood_pressure[16];
    long long arrival_time;
    char arrival_time_str[32];
    int ambulance_arrival;
    int severity_score;
    char triage_level[16];
    int estimated_wait_time;
    char status[16]; // "waiting" or "treated"
} Patient;

// Calculate severity score based on vitals and symptoms
int calculate_severity(const Patient *p);

// Assign triage level based on severity score
void assign_triage_level(int severity_score, char *dest, int max_len);

#endif // TRIAGE_H
