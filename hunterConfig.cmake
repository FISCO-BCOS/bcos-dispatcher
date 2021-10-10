hunter_config(bcos-framework VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/afb584188a4ac1fb9a37e91367ff1399dff6c4c8.tar.gz
    SHA1 ed05e99b239a25e4b1a41cb4b9e7bd6f1ec9b641
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON #DEBUG=ON
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/039d884226ab6de9c5a59d09705ce86cd9b3b1d8.tar.gz
    SHA1 0ad746698643c643d9fbf84764e479e87ec41f45
    CMAKE_ARGS URL_BASE=${URL_BASE}
)