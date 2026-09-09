#include <process/shell.h>
#include <process/task.h>
#include <drivers/vga.h>
#include <drivers/serial.h>
#include <drivers/keyboard.h>
#include <drivers/timer.h>
#include <drivers/rtc.h>
#include <fs/chfs.h>
#include <string.h>

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static char current_args[256];

/* Reboot via keyboard controller pulse */
static void reboot(void) {
    vga_puts("Rebooting...\n");
    serial_puts("Rebooting...\n");
    // Wait for keyboard controller ready
    for(int i=0;i<100;i++) asm volatile("nop");
    outb(0x64, 0xFE); // pulse reset
    // Fallback: triple fault
    asm volatile("cli");
    // Load invalid IDT and trigger interrupt
    struct { uint16_t limit; uint32_t base; } __attribute__((packed)) bad = {0,0};
    asm volatile("lidt %0" : : "m"(bad));
    asm volatile("int $0");
    for(;;) asm volatile("hlt");
}

static void cmd_ls(void) {
    fs_list();
}

static void cmd_mkdir(void) {
    // args is the path
    vga_puts("mkdir: "); vga_puts(current_args); vga_puts("\n");
    int ret = fs_mkdir(current_args);
    if (ret >= 0) {
        vga_puts("  created (inode="); vga_putdec(ret); vga_puts(")\n");
        serial_puts("mkdir OK inode="); serial_puthex(ret); serial_puts("\n");
    } else {
        vga_puts("  failed\n");
        serial_puts("mkdir FAIL\n");
    }
}

static void cmd_rm(void) {
    vga_puts("rm: "); vga_puts(current_args); vga_puts("\n");
    int ret = fs_unlink(current_args);
    if (ret == 0) {
        vga_puts("  deleted\n");
        serial_puts("rm OK\n");
    } else {
        vga_puts("  failed\n");
        serial_puts("rm FAIL\n");
    }
}

static void cmd_cat(void) {
    int fd = fs_open(current_args, 1); // O_RDONLY = 1
    if (fd < 0) {
        vga_puts("cat: cannot open "); vga_puts(current_args); vga_puts("\n");
        return;
    }
    char buf[64];
    int n = fs_read(fd, buf, sizeof(buf)-1);
    if (n > 0) {
        buf[n] = '\0';
        vga_puts(buf); vga_puts("\n");
    }
    fs_close(fd);
}

#define SHELL_BUF 256
static char line_buf[SHELL_BUF];

static void cmd_pwd(void) {
    vga_puts("pwd: / (chFS root)\n");
    serial_puts("pwd: /\n");
}

static void cmd_touch(void) {
    vga_puts("touch: "); vga_puts(current_args); vga_puts("\n");
    int fd = fs_open(current_args, 5); // O_CREATE (4) ama basit
    if (fd >= 0) {
        fs_close(fd);
        vga_puts("  created\n");
        serial_puts("touch OK\n");
    } else {
        vga_puts("  failed\n");
        serial_puts("touch FAIL\n");
    }
}

static void cmd_help(void) {
    vga_puts("Available commands:\n");
    vga_puts("  help     - show this help\n");
    vga_puts("  clear    - clear screen\n");
    vga_puts("  echo <text> - print text\n");
    vga_puts("  ticks    - show timer ticks\n");
    vga_puts("  ps       - list tasks\n");
    vga_puts("  date     - show date/time (RTC)\n");
    vga_puts("  reboot   - reboot system\n");
    vga_puts("  ls       - list chFS files\n");
    vga_puts("  mkdir <d> - create directory\n");
    vga_puts("  cat <path> - show file content\n");
    vga_puts("  rm <path> - remove file\n");
    vga_puts("  pwd      - show current directory\n");
    vga_puts("  touch <f> - create empty file\n");
    serial_puts("Available commands:\n");
    serial_puts("  help, clear, echo, ticks, ps, date, reboot, ls, mkdir, cat\n");
}

