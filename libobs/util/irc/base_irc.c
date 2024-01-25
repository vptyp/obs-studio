#include "media-io/format-conversion.h"
#include "media-io/video-frame.h"
#include "media-io/audio-io.h"
#include "util/threading.h"
#include "util/platform.h"
#include "util/util_uint64.h"
#include "callback/calldata.h"
#include "graphics/matrix3.h"
#include "graphics/vec3.h"
#include <stdlib.h>
#include<unistd.h>
#include <stdint-gcc.h>
#include <stdbool.h>
#include "obs.h"
#include "obs-internal.h"
#include "base_irc.h"


void* ref(void* key);
void deref(void* key);
int video_format_to_frame_data(const void* input, frame_data* output); 
enum internal_video_format get_video_format(void* input); 

irc_encoder* make_main_encoder(){
    main_encoder = (irc_encoder*) malloc(sizeof(irc_encoder));
    main_encoder->ref = ref;
    main_encoder->deref = deref;
    main_encoder->decode = video_format_to_frame_data;
    main_encoder->decode_video_format = get_video_format;
    main_encoder->routine = NULL;
    main_encoder->callback = NULL;
    memset(main_encoder->_outFilename, 0, sizeof(main_encoder->_outFilename));
    char filename[] = "irc_main_timestamps.txt";
    size_t i = 0;
    for(i = 0; i < strlen(filename); ++i){
        main_encoder->_outFilename[i] = filename[i];
    }
    return main_encoder;
}

void* ref(void* key){
    struct obs_source_frame* frame = (struct obs_source_frame*)key;
    os_atomic_inc_long(&frame->refs);
    return (void*)frame;
}

int video_format_to_frame_data(const void* input, frame_data* output){
    if(input == NULL || output == NULL){
        return -1;
    }
    struct obs_source_frame* frame = (struct obs_source_frame*)input;

    output->info.width = frame->width;
    output->info.height = frame->height;
    output->info.format = (enum internal_video_format) frame->format;
    int planes;
    int av_len = MAX_AV_PLANES;
    for(planes = 0; planes < av_len; planes++){
        if(frame->data[planes] == NULL){
            continue;
        }
        output->linesize[planes] = frame->linesize[planes];
        output->data[planes] = frame->data[planes];
    }
    return 0;
}

void deref(void* key){
    struct obs_source_frame* frame = (struct obs_source_frame*)key;
    if (os_atomic_dec_long(&frame->refs) == 0) {
        obs_source_frame_destroy(frame);
        frame = NULL;
    }
}

enum internal_video_format get_video_format(void* input){
    struct obs_source_frame* frame = (struct obs_source_frame*)input;
    return (enum internal_video_format)frame->format;
}