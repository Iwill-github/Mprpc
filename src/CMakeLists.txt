# 设置编译所需的源文件，该方法当添加新源文件时，由于Makefile文件未更新，容易出现连接错误
# aux_source_directory(. SRC_LIST)
set(SRC_LIST 
    mprpc_application.cc 
    mprpc_config.cc 
    rpc_header.pb.cc 
    mprpc_provider.cc
    mprpc_channel.cc
    logger.cc
    zookeeper_util.cc
)

aux_source_directory(./dualbuf_logger DUALBUF_LOGGER)

# 将源代码编译为动态库文件 mprpc
# add_library(mprpc SHARED ${SRC_LIST})     # 因为 muduo_base muduo_net 是静态库。需要保证该静态库以以PIC方式编译（即，可以在）
add_library(mprpc ${SRC_LIST} ${DUALBUF_LOGGER})

# 设置mprpc框架的 依赖库（这些是静态库）
target_link_libraries(mprpc 
    muduo_net 
    muduo_base 
    pthread
    zookeeper_mt    
)   # muduo_net 是muduo网络库的核心，其他均依赖muduo_net


