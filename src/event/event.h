#ifndef EVENT_H
#define EVENT_H

#include <stdbool.h>

#define EVENT_MAX_CHOICES 4

typedef struct {
    char id[32];
    char title[64];
    char desc[256];
    char portrait[128];

    int  cond_min_day;
    int  cond_min_pop;
    int  cond_prob;
    bool fired;

    int  choice_count;
    char choice_text[EVENT_MAX_CHOICES][64];
    int  choice_money[EVENT_MAX_CHOICES];
    int  choice_pop[EVENT_MAX_CHOICES];
    char choice_result[EVENT_MAX_CHOICES][128];
} EventDef;

void  event_init(void);
int   event_count(void);
bool  event_try_trigger(void);
bool  event_is_active(void);
const EventDef* event_current(void);
void  event_apply_choice(int idx);
void  event_close(void);
bool  event_force_trigger(const char *id);

#endif