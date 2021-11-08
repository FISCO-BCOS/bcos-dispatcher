hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/e2f75352fd4333c1dc7f300b475c8fdf2a8c69b0.tar.gz
    SHA1 84ab1370baddea9c14fb12a82c79c73074f305f4
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/325b1c6971e3f42f527996cdf990b722a5eb4d50.tar.gz
    SHA1 06b904725472b1063c6cfc34894cd1979a22ad8a
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON URL_BASE=${URL_BASE}
)

