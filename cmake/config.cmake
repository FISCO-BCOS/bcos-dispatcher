hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/84cc2e3783b4295bc393b6fbbeb9f32f8260964e.tar.gz
    SHA1 a1e69126d0168ccec73ed97c60db0923c805e670
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/502c17b20558f4fd3809135622ca68c71b8ff16d.tar.gz
    SHA1 7f94aa82374b3dbbeaa10af056fcea14bd72720a
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON URL_BASE=${URL_BASE}
)
