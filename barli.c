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

typedef struct {
    char *data;
} Task;

static inline char *skip_spaces(char *s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static char *get_config_path(void) {
    static char path[256];
    const char *home = getenv("HOME");
    if (!home) home = ".";
    
    snprintf(path, sizeof(path), "%s/.config/barli.conf", home);
    if (access(path, F_OK) == 0) return path;
    
    snprintf(path, sizeof(path), "%s/.barli.conf", home);
    if (access(path, F_OK) == 0) return path;
    
    snprintf(path, sizeof(path), "%s/.config/barli.conf", home);
    char dir[256];
    snprintf(dir, sizeof(dir), "%s/.config", home);
    mkdir(dir, 0755);
    
    FILE *f = fopen(path, "w");
    if (f) {
        fputs("TIME: :: date :: \n", f);
        fclose(f);
    }
    return path;
}

static int load_tasks(Task *tasks) {
    int count = 0;
    FILE *f = fopen(get_config_path(), "r");
    if (!f) return 0;
    
    char line[MAX_LINE];
    while (fgets(line, sizeof(line), f) && count < MAX_TASKS) {
        line[strcspn(line, "\n")] = 0;
        char *s = skip_spaces(line);
        if (!*s || *s == '#') continue;
        
        if (strstr(s, "::") && strstr(s, "::")[2]) {
            tasks[count++].data = strdup(s);
        }
    }
    fclose(f);
    return count;
}

static void parse_task(const char *data, char **prefix, char **cmd, 
                       char **suffix, int *shell) {
    static char buf[MAX_LINE];
    strncpy(buf, data, MAX_LINE - 1);
    buf[MAX_LINE - 1] = 0;
    
    *prefix = buf;
    *cmd = *suffix = "";
    *shell = 0;
    
    char *p = strstr(buf, "::");
    if (!p) return;
    *p = 0;
    *cmd = skip_spaces(p + 2);
    
    p = strstr(*cmd, "::");
    if (p) {
        *p = 0;
        *suffix = p + 2;
        p = strstr(*suffix, "::");
        if (p) {
            *p = 0;
            char *sh = skip_spaces(p + 2);
            *shell = (*sh == 's' || *sh == 'S');
        }
    }
}

static int run_command(const char *cmd, int shell, char *out, int size) {
    FILE *fp = popen(shell ? cmd : cmd, "r");
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
    int ntasks = load_tasks(tasks);
    if (!ntasks) {
        XCloseDisplay(dpy);
        return 1;
    }
    
    char status[MAX_STATUS];
    char output[MAX_OUTPUT];
    char *sp;
    
    for (;;) {
        sp = status;
        *sp = 0;
        
        for (int i = 0; i < ntasks; i++) {
            char *pfx, *cmd, *sfx;
            int shell;
            parse_task(tasks[i].data, &pfx, &cmd, &sfx, &shell);
            
            if (run_command(cmd, shell, output, MAX_OUTPUT)) {
                if (sp != status) *sp++ = '|';
                *sp++ = ' ';
                
                if (*pfx) {
                    strcpy(sp, pfx);
                    sp += strlen(pfx);
                }
                
                strcpy(sp, output);
                sp += strlen(output);
                
                if (*sfx) {
                    strcpy(sp, sfx);
                    sp += strlen(sfx);
                }
                
                if (!*pfx && !*sfx) *sp++ = ' ';
            }
        }
        *sp = 0;
        
        XStoreName(dpy, root, status);
        XFlush(dpy);
        sleep(1);
    }
}
