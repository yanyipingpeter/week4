cmake_minimum_required (VERSION 3.8)

project(week4)

include_directories("include")

link_libraries("lib")

add_executable (week4 "week4.cpp")

target_link_libraries(
	alAudioPlayer
	avcodec
	avdevice
	avfilter
	avformat
	avutil
	postproc
	swresample
	swscale
	EFX-Util
	OpenAL32
)

# TODO: 如有需要，请添加测试并安装目标。
