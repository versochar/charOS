/* 15H: ET_DYN "program" (PIE) — dl_entry ile başlar, statik durum + interpose. */
extern int shared_hook(int);

static int pie_calls;
int pie_init_val = 3;

int pie_run(void) {
    pie_calls++;
    return shared_hook(2) + pie_init_val + pie_calls;
}
int pie_peek(void) { return pie_calls; }