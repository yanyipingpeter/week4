# Week3
=====================================================
### Development Enviroment
 - Visual Studio 2017

## Program Description
 - 主程序`week4.cpp`，借鉴了ffmpeg官方例子https://github.com/FFmpeg/FFmpeg/tree/master/doc/examples。
 - 编译后，在Debug/Release中运行exe, 亦可以直接在Debug文件夹中运行week4.exe。
 - 运行方法为，在week4.exe的路径中打开cmd 输入 week4 MP3文件名.mp3或者week4 MP4文件名.mp4。
 - 程序目前仅测试了MP3 和MP4的播放，其余视频封装格式未测试。

###Developer
 - YYP

###Instructions
 - 程序目录下打开cmd， 输入week4 test.mp3 或test.mp4
 - test.mp3和test.mp4 是测试音频,文件名作为启动参数
 - 传递给程序的启动参数是mp3/mp4文件的文件路径，若mp3/mp4文件与`week4.exe`文件在同一文件夹下，则可直接将mp4文件名作为启动参数  
 - 若遇到`查找不到.dll文件`错误信息，请将所需的`.dll`文件复制到week4.exe的相同目录下即可解决。  
