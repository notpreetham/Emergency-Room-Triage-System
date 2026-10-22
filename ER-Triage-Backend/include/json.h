#ifndef JSON_H
#define JSON_H

// Extract a string value from a JSON string based on key
// Returns 1 if successful, 0 if key not found
int json_get_string(const char *json, const char *key, char *dest, int max_len);

// Extract an integer value from a JSON string based on key
// Returns 1 if successful, 0 if key not found
int json_get_int(const char *json, const char *key, int *dest);

// Extract a float value from a JSON string based on key
// Returns 1 if successful, 0 if key not found
int json_get_float(const char *json, const char *key, float *dest);

// Extract a boolean value (true/false or 1/0) from a JSON string based on key
// Returns 1 if successful, 0 if key not found
int json_get_bool(const char *json, const char *key, int *dest);

// Utility to escape string characters (e.g. quotes, newlines) for JSON compliance
void json_escape_string(char *dest, const char *src, int max_len);

#endif // JSON_H
