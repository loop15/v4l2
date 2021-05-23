#!/bin/bash
make clean
echo "清除编译垃圾"
dos2unix *
echo "转换linux格式"

astyle --options=.astylerc *.c
echo "格式化"

if [ -f img/* ]; then
  rm img/* -v
fi
echo "删除img文件夹里内容"
