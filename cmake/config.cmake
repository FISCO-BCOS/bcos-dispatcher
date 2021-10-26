hunter_config(bcos-framework VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/a4229cec8a1b7a501a2f4e6b0339784b334322e7.tar.gz
    SHA1 b6860e54d305075e790e450ffd66b2a57e29382c
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON #DEBUG=ON
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/273bae4bc82825d19481f37391ffda8149f4634b.tar.gz
    SHA1 b76976f9f5e697db577efdd0c6d33deb9c74bc9d
    CMAKE_ARGS URL_BASE=${URL_BASE}
)