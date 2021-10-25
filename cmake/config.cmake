hunter_config(bcos-framework VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/f4a7cdcf5e42cc2a11acfa3a2c8530a2728b87bb.tar.gz
    SHA1 7ea057e411fdf07e6210053157ce471b7489b3cf
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON #DEBUG=ON
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/6071fab4f0248196acdb6580d8ae15a63d225e65.tar.gz
    SHA1 794c1adc1fb64b994af92ce7c804149b68dde658
    CMAKE_ARGS URL_BASE=${URL_BASE}
)