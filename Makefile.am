ACLOCAL_AMFLAGS = -I m4

END=

bin_PROGRAMS = src/efs-ng

src_efs_ng_CXXFLAGS =       \
    $(FUSE_CFLAGS)          \
    -Wall -Wextra -std=c++11

src_efs_ng_LDFLAGS =    \
    $(FUSE_LIBS)        \
    -lboost_filesystem  \
    -lboost_system

# source files in alphabetical order
src_efs_ng_SOURCES =        \
    src/command-line.cpp    \
    src/command-line.h      \
    src/efs-ng.cpp          \
    $(END)


