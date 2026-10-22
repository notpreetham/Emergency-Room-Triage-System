#include "json.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Helper to find a key's value position in JSON
static const char *find_value_position(const char *json, const char *key) {
    if (!json || !key) return NULL;
    
    char key_pattern[128];
    snprintf(key_pattern, sizeof(key_pattern), "\"%s\"", key);
    
    const char *p = json;
    while ((p = strstr(p, key_pattern)) != NULL) {
        // Verify this is a key (followed by colon, possibly with whitespace)
        const char *after_key = p + strlen(key_pattern);
        while (*after_key && isspace((unsigned char)*after_key)) {
            after_key++;
        }
        if (*after_key == ':') {
            return after_key + 1; // Return position right after colon
        }
        p += 1; // Move forward and search again
    }
    return NULL;
}

int json_get_string(const char *json, const char *key, char *dest, int max_len) {
    if (!dest || max_len <= 0) return 0;
    dest[0] = '\0';
    
    const char *val_pos = find_value_position(json, key);
    if (!val_pos) return 0;
    
    // Skip whitespace
    while (*val_pos && isspace((unsigned char)*val_pos)) {
        val_pos++;
    }
    
    if (*val_pos != '"') {
        // If not in quotes (e.g. number/boolean), extract as literal string
        int i = 0;
        while (*val_pos && *val_pos != ',' && *val_pos != '}' && *val_pos != ']' && !isspace((unsigned char)*val_pos) && i < max_len - 1) {
            dest[i++] = *val_pos++;
        }
        dest[i] = '\0';
        return (i > 0);
    }
    
    val_pos++; // Skip opening quote
    int i = 0;
    while (*val_pos && *val_pos != '"' && i < max_len - 1) {
        if (*val_pos == '\\' && *(val_pos + 1)) {
            val_pos++; // Skip backslash
            // Simple unescape
            if (*val_pos == 'n') dest[i++] = '\n';
            else if (*val_pos == 't') dest[i++] = '\t';
            else dest[i++] = *val_pos;
        } else {
            dest[i++] = *val_pos;
        }
        val_pos++;
    }
    dest[i] = '\0';
    return 1;
}

int json_get_int(const char *json, const char *key, int *dest) {
    if (!dest) return 0;
    const char *val_pos = find_value_position(json, key);
    if (!val_pos) return 0;
    
    while (*val_pos && isspace((unsigned char)*val_pos)) {
        val_pos++;
    }
    
    char *endptr;
    long val = strtol(val_pos, &endptr, 10);
    if (endptr == val_pos) {
        // If parsing as integer failed, check if it's a JSON boolean value
        if (strncmp(val_pos, "true", 4) == 0) {
            *dest = 1;
            return 1;
        } else if (strncmp(val_pos, "false", 5) == 0) {
            *dest = 0;
            return 1;
        }
        return 0;
    }
    *dest = (int)val;
    return 1;
}

int json_get_float(const char *json, const char *key, float *dest) {
    if (!dest) return 0;
    const char *val_pos = find_value_position(json, key);
    if (!val_pos) return 0;
    
    while (*val_pos && isspace((unsigned char)*val_pos)) {
        val_pos++;
    }
    
    char *endptr;
    float val = strtof(val_pos, &endptr);
    if (endptr == val_pos) {
        return 0;
    }
    *dest = val;
    return 1;
}

int json_get_bool(const char *json, const char *key, int *dest) {
    if (!dest) return 0;
    const char *val_pos = find_value_position(json, key);
    if (!val_pos) return 0;
    
    while (*val_pos && isspace((unsigned char)*val_pos)) {
        val_pos++;
    }
    
    if (strncmp(val_pos, "true", 4) == 0) {
        *dest = 1;
        return 1;
    } else if (strncmp(val_pos, "false", 5) == 0) {
        *dest = 0;
        return 1;
    } else {
        // Fall back to integer parsing
        int int_val = 0;
        if (json_get_int(json, key, &int_val)) {
            *dest = (int_val != 0);
            return 1;
        }
    }
    return 0;
}

void json_escape_string(char *dest, const char *src, int max_len) {
    if (!dest || max_len <= 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    int d = 0;
    for (int s = 0; src[s] && d < max_len - 2; s++) {
        if (src[s] == '"' || src[s] == '\\') {
            if (d < max_len - 3) {
                dest[d++] = '\\';
                dest[d++] = src[s];
            } else {
                break;
            }
        } else if (src[s] == '\n') {
            if (d < max_len - 3) {
                dest[d++] = '\\';
                dest[d++] = 'n';
            } else {
                break;
            }
        } else if (src[s] == '\t') {
            if (d < max_len - 3) {
                dest[d++] = '\\';
                dest[d++] = 't';
            } else {
                break;
            }
        } else if (src[s] == '\r') {
            // Skip carriage return
        } else {
            dest[d++] = src[s];
        }
    }
    dest[d] = '\0';
}
