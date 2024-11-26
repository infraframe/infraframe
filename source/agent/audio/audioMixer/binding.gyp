{
    "targets": [
        {
            "target_name": "audioMixer",
            "sources": [
                "addon.cc",
                "AudioMixerWrapper.cc",
                "AudioMixer.cpp",
                "AcmDecoder.cpp",
                "FfDecoder.cpp",
                "AcmEncoder.cpp",
                "PcmEncoder.cpp",
                "FfEncoder.cpp",
                "AcmmFrameMixer.cpp",
                "AcmmBroadcastGroup.cpp",
                "AcmmGroup.cpp",
                "AcmmInput.cpp",
                "AcmmOutput.cpp",
                "AudioTime.cpp",
                "../../addons/common/NodeEventRegistry.cc",
                "../../../core/infraframe/MediaFramePipeline.cpp",
                "../../../core/infraframe/AudioUtilities.cpp",
                "../../../core/common/JobTimer.cpp",
            ],
            "cflags_cc": [
                "-Wall",
                "-O$(OPTIMIZATION_LEVEL)",
                "-g",
                "-std=c++17",
                "-DWEBRTC_POSIX",
            ],
            "cflags_cc!": [
                "-fno-exceptions",
            ],
            "include_dirs": [
                "<!(node -e \"require('nan')\")",
                "$(CORE_HOME)/common",
                "$(CORE_HOME)/infraframe",
                "$(CORE_HOME)/../../third_party/webrtc/src",
                "$(DEFAULT_DEPENDENCY_PATH)/include",
                "$(CUSTOM_INCLUDE_PATH)",
            ],
            "libraries": [
                "-L$(CORE_HOME)/../../third_party/webrtc",
                "-lwebrtc",
                "-lboost_thread",
                "-llog4cxx",
                "<!@(pkg-config --libs libavcodec)",
                "<!@(pkg-config --libs libavformat)",
                "<!@(pkg-config --libs libavutil)",
            ],
        }
    ]
}
