
# # 设置源文件的搜索路径
# set(SRC_LIST user_service.cc ../user.pb.cc)

# # 添加可执行文件
# add_executable(provider ${SRC_LIST})

# # 设置连接需要的库文件
# target_link_libraries(provider 
#     mprpc
#     protobuf
# )




# 设置源文件的搜索路径
set(SRC_LIST friend_service.cc ../friend.pb.cc)

# 添加可执行文件
add_executable(provider ${SRC_LIST})

# 设置连接需要的库文件
target_link_libraries(provider 
    mprpc
    protobuf
)

