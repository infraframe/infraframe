{
    "targets": [
        {
            "target_name": "videoAnalyzer-sw",
            "dependencies": ["gstreamer-codecs"],
            "sources": [
                "../addon.cc",
                "../VideoTranscoderWrapper.cc",
                "../VideoTranscoder.cpp",
                "../../../../core/infraframe/MediaFramePipeline.cpp",
                "../../../../core/infraframe/FrameConverter.cpp",
                "../../../../core/infraframe/FrameAnalyzer.cpp",
                "../../../../core/infraframe/FrameProcesser.cpp",
                "../../../../core/infraframe/FFmpegDrawText.cpp",
                "../../../../core/infraframe/I420BufferManager.cpp",
                "../../../../core/infraframe/GstreamerFrameDecoder.cpp",
                "../../../../core/infraframe/GstreamerFrameEncoder.cpp",
                "../../../../core/common/JobTimer.cpp",
            ],
            "cflags_cc": [
                "-Wall",
                "-O$(OPTIMIZATION_LEVEL)",
                "-g",
                "-std=c++17",
                "-DWEBRTC_POSIX",
                "-DBUILD_FOR_ANALYTICS",
            ],
            "cflags_cc!": [
                "-fno-exceptions",
            ],
            "include_dirs": [
                "<!(node -e \"require('nan')\")",
                "..",
                "$(CORE_HOME)/common",
                "$(CORE_HOME)/infraframe",
                "$(CORE_HOME)/../../third_party/webrtc/src",
                "$(CORE_HOME)/../../third_party/webrtc/src/third_party/libyuv/include",
                "$(DEFAULT_DEPENDENCY_PATH)/include",
                "$(CUSTOM_INCLUDE_PATH)",
            ],
            "libraries": [
                "-lboost_thread",
                "-llog4cxx",
                "-L$(CORE_HOME)/../../third_party/webrtc",
                "-lwebrtc",
                "<!@(pkg-config --libs libavutil)",
                "<!@(pkg-config --libs libavcodec)",
                "<!@(pkg-config --libs libavformat)",
                "<!@(pkg-config --libs libavfilter)",
                "-L$(DEFAULT_DEPENDENCY_PATH)/lib",
            ],
        },
        {
            "target_name": "gstreamer-codecs",
            "type": "static_library",
            "sources": [
                "../../../../core/owt_base/gstreamer/VideoEncoder.cpp",
                "../../../../core/owt_base/gstreamer/VideoDecoder.cpp",
                "../../../../core/owt_base/gstreamer/Support.cpp",
                "../../../../core/owt_base/gstreamer/Helpers.cpp",
                "../../../../core/owt_base/gstreamer/H265VideoEncoders.cpp",
                "../../../../core/owt_base/gstreamer/H265VideoDecoders.cpp",
                "../../../../core/owt_base/gstreamer/H264VideoEncoders.cpp",
                "../../../../core/owt_base/gstreamer/H264VideoDecoders.cpp",
                "../../../../core/owt_base/gstreamer/EncoderPipeline.cpp",
                "../../../../core/owt_base/gstreamer/DecoderPipeline.cpp",
                "../../../../core/owt_base/gstreamer/BufferPool.cpp",
            ],
            "cflags_cc": [
                "-Wall",
                "-O$(OPTIMIZATION_LEVEL)",
                "-g",
                "-std=c++17",
                "-DWEBRTC_POSIX",
                "<!@(pkg-config --cflags-only-I gstreamer-1.0)",
                "<!@(pkg-config --cflags-only-I glib-2.0)",
            ],
            "cflags_cc!": [
                "-fno-exceptions",
            ],
            "include_dirs": [
                "$(CORE_HOME)/../../third_party/webrtc_native_amd64_debug/include",
                "$(CORE_HOME)/../../third_party/webrtc_native_amd64_debug/include/libyuv",
            ],
            "libraries": [
                "-L$(CORE_HOME)/../../third_party/webrtc_native_amd64_debug/lib",
                "-lwebrtc",
                "-llibyuv_internal",
                "<!@(pkg-config --libs gstreamer-app-1.0)",
            ],
        },
    ]
}
