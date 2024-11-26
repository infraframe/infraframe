{
    "targets": [
        {
            "target_name": "internalIO",
            "sources": [
                "addon.cc",
                "InternalServerWrapper.cc",
                "InternalClientWrapper.cc",
                "InternalConfig.cc",
                "../../../core/infraframe/MediaFramePipeline.cpp",
                "../../../core/infraframe/internal/TransportServer.cpp",
                "../../../core/infraframe/internal/TransportClient.cpp",
                "../../../core/infraframe/internal/TransportBase.cpp",
                "../../../core/infraframe/internal/InternalServer.cpp",
                "../../../core/infraframe/internal/InternalClient.cpp",
                "../../../core/common/IOService.cpp",
            ],
            "include_dirs": [
                "<!(node -e \"require('nan')\")",
                "$(CORE_HOME)/common",
                "$(CORE_HOME)/infraframe",
                "$(CORE_HOME)/infraframe/internal",
                "$(DEFAULT_DEPENDENCY_PATH)/include",
                "$(CUSTOM_INCLUDE_PATH)",
            ],
            "libraries": [
                "-lboost_system",
                "-lboost_thread",
                "-llog4cxx",
                "-L$(DEFAULT_DEPENDENCY_PATH)/lib",
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
                            "-std=c++17",
                        ],
                        "cflags_cc!": ["-fno-exceptions"],
                    },
                ],
            ],
        }
    ]
}
