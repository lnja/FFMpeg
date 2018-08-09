// FFMpegProject.cpp: 定义控制台应用程序的入口点。
//
#include "stdafx.h"
#include "iostream"
#include "pch.h"

using namespace std;
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
int main() {

	cout << avcodec_configuration() << endl;
	//cout << "hello world!" << endl;

	
	av_register_all();
	av_log_set_level(AV_LOG_ERROR);

	AVFormatContext *fCx = avformat_alloc_context();
	string url = "G:\\Microsoft\\VisualStudio\\tmp\\test1.mp4";

	int ret = avformat_open_input(&fCx, (char *)url.c_str(), NULL, NULL);
	if (ret < 0) {
		cout << "open file fail!" << endl;
		return 0;
	}
	else {
		cout << "open file success!" << endl;
	}


	AVFormatContext *outFX;
	string path = "G:\\Microsoft\\VisualStudio\\tmp\\test.jpg";
	ret = avformat_alloc_output_context2(&outFX,NULL,NULL,path.c_str());
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "init output format context failed!\n");
		return 0;
	}
	ret = avio_open(&(outFX->pb),path.c_str(),AVIO_FLAG_WRITE);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "open output file io failed!\n");
		return 0;
	}

	int videoStream = av_find_best_stream(fCx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);	
	printf("videsStreamIndex = %d\n", videoStream);
	AVCodec *avCodec = avcodec_find_decoder(fCx->streams[videoStream]->codecpar->codec_id);
	AVCodecContext *avCodecContext;
	//avCodecContext = avcodec_alloc_context3(NULL);
	avCodecContext = avcodec_alloc_context3(avCodec);
	avcodec_parameters_to_context(avCodecContext, fCx->streams[videoStream]->codecpar);	
	//av_codec_set_pkt_timebase(avCodecContext, fCx->streams[videoStream]->time_base);
	//avcodec_copy_context(stream->codec, inputContext->streams[i]->codec);

	
	/*AVCodecParameters *outCP = avcodec_parameters_alloc();
	avcodec_parameters_from_context(outCP,avCodecContext);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "output-codec-paras copy from input-codec-paras failed!\n");
		return 0;
	}
	outAvStream->codecpar = outCP;*/

	

	ret = avcodec_open2(avCodecContext, avCodec, NULL);
	if (ret < 0) {
		printf("decode file fail\n");
	}

	AVCodec *picCodec;
	picCodec = avcodec_find_encoder(AV_CODEC_ID_MJPEG);

	AVStream *outAvStream = avformat_new_stream(outFX, picCodec);
	if (outAvStream == NULL) {
		av_log(NULL, AV_LOG_ERROR, "new  output stream failed!\n");
		return 0;
	}
	ret = avformat_write_header(outFX,NULL);
	if (ret < 0) {
		av_log(NULL, AV_LOG_ERROR, "output-format-context write header fail!\n");
		return 0;
	}
	av_log(NULL, AV_LOG_FATAL, "open output file success!\n");

	AVCodecContext *outCC = avcodec_alloc_context3(picCodec);
	outCC->codec_id = picCodec->id;
	outCC->time_base.num = fCx->streams[videoStream]->time_base.num;
	outCC->time_base.den = fCx->streams[videoStream]->time_base.den;
	outCC->pix_fmt = *picCodec->pix_fmts;
	outCC->width = avCodecContext->width;
	outCC->height = avCodecContext->height;
	ret = avcodec_open2(outCC, picCodec, NULL);
	if (ret < 0) {
		printf("open encoder fail\n");
	}

	AVPacket *avPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
	AVFrame *avFrame = av_frame_alloc();
	int index = 0;
	while (av_read_frame(fCx, avPacket) >= 0) {
		index++;
		if (videoStream == avPacket->stream_index) {

			ret = avcodec_send_packet(avCodecContext, avPacket);
			if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
				printf("send packet fail\n");
			}
			else {
				if (index % 100 == 0)
					printf("%d send packet success(%d)\n", index, avPacket->dts);
			}
			av_frame_unref(avFrame);

			ret = avcodec_receive_frame(avCodecContext, avFrame);
			if (ret < 0 &&
				ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
				printf("receive  packet fail\n");
			}
			else {
				if (index % 100 == 0) {
					printf("%d receive frame success(%d)\n", index, avFrame->best_effort_timestamp);
				}
				if(index == 2000){
					ret = avcodec_send_frame(outCC,avFrame);
					if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
						printf("send frame fail\n");
					}
					av_packet_unref(avPacket);
					ret = avcodec_receive_packet(outCC,avPacket);
					if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
						printf("recieve packet fail\n");
					}
					av_interleaved_write_frame(outFX, avPacket);
				}


			}
			av_packet_unref(avPacket);
		}
		else {
			//printf("%d audio packet\n",index);
		}
	}


	/*
	ret = avformat_find_stream_info(fCx, NULL);
	if (ret < 0) {
		cout << "find stream fail!" << endl;
	}
	else {
		cout << "find stream success!" << endl;
	}
	printf("steams = %d;\nname = %s;\nduration = %llds;\niformat long name = %s;\niformat name = %s\n",
		fCx->nb_streams, fCx->filename, fCx->duration, fCx->iformat->long_name,
		fCx->iformat->name);


	AVPacket *avPacket = (AVPacket *)av_malloc(sizeof(AVPacket));
	AVFrame *avFrame = av_frame_alloc();
	int index = 0;
	while (av_read_frame(fCx, avPacket) >= 0) {
		index++;
		if (videoStream == avPacket->stream_index) {

			ret = avcodec_send_packet(avCodecContext, avPacket);
			if (ret < 0 && ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
				printf("send packet fail\n");
			}
			else {
				if (index % 100 == 0)
				printf("%d send packet success(%d)\n",index,avPacket->dts);
			}
			av_frame_unref(avFrame);

			ret = avcodec_receive_frame(avCodecContext, avFrame);
			if (ret < 0 &&
				ret != AVERROR(EAGAIN) && ret != AVERROR_EOF) {
				printf("receive  packet fail\n");
			}
			else {
				if(index%100 == 0)
				printf("%d receive frame success(%d)\n",index,avFrame->best_effort_timestamp);
			}
			av_packet_unref(avPacket);
		}
		else {
			//printf("%d audio packet\n",index);
		}
	}
	*/

	return 0;
}