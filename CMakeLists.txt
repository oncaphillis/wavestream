cmake_minimum_required(VERSION 2.8)

project(NI)

add_executable("test_feedbackloop" "test_feedbackloop.cpp")

add_executable("wavefilter" wavefilter.cpp jobpool.cpp jobpool.h)
target_link_libraries("wavefilter" pthread)
