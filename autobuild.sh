#!/bin/bash

set -e

#如果当前目录下没有build目录，则创建该目录
if [ ! -d `pwd`/build ]; then 
    mkdir `pwd`/build
fi
    
rm -rf `pwd`/build/* #将之前编译文件删除

cd `pwd`/build &&
    cmake .. &&
    make

#回到项目根目录
cd ..

# 把头文件拷贝到 /usr/include/mymuduo so库拷贝到/usr/lib  PATH
if [ ! -d /usr/include/mymuduo ]; then    
    mkdir /usr/include/mymuduo
fi

for header in `ls *.h` #当前目录所有头文件拷贝到目录
do
    cp $header /usr/include/mymuduo
done

cp `pwd`/lib/libmymuduo.so /usr/lib

ldconfig #刷新动态库缓存

