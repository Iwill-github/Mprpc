#!/bin/bash

set -e          # 设置脚本在遇到任何命令失败时立即退出

find `pwd`/build -type f ! -name "readme.txt" -delete
find `pwd`/build -type d -empty -delete

cd `pwd`/build &&
    cmake .. &&
    make
cd ..

cp -r `pwd`/include `pwd`/lib

