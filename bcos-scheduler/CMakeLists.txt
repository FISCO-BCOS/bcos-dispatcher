file(GLOB SRC_LIST "*.cpp")
file(GLOB HEADERS "*.h")

#add_library(${BCOS_DISPATCHER_TARGET} ${HEADERS} ${SRC_LIST})
add_library(${BCOS_DISPATCHER_TARGET} SchedulerImpl.cpp ExecutorManager.cpp)
target_link_libraries(${BCOS_DISPATCHER_TARGET} bcos-framework::utilities)