#ifndef FILEIO_H
#define FILEIO_H

#include "queue.h"

// Load all patients from the data file into the linked list
// Returns number of patients loaded, or -1 on error
int load_patients(PatientNode **head, const char *filename);

// Save all patients from the linked list to the data file
// Returns 1 on success, 0 on error
int save_patients(PatientNode *head, const char *filename);

// Create a backup of the patients file with a timestamp in its name
// Returns 1 on success, 0 on error
int backup_patients(const char *filename);

#endif // FILEIO_H
