{
    "targets": [
        {
            "target_name": "internalIO",
            "sources": [
                "addon.cc",
                "InternalInWrapper.cc",
                "InternalOutWrapper.cc",
                "InternalIOWrapper.cc",
                "InternalConfig.cc",
                "../../../core/infraframe/InternalIn.cpp",
                "../../../core/infraframe/InternalOut.cpp",
                "../../../core/infraframe/InternalSctp.cpp",
                "../../../core/infraframe/MediaFramePipeline.cpp",
                "../../../core/infraframe/RawTransport.cpp",
                "../../../core/infraframe/SctpTransport.cpp",
                "../../../core/common/IOService.cpp",
            ],
            "include_dirs": [
                "$(CORE_HOME)/common",
                "$(CORE_HOME)/infraframe",
                "$(DEFAULT_DEPENDENCY_PATH)/include",
                "$(CUSTOM_INCLUDE_PATH)",
            ],
            "libraries": [
                "-lboost_system",
                "-lboost_thread",
                "-llog4cxx",
                "-L$(DEFAULT_DEPENDENCY_PATH)/lib",
                "-lusrsctp",
            ],
            # 'INET', 'INET6' flags must be added for usrsctp lib, otherwise the arguments of receive callback would shift
            "cflags_cc": ["-DINET", "-DINET6"],
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
        },
        # not build test target
        #  {
        #    'target_name': 'SctpTest',
        #    'type' : 'executable',
        #    'sources': [
        #      '../../../core/infraframe/RawTransport.cpp',
        #      '../../../core/infraframe/SctpTransport.cpp',
        #      '../../../core/infraframe/SctpTransportTest.cpp',
        #    ],
        #    'include_dirs': [
        #      '$(CORE_HOME)/common',
        #      '$(CORE_HOME)/infraframe',
        #      '$(CORE_HOME)/../../third_party/usrsctp/usrsctplib'
        #    ],
        #    'libraries': [
        #      '-lboost_system',
        #      '-lboost_thread',
        #      '-llog4cxx',
        #      '-L$(CORE_HOME)/../../third_party/usrsctp/usrsctplib/.libs', '-lusrsctp',
        #    ],
        #    'conditions': [
        #      [ 'OS=="mac"', {
        #        'xcode_settings': {
        #          'GCC_ENABLE_CPP_EXCEPTIONS': 'YES',        # -fno-exceptions
        #          'MACOSX_DEPLOYMENT_TARGET':  '10.7',       # from MAC OS 10.7
        #          'OTHER_CFLAGS': ['-g -O$(OPTIMIZATION_LEVEL) -stdlib=libc++']
        #        },
        #      }, { # OS!="mac"
        #        'cflags!':    ['-fno-exceptions'],
        #        'cflags_cc':  ['-Wall', '-O$(OPTIMIZATION_LEVEL)', '-g', '-std=c++17', '-DINET', '-DINET6'],
        #        'cflags_cc!': ['-fno-exceptions']
        #      }],
        #    ]
        #  }
    ]
}
