// 4coder_lister_base.cpp:296
#define lister_fix \
i32 max_count = first_index + lister->visible_count + 6; \
count = clamp_top(lister->filtered.count, max_count);
