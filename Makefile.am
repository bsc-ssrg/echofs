ACLOCAL_AMFLAGS = -I m4

bin_PROGRAMS = src/efs-ng

src_efs_ng_CXXFLAGS =       \
    $(FUSE_CFLAGS)          \
    -Wall -Wextra -std=c++11

src_efs_ng_LDFLAGS =    \
    $(FUSE_LIBS)

src_efs_ng_SOURCES =    \
    src/efs-ng.cpp
