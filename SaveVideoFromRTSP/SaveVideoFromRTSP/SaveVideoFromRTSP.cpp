#include "stdafx.h"
#include "iostream"
#include "pch.h"  
#include <string>   

using namespace std;

AVFormatContext *inputAVFormatContext = nullptr;
AVCodecContext *inputAVCodecContext = nullptr;
AVFormatContext *outputAVFormatContext = nullptr;
AVCodecContext *outPutAVCodecContext = nullptr;
struct SwsContext *swsContex = nullptr;
AVRational inputTimeBase;
int inputVideoStream;
string inputStreamUrl;
string outputStreamUrlBase;

void initEnv() {

	av_register_all();
	avformat_network_init();
	av_log_set_level(AV_LOG_INFO);

	inputStreamUrl = "rtsp://192.168.3.34/test.264";
	outputStreamUrlBase = "G:\\Microsoft\\VisualStudio\\tmp\\";
}

int initInput(string inputURL) {

	//open the input stream and read the herder.The codecs are not opened
	inputAVFormatContext = avformat_alloc_context();
	int ret = avformat_open_input(&inputAVFormatContext, (const char *)inputURL.c_str(), nullptr, nullptr);
	if (ret < 0) {
		av_log(nullptr, AV_LOG_ERROR, " Open The input stream and read the header failed!(%s)\n", inputURL.c_str());
		return ret;
	}
	else {
		av_log(nullptr, AV_LOG_FATAL, "Open The input stream and read the header success!\n");
	}

	//Initialize the AVCodecContext to use the given(AVMEDIA_TYPE_VIDEO,we just decode video part later) AVCodec,
	inputVideoStream = av_find_best_stream(inputAVFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
	if (inputVideoStream < 0) {
		av_log(nullptr, AV_LOG_ERROR, "find Video stream failed in the input stream !\n");
		return -1;
	}
	else {
		av_log(nullptr, AV_LOG_FATAL, "find Video stream success in the input stream !\n");
	}

	inputTimeBase = inputAVFormatContext->streams[inputVideoStream]->time_base;
	AVCodec *avCodec = avcodec_find_decoder(inputAVFormatContext->streams[inputVideoStream]->codecpar->codec_id);
	inputAVCodecContext = avcodec_alloc_context3(avCodec);
	avcodec_parameters_to_context(inputAVCodecContext, inputAVFormatContext->streams[inputVideoStream]->codecpar);

	ret = avcodec_open2(inputAVCodecContext, avCodec, nullptr);
	if (ret < 0) {
		av_log(nullptr, AV_LOG_ERROR, "Initialize the input stream AVCodecContext to use the given AVCodec(AVMEDIA_TYPE_VIDEO) failed!\n");
		return ret;
	}
	else {
		av_log(nullptr, AV_LOG_FATAL, "Initialize the input stream AVCodecContext to use the given AVCodec(AVMEDIA_TYPE_VIDEO) success!\n");
	}
	return 0;
}

int initOutput(const char* outputURL, AVRational timebase, AVCodecContext *inputCC) {
	if ((outputURL == nullptr) || (inputCC == nullptr)) {
		av_log(nullptr, AV_LOG_ERROR, "output paramaters is nullptr");
		return -1;
	}

	AVCodec *videoCodec;
	videoCodec = avcodec_find_encoder(AV_CODEC_ID_H264);
	outPutAVCodecContext = avcodec_alloc_context3(videoCodec);
	outPutAVCodecContext->codec_id = videoCodec->id;
	outPutAVCodecContext->time_base.num = timebase.num;
	outPutAVCodecContext->time_base.den = timebase.den;
	outPutAVCodecContext->pix_fmt = *videoCodec->pix_fmts;
	outPutAVCodecContext->width = inputCC->width;
	outPutAVCodecContext->height = inputCC->height;
	int ret = avcodec_open2(outPutAVCodecContext, videoCodec, nullptr);
	if (ret < 0) {
		av_log(nullptr, AV_LOG_ERROR, "Initialize the out stream AVCodecContext  to use the given AVCodec(AV_CODEC_ID_H264) failed!\n");
		return ret;
	}
	else {
		av_log(nullptr, AV_LOG_FATAL, "Initialize the out stream AVCodecContext  to use the given AVCodec(AV_CODEC_ID_H264) success!\n");
	}

	ret = avformat_alloc_output_context2(&outputAVFormatContext, nullptr, nullptr, outputURL);
	if (ret < 0) {
		av_log(nullptr, AV_LOG_ERROR, outputURL);
		av_log(nullptr, AV_LOG_ERROR, "Allocate an AVFormatContext failed for the outputURL!(%s)*\n", outputURL);
		return ret;
	}
	else {
		av_log(nullptr, AV_LOG_FATAL, "Allocate an AVFormatContext success for the outputURL!(%s)*\n", outputURL);
	}

	ret = avio_open(&(outputAVFormatContext->pb), outputURL, AVIO_FLAG_WRITE);
	if (ret < 0) {
		av_log(nullptr, AV_LOG_ERROR, "Create and initialize a AVIOContext failed for accessing the resource indicated by the outputUrl!\n");
		return ret;
	}
	else {
		av_log(nullptr, AV_LOG_FATAL, "Create and initialize a AVIOContext success for accessing the resource indicated by the outputUrl!\n");
	}
	
	AVStream *outAvStream = avformat_new_stream(outputAVFormatContext, videoCodec);
	if (outAvStream == nullptr) {
		av_log(nullptr, AV_LOG_ERROR, "Add a output stream to a media file for mux later failed!\n");
		return -1;
	}
	else {
		av_log(nullptr, AV_LOG_FATAL, "Add a output stream to a media file for mux later  success!\n");
	}
	avcodec_parameters_from_context(outputAVFormatContext->streams[0]->codecpar,outPutAVCodecContext);
	return 0;
}

int decode(AVCodecContext *inputCC, AVPacket *packet, AVFrame *frame, int packetIndex) {
	int ret = avcodec_send_packet(inputCC, packet);
	if (ret < 0) {
		if (ret == AVERROR(EAGAIN)) {
			av_log(nullptr, AV_LOG_ERROR, "send packet failed( errcode = AVERROR(EAGAIN) )!\n");
		}else if (ret == AVERROR_EOF) {
			av_log(nullptr, AV_LOG_ERROR, "send packet failed( errcode = AVERROR_EOF )!\n");
		}else {
			av_log(nullptr, AV_LOG_ERROR, "send packet failed( errcode = %d )!\n",ret);
		}
		return ret;
	}
	else {
		if (packetIndex % 20 == 0 || packetIndex < 20)
			av_log(nullptr, AV_LOG_FATAL, "%d send packet success(%d)\n", packetIndex, packet->dts);
	}
	av_frame_unref(frame);

	ret = avcodec_receive_frame(inputCC, frame);
	if (ret < 0) {
		if (ret == AVERROR(EAGAIN)) {
			av_log(nullptr, AV_LOG_ERROR, "receive frame failed( errcode = AVERROR(EAGAIN) )!\n");
		}
		else if (ret == AVERROR_EOF) {
			av_log(nullptr, AV_LOG_ERROR, "receive frame failed( errcode = AVERROR_EOF )!\n");
		}
		else {
			av_log(nullptr, AV_LOG_ERROR, "receive frame failed( errcode = %d )!\n", ret);
		}
		return ret;
	}
	else {
		if (packetIndex % 20 == 0 || packetIndex < 20) {
			av_log(nullptr, AV_LOG_FATAL, "%d receive frame success(%d)\n", packetIndex, frame->best_effort_timestamp);
		}
	}
	return 0;
}

int encode(AVCodecContext *outCC, AVPacket *packet, AVFrame *frame, int packetIndex) {
	int ret = avcodec_send_frame(outCC, frame);
	if (ret < 0) {
		if (ret == AVERROR(EAGAIN)) {
			av_log(nullptr, AV_LOG_ERROR, "send frame failed( errcode = AVERROR(EAGAIN) )!\n");
		}
		else if (ret == AVERROR_EOF) {
			av_log(nullptr, AV_LOG_ERROR, "send frame failed( errcode = AVERROR_EOF )!\n");
		}
		else {
			av_log(nullptr, AV_LOG_ERROR, "send frame failed( errcode = %d )!\n", ret);
		}
		return ret;
	}
	else {
		if (packetIndex % 20 == 0 || packetIndex < 20) {
			av_log(nullptr, AV_LOG_FATAL, "%d send frame success(%d)\n", packetIndex, frame->best_effort_timestamp);
		}
	}
	av_packet_unref(packet);
	ret = avcodec_receive_packet(outCC, packet);
	if (ret < 0) {
		if (ret == AVERROR(EAGAIN)) {
			av_log(nullptr, AV_LOG_ERROR, "receive packet failed( errcode = AVERROR(EAGAIN) )!\n");
		}
		else if (ret == AVERROR_EOF) {
			av_log(nullptr, AV_LOG_ERROR, "receive packet failed( errcode = AVERROR_EOF )!\n");
		}
		else {
			av_log(nullptr, AV_LOG_ERROR, "receive packet failed( errcode = %d )!\n", ret);
		}
		return ret;
	}
	else {
		if (packetIndex % 20 == 0 || packetIndex < 20)
			av_log(nullptr, AV_LOG_FATAL, "%d receive packet success(%d)\n", packetIndex, packet->dts);
	}
	return 0;
}

int writeHeader(AVFormatContext *outFX) {

	int ret = avformat_write_header(outFX, nullptr);
	if (ret < 0) {
		av_log(nullptr, AV_LOG_ERROR, "Allocate the stream private data and write the stream header to the media file failed!\n");
		return ret;
	}
	else {
		av_log(nullptr, AV_LOG_FATAL, "Allocate the stream private data and write the stream header to the media file success!\n");
	}
	return 0;
}

int writepacket(AVFormatContext *outFX, AVPacket *packet, AVFormatContext *inputFX, AVFrame *inputFrame,int packetIndex) {
	//int64_t tmp = packetIndex * outFX->streams[0]->time_base.den / outFX->streams[0]->time_base.num / 30;
	//get src timestamp
	//int64_t scrTime = av_frame_get_best_effort_timestamp(inputFrame)*av_q2d(inputFX->streams[0]->time_base);
	//get dst timestamp
	//int64_t dstTime = scrTime / av_q2d(outFX->streams[0]->time_base);
	packet->pts = packet->dts = av_rescale_q(av_frame_get_best_effort_timestamp(inputFrame), inputFX->streams[0]->time_base, outFX->streams[0]->time_base);
	int ret = av_interleaved_write_frame(outFX, packet);
	if (ret < 0) {
		av_log(nullptr, AV_LOG_ERROR, "Write a packet to the output media file failed!\n");
		return ret;
	}
	else {
		if (packetIndex % 20 == 0 || packetIndex < 20)
			av_log(nullptr, AV_LOG_FATAL, "Write a packet to the output media file success!\n");
	}
	return ret;
}

int initSws(SwsContext **swsC,AVCodecContext *inputCC, AVCodecContext *outputCC,AVFrame *outputF) {
	*swsC = sws_getContext(inputCC->width, inputCC->height, inputCC->pix_fmt, outputCC->width, outputCC->height, outputCC->pix_fmt, SWS_BICUBIC, nullptr, nullptr, nullptr);
	if (*swsC == nullptr) {
		av_log(nullptr, AV_LOG_ERROR, "get SWSContext failed!\n");
		return -1;
	}

	int numBytes = av_image_get_buffer_size(outputCC->pix_fmt, outputCC->width, outputCC->height, 1);
	if (numBytes < 0) {
		av_log(nullptr, AV_LOG_ERROR, "get Image buffer failed!\n");
		return numBytes;
	}

	const uint8_t *outputSwpBuffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
	int ret = av_image_fill_arrays(outputF->data, outputF->linesize, outputSwpBuffer, outputCC->pix_fmt, outputCC->width, outputCC->height, 1);
	if (ret < 0) {
		av_log(nullptr, AV_LOG_ERROR, "set outputFrame data point and linesize failed!\n");
		return ret;
	}
	outputF->width = outputCC->width;
	outputF->height = outputCC->height;
	outputF->format = outputCC->pix_fmt;
	return 0;
}


void releaseInputResource() {
	if (inputAVFormatContext != nullptr)
	{
		avformat_free_context(inputAVFormatContext);
	}
}


void releaseOutputResource() {
	if (outputAVFormatContext != nullptr)
	{
		avformat_free_context(outputAVFormatContext);
	}
}

void closeInput()
{
	if (inputAVFormatContext != nullptr)
	{
		avformat_close_input(&inputAVFormatContext);
	}
	if (inputAVCodecContext != nullptr) {
		avcodec_close(inputAVCodecContext);
	}	
}

void closeoutput()
{
	if (outputAVFormatContext != nullptr)
	{
		int ret = av_write_trailer(outputAVFormatContext);
		avformat_close_input(&outputAVFormatContext);
	}
	if (outPutAVCodecContext != nullptr) {
		avcodec_close(outPutAVCodecContext);
	}
	if (outputAVFormatContext != nullptr)
	{
		avformat_free_context(outputAVFormatContext);
	}
}

//int _tmain(int argc, _TCHAR* argv[]) {
//int main(int argc, char* argv[]) {
int main() {

	initEnv();

	int ret = initInput(inputStreamUrl);
	if (ret < 0) {
		closeInput();
		releaseInputResource();
		return ret;
	}
	av_dump_format(inputAVFormatContext, 0, inputStreamUrl.c_str(), 0);	

	AVPacket *avPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
	AVFrame *inputAVFrame = av_frame_alloc();
	AVFrame *outputAVFrame = av_frame_alloc();
	int avPacketIndex = 0;
	int64_t startTime = av_gettime();
	bool isOuputInited = false;
	while (av_read_frame(inputAVFormatContext, avPacket) >= 0) {		
		if (av_gettime() - startTime > 30000000) {
			break;
		}
		if (inputVideoStream == avPacket->stream_index) {			
			avPacketIndex++;
			ret = decode(inputAVCodecContext, avPacket, inputAVFrame, avPacketIndex);
			av_packet_unref(avPacket);
			if (ret < 0) {
				continue;
			}
			if (!isOuputInited) {
				string outputStreamUrl = outputStreamUrlBase;
				outputStreamUrl.append("rtsp2h264.mp4");
				ret = initOutput(outputStreamUrl.c_str(), inputTimeBase, inputAVCodecContext);
				if (ret < 0) {
					break;
				}

				ret = writeHeader(outputAVFormatContext);
				if (ret < 0) {
					break;
				}

				ret = initSws(&swsContex,inputAVCodecContext,outPutAVCodecContext,outputAVFrame);
				if (ret < 0) {
					break;
				}
				isOuputInited = true;
			}
			sws_scale(swsContex, (const uint8_t *const *)inputAVFrame->data,inputAVFrame->linesize, 0, inputAVFrame->height, (uint8_t *const *)outputAVFrame->data, outputAVFrame->linesize);
			ret = encode(outPutAVCodecContext, avPacket, outputAVFrame, avPacketIndex);
			if (ret < 0) {
				continue;
			}			
			ret = writepacket(outputAVFormatContext, avPacket,inputAVFormatContext,inputAVFrame,avPacketIndex);
			av_packet_unref(avPacket);
			if (ret < 0) {
				continue;
			}
		}
		else {
			//printf("%d audio packet\n",index);
		}
	}
	av_frame_free(&inputAVFrame);
	av_frame_free(&outputAVFrame);
	closeInput();
	if (swsContex != nullptr) {
		sws_freeContext(swsContex);
	}
	closeoutput();
	releaseOutputResource();
	releaseInputResource();
	return 0;
}

/*
avcodec.lib
avdevice.lib
avfilter.lib
avformat.lib
avutil.lib
postproc.lib
swresample.lib
swscale.lib
*/