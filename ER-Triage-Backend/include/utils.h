#ifndef UTILS_H
#define UTILS_H

#include <time.h>

// Trim leading and trailing whitespace
void trim_whitespace(char *str);

// Case-insensitive substring search
// Returns pointer to first occurrence, or NULL if not found
const char *strcasestr_custom(const char *haystack, const char *needle);

// Get current Unix timestamp
long long get_current_timestamp(void);

// Format Unix timestamp as readable string (YYYY-MM-DD HH:MM:SS)
void format_timestamp(long long timestamp, char *dest, int max_len);

// Parse formatted string back to Unix timestamp
long long parse_timestamp(const char *str);

// Safely copy string, ensuring null-termination
void safe_strcpy(char *dest, const char *src, int max_len);

#endif // UTILS_H
