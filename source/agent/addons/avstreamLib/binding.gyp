{
    "targets": [
        {
            "target_name": "avstream",
            "sources": [
                "addon.cc",
                "AVStreamInWrap.cc",
                "AVStreamOutWrap.cc",
                "../../addons/common/NodeEventRegistry.cc",
                "../../../core/infraframe/MediaFramePipeline.cpp",
                "../../../core/infraframe/AVStreamOut.cpp",
                "../../../core/infraframe/MediaFileOut.cpp",
                "../../../core/infraframe/LiveStreamOut.cpp",
                "../../../core/infraframe/LiveStreamIn.cpp",
            ],
            "include_dirs": [
                "<!(node -e \"require('nan')\")",
                "$(CORE_HOME)/common",
                "$(CORE_HOME)/infraframe",
                "$(DEFAULT_DEPENDENCY_PATH)/include",
                "$(CUSTOM_INCLUDE_PATH)",
            ],
            "libraries": [
                "-lboost_thread",
                "-llog4cxx",
                "<!@(pkg-config --libs libavformat)",
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
