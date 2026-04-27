#include <libavutil/imgutils.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timestamp.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>

#include "screencaps_utils.h"

#include <stdio.h>
#include <stdlib.h>

AVFormatContext *fmt_ctx = NULL;
AVCodecContext *video_dec_ctx = NULL, *audio_dec_ctx = NULL;

int width, height;
enum AVPixelFormat pix_fmt;

AVStream *video_stream = NULL, *audio_stream = NULL;

const char *video_file = NULL;

uint8_t *video_dst_data[4] = {NULL};
int      video_dst_linesize[4];
int video_dst_bufsize;

int video_stream_idx = -1, audio_stream_idx = -1;
AVFrame *frame = NULL;
AVPacket *pkt = NULL;
int video_frame_count = 0;
int audio_frame_count = 0;

static int output_video_frame(AVFrame *frame){
    if (frame->width != width || frame->height != height || frame->format != pix_fmt){
        /*To handle this change, one could call av_imge_alloc again and decode the following frames into another rawvideo file.*/
        fprintf(stderr, "Error: width, height, and pixel format have to be " 
                "constant in a rawvideo file, but the width, height or pixel format of the input video changed: \n" 
                "old: width = %d, height = %d, format = %s\n" 
                "new: width = %d, height = %d, format = %s\n", 
                width, height, av_get_pix_fmt_name(pix_fmt),
                frame->width, frame->height, av_get_pix_fmt_name(frame->format));
        return -1;
    }

    printf("Video_frame n: %d\n", video_frame_count++);

    /*Copy decoded frame to destination buffer:
      this will required since rawvideo expects non aligned data*/
      av_image_copy2(video_dst_data, video_dst_linesize, frame->data, frame->linesize, pix_fmt, width, height);

    return 0;
}

static int output_audio_frame(AVFrame *frame){
    size_t unpadded_linesize = frame->nb_samples *av_get_bytes_per_sample(frame->format);
    printf("Audio frame n: %d nb_sample: %d pts:%s\n", audio_frame_count++, frame->nb_samples, av_ts2timestr(frame->pts, &audio_dec_ctx->time_base));

    /*Write the raw audio data samples of the first plane. This work fine
    for packet formats (e.g. AV_SAMPLE_FMT_S16). However, most audio decoders output planar audio, which used a separate
    plane of audio samples for each channel (e.g. AV_SAMPLE_FMT_S16P).
    In other words, this code will write only the first audio channel in these cases.
    We should use libswresample or libavfilter to convert the frame to packet data.*/

    return 0;
}

static int decode_packet(AVCodecContext *dec, const AVPacket *pkt){
    int ret = 0;

    //Submit the packet to the decoder
    ret = avcodec_send_packet(dec, pkt);
    if (ret < 0){
        fprintf(stderr, "Error submitting a packet for decoding (%s)\n", av_err2str(ret));
        return ret;
    }

    //Get all the available frames from the decoder
    while (ret >= 0){
        ret = avcodec_receive_frame(dec, frame);
        if (ret < 0){
            //Those two return values are special and mean there is no output
            //Frame available, but there were no errors during decoding
            if (ret == AVERROR_EOF || ret == AVERROR(EAGAIN)){
                return 0;
            }
            fprintf(stderr, "Error during decoding (%s)\n", av_err2str(ret));
            return ret;
        }

        //write the frame data to output file
        if (dec->codec->type == AVMEDIA_TYPE_VIDEO){
            ret = output_video_frame(frame);
        }
        else{
            ret = output_audio_frame(frame);
        }
        av_frame_unref(frame);
    }

    return ret;
}

