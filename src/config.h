#define SMOOTH_RATIO 0.9f
#define STAND2
#define TENSO

#ifdef STAND1
#define PIN_SCK PB_4
#define PIN_MISO PB_5
#define PIN_ACH_1 PA_8
#define PIN_ACH_2 PF_1
#define PIN_PWM PA_11
#define PIN_CURRENT PA_0
#define TMP36
#define TEMP_PIN PA_4
#define NOISE_PIN PA_5
#define CURRENT_OFFSET 2380
#endif

#ifdef STAND2
#define PIN_SCK PB_4
#define PIN_MISO PB_5
#define PIN_ACH_1 PA_12
#define PIN_ACH_2 PB_7
#define PIN_PWM PB_0
#define PIN_CURRENT PA_4
#define TMP36 
#define TEMP_PIN PA_1
#define NOISE_PIN PA_0
#define CURRENT_OFFSET 2500
#endif
