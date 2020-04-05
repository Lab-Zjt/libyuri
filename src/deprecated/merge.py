#! /usr/bin/python3
import os
import io

file_list = ["reflect.h", "serializer.h", "deserializer.h"]

output = open("../yuri.h", "w")
output.write("#ifndef LIBYURI_YURI_H_\n")
output.write("#define LIBYURI_YURI_H_\n")
for file in file_list:
  f = open(file,"r")
  while True:
    s = f.readline()
    if len(s) == 0:
      break
    if s.startswith("#ifndef LIB") or s.startswith("#define LIB") or s.startswith('#include "reflect.h"') or s.startswith("#endif"):
      continue
    output.write(s)
  f.close()
output.write("#endif\n")
output.close()

