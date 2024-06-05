
# # 设置源文件的搜索路径
# set(SRC_LIST call_user_service.cc ../user.pb.cc)

# # 添加可执行文件
# add_executable(consumer ${SRC_LIST})

# # 设置连接需要的库文件
# target_link_libraries(consumer 
#     mprpc
#     protobuf
# )



# 设置源文件的搜索路径
set(SRC_LIST call_friend_service.cc ../friend.pb.cc)

# 添加可执行文件
add_executable(consumer ${SRC_LIST})

# 设置连接需要的库文件
target_link_libraries(consumer 
    mprpc
    protobuf
)