static void cmd_date(void) {
    struct rtc_time t;
    rtc_get_time(&t);
    vga_puts("Date: ");
    if(t.day<10) { vga_putc('0'); } vga_putdec(t.day); vga_putc('/');
    if(t.month<10) { vga_putc('0'); } vga_putdec(t.month); vga_putc('/');
    vga_putdec(t.year);
    vga_puts(" Time: ");
    if(t.hour<10) vga_putc('0');
    vga_putdec(t.hour); vga_putc(':');
    if(t.minute<10) vga_putc('0');
    vga_putdec(t.minute); vga_putc(':');
    if(t.second<10) vga_putc('0');
    vga_putdec(t.second);
    vga_puts(" WD:"); vga_putdec(t.weekday); vga_puts("\n");
    serial_puts("Date: ");
    // Serial dec manual
    if(t.day<10) serial_putc('0');
    { uint32_t v=t.day; char rev[4]; int r=0; if(v==0) serial_putc('0'); else {while(v>0){rev[r++]='0'+(v%10); v/=10;} while(r>0) serial_putc(rev[--r]);}}
    serial_putc('/');
    if(t.month<10) serial_putc('0');
    { uint32_t v=t.month; char rev[4]; int r=0; if(v==0) serial_putc('0'); else {while(v>0){rev[r++]='0'+(v%10); v/=10;} while(r>0) serial_putc(rev[--r]);}}
    serial_putc('/');
    { uint32_t v=t.year; char rev[6]; int r=0; if(v==0) serial_putc('0'); else {while(v>0){rev[r++]='0'+(v%10); v/=10;} while(r>0) serial_putc(rev[--r]);}}
    serial_puts(" ");
    if(t.hour<10) serial_putc('0');
    { uint32_t v=t.hour; char rev[4]; int r=0; if(v==0) serial_putc('0'); else {while(v>0){rev[r++]='0'+(v%10); v/=10;} while(r>0) serial_putc(rev[--r]);}}
    serial_putc(':');
    if(t.minute<10) serial_putc('0');
    { uint32_t v=t.minute; char rev[4]; int r=0; if(v==0) serial_putc('0'); else {while(v>0){rev[r++]='0'+(v%10); v/=10;} while(r>0) serial_putc(rev[--r]);}}
    serial_putc(':');
    if(t.second<10) serial_putc('0');
    { uint32_t v=t.second; char rev[4]; int r=0; if(v==0) serial_putc('0'); else {while(v>0){rev[r++]='0'+(v%10); v/=10;} while(r>0) serial_putc(rev[--r]);}}
    serial_puts("\n");
}

static void cmd_clear(void) {
    vga_clear();
}

static void cmd_ticks(void) {
    uint32_t t = timer_get_ticks();
    vga_puts("Ticks: "); vga_putdec(t); vga_puts(" (");
    vga_putdec(t/100); vga_puts(" sec)\n");
    serial_puts("Ticks: ");
    uint32_t v=t;
    if(v==0) serial_putc('0');
    else {
        char rev[12]; int r=0;
        while(v>0){ rev[r++]='0'+(v%10); v/=10; }
        while(r>0) serial_putc(rev[--r]);
    }
    serial_puts(" ticks\n");
}

void shell_execute(const char* line) {
    // Trim leading spaces
    while(*line==' '||*line=='\t') line++;
    if(*line==0) return;

    // Extract command
    char cmd[32]; int i=0;
    while(line[i] && line[i]!=' ' && line[i]!='\t' && i<31){ cmd[i]=line[i]; i++; }
    cmd[i]=0;
    const char* args = line+i;
    while(*args==' '||*args=='\t') args++;
    strncpy(current_args, args, 255);
    current_args[255] = 0;

    if(strcmp(cmd,"help")==0) {
        cmd_help();
    } else if(strcmp(cmd,"clear")==0) {
        cmd_clear();
    } else if(strcmp(cmd,"reboot")==0) {
        reboot();
    } else if(strcmp(cmd,"ticks")==0) {
        cmd_ticks();
    } else if(strcmp(cmd,"ps")==0) {
        task_dump();
    } else if(strcmp(cmd,"date")==0 || strcmp(cmd,"time")==0) {
        cmd_date();
    } else if(strcmp(cmd,"ls")==0) {
        cmd_ls();
    } else if(strcmp(cmd,"mkdir")==0) {
        cmd_mkdir();
    } else if(strcmp(cmd,"cat")==0) {
        cmd_cat();
    } else if(strcmp(cmd,"rm")==0) {
        cmd_rm();
    } else if(strcmp(cmd,"pwd")==0) {
        cmd_pwd();
    } else if(strcmp(cmd,"touch")==0) {
        cmd_touch();
    } else if(strcmp(cmd,"echo")==0) {
        vga_puts(args); vga_puts("\n");
        serial_puts(args); serial_puts("\n");
    } else {
        vga_puts("Unknown command: "); vga_puts(cmd); vga_puts("\n Type 'help' for list\n");
        serial_puts("Unknown command: "); serial_puts(cmd); serial_puts("\n");
    }
}

#define SHELL_BUF 256
static char line_buf[SHELL_BUF];

void shell_init(void) {
    // Nothing yet
}

void shell_run(void) {
    vga_puts("\ncharOS shell ready. Type 'help' for commands.\n");
    serial_puts("\ncharOS shell ready. Type 'help' for commands.\n");
    for(;;) {
        vga_set_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK);
        vga_puts("charos> ");
        vga_set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
        serial_puts("charos> ");

        int pos=0;
        memset(line_buf,0,SHELL_BUF);
        // Read line
        while(1) {
            char c = keyboard_getchar();
            if(c=='\n' || c=='\r') {
                vga_putc('\n');
                serial_putc('\n');
                line_buf[pos]=0;
                break;
            } else if(c=='\b') {
                if(pos>0){
                    pos--;
                    vga_putc('\b');
                    // serial backspace handled as \b
                    serial_putc('\b');
                    serial_putc(' ');
                    serial_putc('\b');
                }
            } else if(c==27) { // ESC
                // Clear line?
                // ignore
            } else {
                if(pos < SHELL_BUF-1){
                    line_buf[pos++]=c;
                    vga_putc(c);
                    serial_putc(c);
                }
            }
        }
        shell_execute(line_buf);
    }
}