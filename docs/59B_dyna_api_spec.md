# 59B - Dynamic Analysis API Spec

## API
```c
int dyna64_init(void);
int dyna64_probe_add(u32 addr, int granularity);
int dyna64_probe_remove(u32 addr);
int dyna64_start(void);
int dyna64_stop(void);
int dyna64_step(u32 *out_pc, int *out_event);
int dyna64_event_count(int ev);
int dyna64_log_dump(char *buf, int max);
int dyna64_trace_flush(void);
int dyna64_stack_snapshot(u32 *stack, int *depth);
int dyna64_config_set(int key, int val);
int dyna64_config_get(int key, int *out);
```
