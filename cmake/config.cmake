hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/2f160cca08dafffc06c1b5e24121c54b6ffced69.tar.gz
    SHA1 46641348cbfdc3ad03619400d2d489eaece3f7f7
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/502c17b20558f4fd3809135622ca68c71b8ff16d.tar.gz
    SHA1 7f94aa82374b3dbbeaa10af056fcea14bd72720a
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON URL_BASE=${URL_BASE}
)
