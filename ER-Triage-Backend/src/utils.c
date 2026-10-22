#include "utils.h"
#include <ctype.h>
#include <string.h>
#include <stdio.h>

void trim_whitespace(char *str) {
    if (!str) return;
    int len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
    int start = 0;
    while (str[start] && isspace((unsigned char)str[start])) {
        start++;
    }
    if (start > 0) {
        memmove(str, str + start, len - start + 1);
    }
}

const char *strcasestr_custom(const char *haystack, const char *needle) {
    if (!haystack || !needle) return NULL;
    if (!*needle) return haystack;

    for (; *haystack; haystack++) {
        if (tolower((unsigned char)*haystack) == tolower((unsigned char)*needle)) {
            const char *h, *n;
            for (h = haystack, n = needle; *h && *n; h++, n++) {
                if (tolower((unsigned char)*h) != tolower((unsigned char)*n)) {
                    break;
                }
            }
            if (!*n) {
                return haystack;
            }
        }
    }
    return NULL;
}

long long get_current_timestamp(void) {
    return (long long)time(NULL);
}

void format_timestamp(long long timestamp, char *dest, int max_len) {
    time_t t = (time_t)timestamp;
    struct tm *tmp = localtime(&t);
    if (tmp == NULL) {
        snprintf(dest, max_len, "1970-01-01 00:00:00");
        return;
    }
    if (strftime(dest, max_len, "%Y-%m-%d %H:%M:%S", tmp) == 0) {
        snprintf(dest, max_len, "1970-01-01 00:00:00");
    }
}

long long parse_timestamp(const char *str) {
    if (!str || strlen(str) < 10) return get_current_timestamp();
    struct tm tm_val = {0};
    int y = 1970, m = 1, d = 1, hr = 0, min = 0, sec = 0;
    
    // Support both standard "YYYY-MM-DD HH:MM:SS" and ISO "YYYY-MM-DDTHH:MM:SS"
    int parsed = sscanf(str, "%d-%d-%d %d:%d:%d", &y, &m, &d, &hr, &min, &sec);
    if (parsed < 3) {
        parsed = sscanf(str, "%d-%d-%dT%d:%d:%d", &y, &m, &d, &hr, &min, &sec);
    }
    if (parsed < 3) {
        return get_current_timestamp();
    }
    
    tm_val.tm_year = y - 1900;
    tm_val.tm_mon = m - 1;
    tm_val.tm_mday = d;
    tm_val.tm_hour = hr;
    tm_val.tm_min = min;
    tm_val.tm_sec = sec;
    tm_val.tm_isdst = -1; // Let system determine daylight saving status
    
    time_t t = mktime(&tm_val);
    if (t == -1) return get_current_timestamp();
    return (long long)t;
}

void safe_strcpy(char *dest, const char *src, int max_len) {
    if (!dest || max_len <= 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, max_len - 1);
    dest[max_len - 1] = '\0';
}
