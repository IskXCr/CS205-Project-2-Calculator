cmake_minimum_required(VERSION 3.22)

# set the project name
project(Project_2_Calcultor)

aux_source_directory(./src DIR_SRCS)

include_directories(./include)

# add the executable
add_executable(calculator ${DIR_SRCS})