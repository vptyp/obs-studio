#ifndef FF_ROUTINE_H
#define FF_ROUTINE_H

#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#ifdef __cplusplus
extern "C" {
#endif

#include <image_recognize.h>

typedef struct enc_data {
	long pts; // presentation timestamp
	char direction; // 0 - internal, 1 - external
	long time[2]; // time of frame
	int refs; // reference counter
}enc_data;

static irc_encoder* ff_encoder = NULL; //global variable for ffmpeg encoder
irc_encoder* make_ff_encoder();
irc_encoder* get_ffenc_encoder();
enc_data* make_enc_data(long pts, char direction);
#ifdef __cplusplus
}
#endif

#endif // FF_ROUTINE_H