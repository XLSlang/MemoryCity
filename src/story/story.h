#ifndef STORY_H
#define STORY_H

#include <stdbool.h>

void  story_init(void);           // 扫描 assets/story/
void  story_tick(void);           // 每天调用，推进剧情
bool  story_active(void);
const char* story_current_text(void);

#endif