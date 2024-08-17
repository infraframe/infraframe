{
    "targets": [
        {
            "target_name": "audioRanker",
            "sources": [
                "addon.cc",
                "AudioRankerWrapper.cc",
                "../../../core/infraframe/selector/AudioRanker.cpp",
                "../../../core/infraframe/MediaFramePipeline.cpp",
                "../../../core/common/IOService.cpp",
            ],
            "include_dirs": [
                "<!(node -e \"require('nan')\")",
                "../common",
                "../../../core/common/",
                "../../../core/infraframe/",
                "../../../core/infraframe/selector/",
            ],
            "libraries": [
                "-lboost_thread",
                "-llog4cxx",
            ],
            "conditions": [
                [
                    'OS=="mac"',
                    {
                        "xcode_settings": {
                            "GCC_ENABLE_CPP_EXCEPTIONS": "YES",  # -fno-exceptions
                            "MACOSX_DEPLOYMENT_TARGET": "10.7",  # from MAC OS 10.7
                            "OTHER_CFLAGS": [
                                "-g -O$(OPTIMIZATION_LEVEL) -stdlib=libc++"
                            ],
                        },
                    },
                    {  # OS!="mac"
                        "cflags!": ["-fno-exceptions"],
                        "cflags_cc": [
                            "-Wall",
                            "-O$(OPTIMIZATION_LEVEL)",
                            "-g",
                            "-std=c++11",
                        ],
                        "cflags_cc!": ["-fno-exceptions"],
                    },
                ],
            ],
        }
    ]
}
