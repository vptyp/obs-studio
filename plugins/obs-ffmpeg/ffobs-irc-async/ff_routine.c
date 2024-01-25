#include "ff_routine.h"
#include <p_timer_file.h>

void* ff_reference_frame(void* frame){
    return av_frame_clone((AVFrame*)frame);
}

void ff_callback(void* event_data);

void ff_unref_frame(void* frame){
    if (frame != NULL) {
 //             free(frame);
 //             av_frame_free(&frame);
        av_frame_unref((AVFrame*)frame);
 //           free(frame);
    }
}

int avframe_to_frame_data(const void *input, frame_data* output);

enum internal_video_format video_format_from_pix_fmt(void* source);

irc_encoder* make_ff_encoder(){
    ff_encoder = (irc_encoder*) malloc(sizeof(irc_encoder));
    ff_encoder->ref = ff_reference_frame;
    ff_encoder->deref = ff_unref_frame;
    ff_encoder->decode = avframe_to_frame_data;
    ff_encoder->decode_video_format = video_format_from_pix_fmt;
    ff_encoder->routine = NULL;
	ff_encoder->callback = ff_callback;

    memset(ff_encoder->_outFilename, 0, sizeof(ff_encoder->_outFilename));
    const char filename[] = "irc_ffmpeg_timestamps.txt";
    for(size_t i = 0; i < strlen(filename); i++){
        ff_encoder->_outFilename[i] = filename[i];
    }
    return ff_encoder;
}

enum internal_video_format video_format_from_pix_fmt(void* source){
    if(source == NULL){
        return IVIDEO_FORMAT_NONE;
    }
    const AVFrame* frame = (AVFrame*)source;
    switch(frame->format){
        case AV_PIX_FMT_YUV420P:
            return IVIDEO_FORMAT_I420;
        case AV_PIX_FMT_NV12:
            return IVIDEO_FORMAT_NV12;
        case AV_PIX_FMT_UYVY422:
            return IVIDEO_FORMAT_UYVY;
        case AV_PIX_FMT_RGBA:
            return IVIDEO_FORMAT_RGBA;
        case AV_PIX_FMT_BGRA:
            return IVIDEO_FORMAT_BGRA;
        default:
            return IVIDEO_FORMAT_NONE;
    }
}

int avframe_to_frame_data(const void *input, frame_data* output){
    if(input == NULL || output == NULL){
        return -1;
    }
    AVFrame* frame = (AVFrame*)input;
    output->info.width = frame->width;
    output->info.height = frame->height;
    output->info.format = video_format_from_pix_fmt(frame);

    const int av_len = MAX_AV_PLANES;
    for(int planes = 0; planes < av_len; planes++){
        if(frame->data[planes] == NULL){
            continue;
        }
        output->linesize[planes] = frame->linesize[planes];
        output->data[planes] = frame->data[planes];
    }
    return 0;
}

enc_data* make_enc_data(const long pts, const char direction){
	enc_data* data = malloc(sizeof(enc_data));
	irc_get_current_time(data->time);
	data->pts = pts;
	data->direction = direction;
	data->refs = 0;
	return data;
}

void* encoded_ref(void* key)
{
	enc_data* data = key;
	++data->refs;
	return key;
}

void encoded_deref(void* key)
{
	enc_data* data = key;
	--data->refs;
	if(data->refs < 1) {
		free(data);
	}
}

//binary search first element > value in rotated array
int binary_search(long* array, int size, long value){
	int left = 0;
	int right = size - 1;
	int mid = 0;
	while(left < right){
		mid = (left + right) / 2;
		if(array[mid] > value){
			right = mid;
		} else {
			left = mid + 1;
		}
	}
	return left;
}

int pivoted_search(long* array, int size, long value, int pivot)
{
	if(pivot < 0 || size == 0 || value < 0) return -1;

	if(value > array[pivot]) {
		return binary_search(array, pivot, value);
	}
	return binary_search(array + pivot, size - pivot, value);
}
/**
 * \brief remove count elements from array based on pivot
 * \param array array to modify
 * \param size size of array
 * \param pivot rotation point
 * \param count number of elements to remove
 * \return 0 on success, -1 on error
 */
