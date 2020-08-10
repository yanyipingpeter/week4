#include <iostream>
#include <stdio.h>
#include <vector>

extern "C"
{

#include <AL/al.h>
#include <AL/alc.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
};


using std::cin;
using std::cout;
using std::string;
using namespace std;
#define BUFFER_NUM 3
#define	SERVICE_UPDATE_PERIOD 20
#define MAX_AUDIO_FRME_SIZE 192000

void initOpenAL(ALuint source) {
	ALfloat SourceP[] = { 0.0,0.0,0.0 };
	ALfloat SourceV[] = { 0.0,0.0,0.0 };
	ALfloat ListenerP[] = { 0.0,0.0 };
	ALfloat ListenerV[] = { 0.0,0.0,0.0 };
	ALfloat ListenerO[] = { 0.0,0.0,-1.0,0.0,1.0,0.0 };
	alSourcef(source, AL_PITCH, 1.0);
	alSourcef(source, AL_GAIN, 1.0);
	alSourcefv(source, AL_POSITION, SourceP);
	alSourcefv(source, AL_VELOCITY, SourceV);
	alSourcef(source, AL_REFERENCE_DISTANCE, 50.0f);
	alSourcei(source, AL_LOOPING, AL_FALSE);
}


int main(int argc, char* argv[])
{
	//ffmpeg对于封装文件进行解码
	AVCodec* pcodec;//对应一种编解码器
	AVStream* pStream;//代表音频流
	AVFormatContext* pformatContext;//封装格式上下文结构体
	AVCodecContext* pcodecContext;//编解码器上下文结构体

	if (argc != 2)
	{
		printf("Usage：%s 'path of the video/audio file'\n", argv[0]);
	}
	avformat_network_init();
	pformatContext = avformat_alloc_context();//初始化
	if (avformat_open_input(&pformatContext, argv[1], NULL, NULL) != 0)//打开音频/视频流
	{
		cout<<"Couldn't open the input file"<<endl;
		return -1;
	}
	
	if (avformat_find_stream_info(pformatContext, NULL) < 0)//查找视频/音频流信息
	{
		cout<<"Couldn't find the stream information"<<endl;
		return -1;
	}
	av_dump_format(pformatContext, 0, argv[1], false);

	//查找音频流
	int index = -1;
	for (int i = 0; i < pformatContext->nb_streams; i++)
	{
		if (pformatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO)
		{
			index = i;
			break;
		}
	}
	if (index == -1)
	{
		cout<<"Couldn't find a audio stream"<<endl;
		return -1;
	}

	//查找解码器
	pStream = pformatContext->streams[index];
	pcodecContext = avcodec_alloc_context3(NULL);
	avcodec_parameters_to_context(pcodecContext, pStream->codecpar);
	pcodec = avcodec_find_decoder(pcodecContext->codec_id);
	pcodecContext->pkt_timebase = pformatContext->streams[index]->time_base;
	if (pcodec == NULL)//查找解码器
	{
		cout<<"Couldn't find decoder"<<endl;
		return -1;
	}
	//打开解码器
	if (avcodec_open2(pcodecContext, pcodec, NULL) < 0)
	{
		cout<<"Couldn't open decoder"<<endl;
		return -1;
	}

	AVPacket* packet = (AVPacket*)av_malloc(sizeof(AVPacket));//压缩数	
	AVFrame* frame = av_frame_alloc();//解压缩数据
	SwrContext* swr = swr_alloc();//重采样
	
	enum AVSampleFormat in_sample_fmt = pcodecContext->sample_fmt;//输入采样格式
	enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;	//输出采样格式
	int in_sample_rate = pcodecContext->sample_rate;//输入采样率
	int out_sample_rate = in_sample_rate;	//输出采样率
	uint64_t in_ch_layout = pcodecContext->channel_layout;	//输入声道布局
	uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;	//输出声道布局为立体声
	swr_alloc_set_opts(swr,out_ch_layout, out_sample_fmt, out_sample_rate,in_ch_layout, in_sample_fmt, in_sample_rate,0, NULL);
	swr_init(swr);
	int nb_out_channel = av_get_channel_layout_nb_channels(out_ch_layout);//输出声道个数

	//PCM
	uint8_t* out_buffer = (uint8_t*)av_malloc(MAX_AUDIO_FRME_SIZE);
	int out_buffer_size;
	FILE* fpPCM = fopen("output.pcm", "wb");
	while (av_read_frame(pformatContext, packet) >= 0)//读取压缩数据
	{
		if (packet->stream_index == index)
		{
			
			int ret = avcodec_send_packet(pcodecContext, packet);//解码
			if (ret != 0)
			{
				cout<<"Couldn't submit the packet to the decoder"<<endl;
				exit(1);
			}
			int getvideo = avcodec_receive_frame(pcodecContext, frame);
			if (getvideo == 0)
			{
				swr_convert(swr, &out_buffer, MAX_AUDIO_FRME_SIZE, (const uint8_t**)frame->data, frame->nb_samples);
				out_buffer_size = av_samples_get_buffer_size(NULL, nb_out_channel,frame->nb_samples, out_sample_fmt, 1);//抽样样本大小
				fwrite(out_buffer, 1, out_buffer_size, fpPCM);
			}
		}
		av_packet_unref(packet);
	}

	//openAL
	ALCdevice* pDevice = alcOpenDevice(NULL);
	ALCcontext* pContext = alcCreateContext(pDevice, NULL);
	ALuint source;
	alcMakeContextCurrent(pContext);
	if (alcGetError(pDevice) != ALC_NO_ERROR)
		return AL_FALSE;
	alGenSources(1, &source);
	if (alGetError() != AL_NO_ERROR)
	{
		cout<<"Couldn't generate audio source"<<endl;
		return -1;
	}

	initOpenAL(source);//初始化
	
	FILE* pcm = NULL;//打开PCM
	if ((pcm = fopen("output.pcm", "rb")) == NULL)
	{
		cout<<"Failed open the PCM file"<<endl;
		return -1;
	}

	ALuint alBufferArray[BUFFER_NUM];
	alGenBuffers(BUFFER_NUM, alBufferArray);

	int seek_location = 0;//填充数据
	for (int i = 0; i < BUFFER_NUM; i++)
	{
		int ret = 0;
		char ndata[4096 + 1] = { 0 };
		fseek(pcm, seek_location, SEEK_SET);
		ret = fread(ndata, 1,4096, pcm);
		alBufferData(alBufferArray[i], AL_FORMAT_STEREO16, ndata, 4096, out_sample_rate);
		alSourceQueueBuffers(source, 1, &alBufferArray[i]);
		seek_location += 4096;
	}

	
	alSourcePlay(source);//播放

	ALint total_buf_count = 0;
	ALint buffer_count;
	ALint iState;
	ALuint bufferId;
	ALint iQueuedBuffers;

	while (true)
	{
		buffer_count = 0;
		alGetSourcei(source, AL_BUFFERS_PROCESSED, &buffer_count);
		total_buf_count += buffer_count;
		total_buf_count += buffer_count;
		while (buffer_count > 0) {
			bufferId = 0;
			alSourceUnqueueBuffers(source, 1, &bufferId);
			int ret = 0;
			char ndata[4096 + 1] = { 0 };
			fseek(pcm, seek_location, SEEK_SET);
			ret = fread(ndata, 1, 4096, pcm);
			alBufferData(bufferId, AL_FORMAT_STEREO16, ndata, 4096, out_sample_rate);
			alSourceQueueBuffers(source, 1, &bufferId);
			seek_location += 4096;
			buffer_count -= 1;
		}
		alGetSourcei(source, AL_SOURCE_STATE, &iState);
		if (iState != AL_PLAYING) {
			alGetSourcei(source, AL_BUFFERS_QUEUED, &iQueuedBuffers);
			if (iQueuedBuffers) {
				alSourcePlay(source);
			}
			else {
				break;
			}
		}
	}

	alSourceStop(source);
	alSourcei(source, AL_BUFFER, 0);
	alDeleteSources(1, &source);
	alDeleteBuffers(BUFFER_NUM, alBufferArray);

	fclose(pcm);
	fclose(fpPCM);

	alcMakeContextCurrent(NULL);
	alcDestroyContext(pContext);
	pContext = NULL;
	alcCloseDevice(pDevice);
	pDevice = NULL;
	av_frame_free(&frame);
	av_free(out_buffer);
	swr_free(&swr);
	avcodec_close(pcodecContext);
	avformat_close_input(&pformatContext);

	return 0;
}