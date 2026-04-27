#ifndef SCREENCAPS_UTILS_H
#define SCREENCAPS_UTILS_H

#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

extern AVFormatContext *fmt_ctx;
extern AVCodecContext *video_dec_ctx, *audio_dec_ctx;

extern int width, height;
extern enum AVPixelFormat pix_fmt;

extern AVStream *video_stream, *audio_stream;

extern const char *video_file;

extern uint8_t *video_dst_data[4];
extern int      video_dst_linesize[4];
extern int video_dst_bufsize;

extern int video_stream_idx, audio_stream_idx;
extern AVFrame *frame;
extern AVPacket *pkt;
extern int video_frame_count;
extern int audio_frame_count;

static int output_video_frame(AVFrame *frame);
static int output_audio_frame(AVFrame *frame);
static int decode_packet(AVCodecContext *dec, const AVPacket *pkt);
static int open_codec_context(int *stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type);

int process(char video_file[250]); //Return 0 mean working, and 1 mean something inside not working

#endif