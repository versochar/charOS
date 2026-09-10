# 36B - Ses API
int pcm_init(void);
int pcm_write(const int16_t* s, uint32_t n);
int pcm_read(int16_t* out, uint32_t n);
uint32_t pcm_avail(void);
uint32_t pcm_underruns(void);
uint32_t pcm_overruns(void);
