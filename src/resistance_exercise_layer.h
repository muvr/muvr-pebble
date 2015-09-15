#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/// Repetition count
typedef uint8_t repetitions_t;

/// Intensity in 0..100
typedef uint8_t intensity_t;

/// Classification confidence in 0..100
typedef uint8_t confidence_t;

/// Weight in user units (converted on the mobile). Beyond mobile, everything is in the metric system
typedef uint16_t weight_t;

#define UNK_REPETITIONS 255
#define UNK_INTENSITY 255
#define UNK_WEIGHT 65535

typedef struct __attribute__((__packed__)) {
    char name[24];
    confidence_t  confidence;       // 0..100
    repetitions_t repetitions;      // 1..~50,  UNK_REPETITIONS for unknown
    intensity_t   intensity;        // 1..100,  UNK_INTENSITY for unknown
    weight_t      weight;           // 1..~500, UNK_WEIGHT for unknown
} resistance_exercise_t;

///
/// Callback that will be called when the user confirms the exercise
///
typedef void (*classification_accepted_callback_t)(const uint8_t index);

///
/// Callback that will be called when the classification UI has been dismissed and
/// all handlers cleared
///
typedef void (*classification_dismissed_callback_t)(void);

///
/// Callback that will be called when the user rejects the exercise
///
typedef void (*classification_rejected_callback_t)(void);

///
/// Callback that will be called when the user just leaves the selection
///
typedef void (*classification_timedout_callback_t)(const uint8_t index);

///
/// Display the resistance_exercise examples.
///
void rex_classification_completed(resistance_exercise_t *exercises, uint8_t count,
                                  classification_accepted_callback_t accepted,
                                  classification_timedout_callback_t timed_out,
                                  classification_rejected_callback_t rejected);

///
/// The exercise that the user should be doing next.
///
void rex_set_current(resistance_exercise_t *exercise);

void rex_moving(void);

void rex_exercising(void);

void rex_not_moving(void);

void rex_empty(void);

Window* rex_init(classification_dismissed_callback_t dismissed);

#ifdef __cplusplus
}
#endif
