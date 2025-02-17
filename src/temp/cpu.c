#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void get_memory_info() {
    FILE *file = fopen("/proc/meminfo", "r");
    if (file == NULL) {
        perror("Error opening meminfo file");
        return;
    }

    char line[256];
    while (fgets(line, sizeof(line), file)) {
        printf("---%s", line);
        if (strncmp(line, "MemTotal", 8) == 0) {
            printf("Total Memory: %s", line);
        } else if (strncmp(line, "MemFree", 7) == 0) {
            printf("Free Memory: %s", line);
        }
    }

    fclose(file);
}

int main() {
    get_memory_info();
    return 0;
}
