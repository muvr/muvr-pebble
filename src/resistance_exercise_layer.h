#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// Repetition count
typedef uint8_t repetitions_t;

/// Intensity in 0..100
typedef uint8_t intensity_t;

/// Duration in 0..16k seconds
typedef uint16_t duration_t;

/// Weight in user units (converted on the mobile). Beyond mobile, everything is in the metric system
typedef uint16_t weight_t;

#define UNK_REPETITIONS 255
#define UNK_WEIGHT 65535
#define UNK_DURATION 65535

typedef struct __attribute__((__packed__)) {
    char name[24];
    duration_t    duration;         // 0..100
    repetitions_t repetitions;      // 1..~600, UNK_DURATION for unknown
    intensity_t   intensity;        // 1..100,  UNK_INTENSITY for unknown
    weight_t      weight;           // 1..~500, UNK_WEIGHT for unknown
} resistance_exercise_t;

///
/// Callback that will be called when the classification UI has been dismissed and
/// all handlers cleared
///
typedef void (*screen_back_callback_t)(void);

///
/// The exercise that the user should be doing next.
///
void rex_set_current(resistance_exercise_t *exercise);

void rex_moving(void);

void rex_exercising(void);

void rex_not_moving(void);

void rex_empty(void);

Window* rex_init(screen_back_callback_t dismissed);

#ifdef __cplusplus
}
#endif
