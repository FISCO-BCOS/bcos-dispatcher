hunter_config(bcos-framework VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/3ddf34a77c83109d92636d41dbf13231874ad7cd.tar.gz
    SHA1 dba9faa293b7fc8ef5187c549915bef8ecf69c10
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON #DEBUG=ON
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/9bdc2a0fbe8e3d619bc09944e475b3757e833161.tar.gz
    SHA1 570f0e2ff924ff6a1c4bf158f5e859d8aa91d9d3
    CMAKE_ARGS URL_BASE=${URL_BASE}
)