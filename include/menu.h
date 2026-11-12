// Keyboard input and menu parsing.
#ifndef MENU_H
#define MENU_H

#include <stddef.h>

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

MenuChoice_t parseMenuChoice(const char *text);

MenuChoice_t readMenuChoice(void);

Status_t readFileName(char *buffer, size_t size);

Status_t readIdentifier(uint16_t *id);

Status_t readFrameFromUser(CanFrame_t *frame);

#ifdef __cplusplus
}
#endif

#endif // MENU_H 