static int open_codec_context(int *stream_idx, AVCodecContext **dec_ctx, AVFormatContext *fmt_ctx, enum AVMediaType type){
    int ret, stream_index;
    AVStream *st;
    const AVCodec *dec = NULL;

    ret = av_find_best_stream(fmt_ctx, type, -1, -1, NULL, 0);
    if (ret < 0){
        fprintf(stderr, "Failed to find %s stream in input file '%s'\n", av_get_media_type_string(type), video_file);
        return ret;
    }
    else{
        stream_index = ret;
        st = fmt_ctx->streams[stream_index];

        /*Find decoder for the stream*/
        dec = avcodec_find_decoder(st->codecpar->codec_id);
        if (!dec){
            fprintf(stderr, "Failed to find %s codec\n", av_get_media_type_string(type));
            return AVERROR(EINVAL);
        }

        /* Allocate a codec context for the decoder*/
        *dec_ctx = avcodec_alloc_context3(dec);
        if (!*dec_ctx){
            fprintf(stderr, "Failed to allocate memory for the %s codec context\n", av_get_media_type_string(type));
            return AVERROR(ENOMEM);
        }

        /*Copy codec parameters from input stream to output codec context*/
        if ((ret = avcodec_parameters_to_context(*dec_ctx, st->codecpar)) < 0){
            fprintf(stderr, "Failed to copy %s codec parameters to decoder context\n", av_get_media_type_string(type));
            return ret;
        }

        /*Init the decoders*/
        if ((ret = avcodec_open2(*dec_ctx, dec, NULL)) < 0){
            fprintf(stderr, "Failed to open %s codec\n", av_get_media_type_string(type));
            return ret;
        }
        *stream_idx = stream_index;
    }

    return 0;
}

int process(char video_file_name[250]){
    int ret = 0; //ret is in short for return :D

    video_file = video_file_name;

    /*Open input file, and allocate format context*/
    if (avformat_open_input(&fmt_ctx, video_file, NULL, NULL) < 0){
        fprintf(stderr, "Failed to open video file %s\n", video_file);
        return EXIT_FAILURE;
    }

    /* Retrieve stream information*/
    if (avformat_find_stream_info(fmt_ctx, NULL) < 0){
        fprintf(stderr, "Failed to find stream information\n");
        return EXIT_FAILURE;
    }

    /*Open codec context for video frames*/
    if (open_codec_context(&video_stream_idx, &video_dec_ctx, fmt_ctx, AVMEDIA_TYPE_VIDEO) >= 0){
        video_stream = fmt_ctx->streams[video_stream_idx];

        /*Allocate image where the decoded image will be put*/
        width = video_dec_ctx->width;
        height = video_dec_ctx->height;
        pix_fmt = video_dec_ctx->pix_fmt;
        ret = av_image_alloc(video_dst_data, video_dst_linesize, width, height, pix_fmt, 1);

        if (ret < 0){
            fprintf(stderr, "Failed to allocate raw video buffer\n");
            goto end;
        }
        video_dst_bufsize = ret;
    }

    if (open_codec_context(&audio_stream_idx, &audio_dec_ctx, fmt_ctx, AVMEDIA_TYPE_AUDIO) >= 0){
        audio_stream = fmt_ctx->streams[audio_stream_idx];
    }

    /*Dump input information to stderr*/
    av_dump_format(fmt_ctx, 0, video_file, 0);

    if (!video_stream && !audio_stream){
        fprintf(stderr, "Could not find video or audio stream in the input file, aborting\n");
        goto end;
    }

    //Allocate memory for the frames
    frame = av_frame_alloc();
    if (!frame){
        fprintf(stderr, "Failed to allocate memory for frames\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    pkt = av_packet_alloc();
    if (!pkt){
        fprintf(stderr, "Failed to allocate memory for packet\n");
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /*Read frames from the file*/
    while (av_read_frame(fmt_ctx, pkt) >= 0){
        //Check if the packet belongs to a stream we are interested in, otherwise skip it
        if (pkt->stream_index == video_stream_idx){
            ret = decode_packet(video_dec_ctx, pkt);
        }
        else if (pkt->stream_index == audio_stream_idx){
            ret = decode_packet(audio_dec_ctx, pkt);
        }

        av_packet_unref(pkt);
        if (ret < 0){
            break;
        }
    }

    /*Flush the decoders*/
    if (video_dec_ctx){
        decode_packet(video_dec_ctx, NULL);
    }
    if (audio_dec_ctx){
        decode_packet(audio_dec_ctx, NULL);
    }

    printf("Demuxing succeeded\n");

    end:
        avcodec_free_context(&video_dec_ctx);
        avcodec_free_context(&audio_dec_ctx);
        avformat_close_input(&fmt_ctx);

        av_packet_free(&pkt);
        av_frame_free(&frame);
        return ret < 0;
}