int circular_remove_before(long* array, size_t size, size_t* pivot, size_t count)
{
	if(size == 0) return -1;
	if(count == 0) return 0;
	if(count == size) {
		memset(array, 0, sizeof(long) * size);
		return 0;
	}
	long* right = malloc(sizeof(long) * *pivot);
	memcpy(right, array, sizeof(long) * *pivot);
	const size_t move = *pivot + count;
	size_t last_idx;
	//if move > size, then we don't need to move elements from smaller side
	if(move >= size) {
		//move elements from right (bigger) to beginning of array
		memmove(array, right + move - size, sizeof(long) * (move - size));
		last_idx = move - size;
	} else {
		//move elements from left (smaller) to the beginning of array
		memmove(array, array + move, sizeof(long) * (size - move));
		//append right (bigger) elements to the end of left side
		memmove(array + move, right, sizeof(long) * *pivot);
		last_idx = move + *pivot;
	}
	*pivot = last_idx;
	//zero out the rest of the array
	if(last_idx < size){
		memset(array + last_idx, 0, sizeof(long) * (size - last_idx));
	} else {
		*pivot = 0;
	}
	free(right);
	return 0;
}

void* encoded_routine(void* arg)
{
	#define buf_size 60
	irc_encoder* encoder = (irc_encoder*)arg;
	thread_data* data = irc_get_thread_data(encoder);
	void* frame = NULL;
	//all values < internArray[0] must be removed
	long externArray[buf_size];
	size_t pivotExt = 0;
	bool isPivotExt = false;
	long internArray[buf_size];
	size_t pivotInt = 0;
	bool isPivotInt = false;
	memset(externArray, 0, sizeof(externArray));
	memset(internArray, 0, sizeof(internArray));
	while(true) {
		if(irc_get_from_thread_queue(data, &frame) == 0) {
			enc_data* _temp = frame;
			long value = _temp->pts;
			char direction = _temp->direction;
			long time[2] = {_temp->time[0], _temp->time[1]};
			encoder->deref(_temp);
			if(!value) continue;

			if(direction) {
				externArray[pivotExt++] = value;
				if(pivotExt >= buf_size) {
					pivotExt = 0;
					isPivotExt = true;
				}
			} else {
				internArray[pivotInt++] = value;
				if(pivotInt >= buf_size) {
					pivotInt = 0;
					isPivotInt = true;
				}
			}

			int result = pivoted_search(externArray, isPivotExt ? buf_size : pivotExt,
							internArray[isPivotInt ? pivotInt : 0],
							isPivotExt ? pivotExt : 0);
			if(result > 0) {
				if(externArray[result] == internArray[isPivotInt ? pivotInt : 0]) {
					#define len 40
					char str[len];
					get_current_time_str(str, len, time[0], time[1]);
					p_timer_append_file(encoder->_outFilename, (char*) time,
											sizeof(long), 2);
					printf("%s: %s.%03ld\n", encoder->_outFilename, str, time[1]);
				}
				circular_remove_before(externArray, buf_size,
										&pivotExt, result);
				circular_remove_before(internArray, buf_size, &pivotInt, 1);
				isPivotExt = false;
			} else {
				if(result == 0) {
					result = pivoted_search(internArray, isPivotInt ? buf_size : pivotInt,
							externArray[isPivotExt ? pivotExt : 0],
							isPivotInt ? pivotInt : 0);
					if(result == 0 && internArray[result] != externArray[isPivotExt ? pivotExt : 0]){
						circular_remove_before(internArray, buf_size, &pivotInt, buf_size - 1);
					} else 
					if(result > 0) {
						circular_remove_before(internArray, buf_size,
												&pivotInt, result);
						isPivotInt = false;
					}
				}
			}
		}
	}
}

irc_encoder* make_ff_encoded_encoder(){
	irc_encoder* ffenc_encoder = malloc(sizeof(irc_encoder));
	ffenc_encoder->ref = encoded_ref;
	ffenc_encoder->deref = encoded_deref;
	ffenc_encoder->routine = encoded_routine;
	ffenc_encoder->decode = NULL;
	ffenc_encoder->decode_video_format = NULL;
	memset(ffenc_encoder->_outFilename, 0,
						sizeof(ffenc_encoder->_outFilename));
	const char filename[] = "irc_encFF_timestamps.txt";
	for(size_t i = 0; i < strlen(filename); i++){
		ffenc_encoder->_outFilename[i] = filename[i];
	}
	return ffenc_encoder;
}

void ff_callback(void* event_data){
	irc_encoder* ffenc_encoder = get_ffenc_encoder();
	const AVFrame* frame = (AVFrame*)event_data;
	printf("FFMPEG callback: %ld\n", frame->pts);
	irc_frame_to_queue(ffenc_encoder,
		ffenc_encoder->ref(make_enc_data(frame->pts, 0)));
}

irc_encoder* get_ffenc_encoder()
{
	static irc_encoder* ffenc_encoder = NULL;
	if(ffenc_encoder == NULL){
		ffenc_encoder = make_ff_encoded_encoder();
	}
	return ffenc_encoder;
}