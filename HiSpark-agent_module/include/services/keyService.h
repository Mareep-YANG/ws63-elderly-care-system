#include "bits/alltypes.h"
#include "platform_core_rom.h"
#include "stdbool.h"

void KeyIsr(pin_t pin, uintptr_t param);
void KeyInit(void);
bool KeyConsumeEvent(void);