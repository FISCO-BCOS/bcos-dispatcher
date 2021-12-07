hunter_config(bcos-framework VERSION 3.0.0-local
	URL https://${URL_BASE}/FISCO-BCOS/bcos-framework/archive/1003d35925569d25c84483aef4402556e6c74f18.tar.gz
    SHA1 502eb233c2ce6f7c0e7e70b8bf64e50d25545c31
	CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON
)

hunter_config(bcos-tars-protocol
    VERSION 3.0.0-local
    URL https://${URL_BASE}/FISCO-BCOS/bcos-tars-protocol/archive/78d25fca2cc23eb0e082a1d381ebece6f6bff6fc.tar.gz
    SHA1 f2ce87c145eabd792abc8190a8f26e9424f3d517
    CMAKE_ARGS HUNTER_PACKAGE_LOG_BUILD=ON HUNTER_PACKAGE_LOG_INSTALL=ON URL_BASE=${URL_BASE}
)
