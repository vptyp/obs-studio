#ifndef BASE_IRC_H
#define BASE_IRC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <image_recognize.h>

static irc_encoder* main_encoder = NULL; //global variable for ffmpeg encoder
irc_encoder* make_main_encoder();

#ifdef __cplusplus
}
#endif
#endif // BASE_IRC_H