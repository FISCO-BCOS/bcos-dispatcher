hunter_config(bcos-framework VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/b12573238ef16d636f658cdc6ab46090dc396b7b.tar.gz
    SHA1 349711222381d56bfec688979eb0603035dbf8c8
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON #DEBUG=ON
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/34f6840626d20c6228c455fd16668f5fe40c6e75.tar.gz
    SHA1 d992f1a843ffdfadeb67ad8f5e46433bdfd11164
    CMAKE_ARGS URL_BASE=${URL_BASE}
)