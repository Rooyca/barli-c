#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <X11/Xlib.h>

#define MAX_TASKS 12
#define MAX_LINE 200
#define MAX_OUTPUT 96
#define MAX_STATUS 768
#define DEFAULT_SLEEP_TIME 2

typedef struct {
    char *prefix;
    char *cmd;
    char *suffix;
    int shell;
} Task;

static inline char *skip_spaces(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static char *get_config_path(void) {
    static char path[256];
    const char *home = getenv("HOME");
    if (!home || !*home) {
        fprintf(stderr, "Error: HOME env not set\n");
        exit(1);
    }
    
    snprintf(path, sizeof(path), "%s/.config/barli.conf", home);
    if (access(path, F_OK) == 0) return path;
    
    FILE *f = fopen(path, "w");
    if (f) {
        fputs("TIME: :: date :: \n", f);
        fclose(f);
    }
    return path;
}

static void parse_and_store_task(const char *data, Task *task) {
    char *buf = strdup(data);
    if (!buf) return;
    
    task->prefix = buf;
    task->cmd = task->suffix = "";
    task->shell = 0;
    
    char *p = strstr(buf, "::");
    if (!p) return;
    *p = 0;
    task->cmd = skip_spaces(p + 2);
    
    p = strstr(task->cmd, "::");
    if (p) {
        *p = 0;
        task->suffix = skip_spaces(p + 2);
        p = strstr(task->suffix, "::");
        if (p) {
            *p = 0;
            char *sh = skip_spaces(p + 2);
            task->shell = (*sh == 's' || *sh == 'S');
        }
    }
}

static int load_tasks(Task *tasks, int *sleep_time) {
    int count = 0;
    *sleep_time = DEFAULT_SLEEP_TIME;
    
    FILE *f = fopen(get_config_path(), "r");
    if (!f) return 0;
    
    char line[MAX_LINE];
    int first_line = 1;
    
    while (fgets(line, sizeof(line), f) && count < MAX_TASKS) {
        line[strcspn(line, "\n")] = 0;
        char *s = skip_spaces(line);
        
        if (first_line && *s && *s != '#') {
            if (strncmp(s, "SLEEP_TIME:", 11) == 0) {
                int val = atoi(skip_spaces(s + 11));
                if (val > 0) *sleep_time = val;
                first_line = 0;
                continue;
            }
            first_line = 0;
        }
        
        if (!*s || *s == '#') continue;
        
        if (strstr(s, "::") && strstr(s, "::")[2]) {
            parse_and_store_task(s, &tasks[count]);
            count++;
        }
    }
    fclose(f);
    return count;
}

static int run_command(const char *cmd, int shell, char *out, int size) {
    char cmdline[MAX_LINE * 2];
    if (shell) {
        char escaped[MAX_LINE * 2];
        char *dst = escaped;
        const char *src = cmd;
        
        while (*src && (dst - escaped) < (int)sizeof(escaped) - 10) {
            if (*src == '\'') {
                *dst++ = '\'';
                *dst++ = '\\';
                *dst++ = '\'';
                *dst++ = '\'';
            } else {
                *dst++ = *src;
            }
            src++;
        }
        *dst = 0;
        
        snprintf(cmdline, sizeof(cmdline), "sh -c '%s'", escaped);
    } else {
        strncpy(cmdline, cmd, sizeof(cmdline) - 1);
        cmdline[sizeof(cmdline) - 1] = 0;
    }
    
    FILE *fp = popen(cmdline, "r");
    if (!fp) return 0;
    
    int has_output = 0;
    if (fgets(out, size, fp)) {
        out[strcspn(out, "\n")] = 0;
        has_output = (*out != 0);
    }
    pclose(fp);
    return has_output;
}

int main(void) {
    Display *dpy = XOpenDisplay(NULL);
    if (!dpy) return 1;
    
    Window root = DefaultRootWindow(dpy);
    
    Task tasks[MAX_TASKS];
    int sleep_time;
    int ntasks = load_tasks(tasks, &sleep_time);
    if (!ntasks) {
        XCloseDisplay(dpy);
        return 1;
    }
    
    char status[MAX_STATUS];
    char output[MAX_OUTPUT];
    
    for (;;) {
        char *sp = status;
        *sp = 0;
        
        for (int i = 0; i < ntasks; i++) {
            if (run_command(tasks[i].cmd, tasks[i].shell, output, MAX_OUTPUT)) {
                if (sp != status) {
                    int remaining = MAX_STATUS - (sp - status);
                    if (remaining > 3) {
                        *sp++ = ' ';
                        *sp++ = '|';
                        *sp++ = ' ';
                    }
                }
                
                if (*tasks[i].prefix) {
                    int len = strlen(tasks[i].prefix);
                    int remaining = MAX_STATUS - (sp - status);
                    if (len < remaining) {
                        strcpy(sp, tasks[i].prefix);
                        sp += len;
                    }
                }
                
                int len = strlen(output);
                int remaining = MAX_STATUS - (sp - status);
                if (len < remaining) {
                    strcpy(sp, output);
                    sp += len;
                }
                
                if (*tasks[i].suffix) {
                    int len = strlen(tasks[i].suffix);
                    int remaining = MAX_STATUS - (sp - status);
                    if (len < remaining) {
                        strcpy(sp, tasks[i].suffix);
                        sp += len;
                    }
                }
            }
        }
        *sp = 0;
        
        XStoreName(dpy, root, status);
        XFlush(dpy);
        sleep(sleep_time);
    }
}
