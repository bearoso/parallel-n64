LOCAL_PATH := $(call my-dir)
PERFTEST = 0
HAVE_HWFBE = 0
HAVE_SHARED_CONTEXT=0
SINGLE_THREAD=0
GLIDE2GL=1
GLIDE64MK2=0
GLES = 1

include $(CLEAR_VARS)

LOCAL_MODULE := retro

ROOT_DIR := ../..
LIBRETRO_DIR = ..

ifeq ($(TARGET_ARCH),arm)
LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -marm
WITH_DYNAREC := arm

COMMON_FLAGS := -DANDROID_ARM -DDYNAREC -DNEW_DYNAREC=3 -DNO_ASM -DNOSSE
WITH_DYNAREC := arm

ifeq ($(TARGET_ARCH_ABI), armeabi-v7a)
LOCAL_ARM_NEON := true
HAVE_NEON := 1
endif

endif

ifeq ($(TARGET_ARCH),x86)
COMMON_FLAGS := -DANDROID_X86 -DDYNAREC -D__SSE2__ -D__SSE__ -D__SOFTFP__
WITH_DYNAREC := x86
endif

ifeq ($(TARGET_ARCH),mips)
COMMON_FLAGS := -DANDROID_MIPS
endif

ifeq ($(NDK_TOOLCHAIN_VERSION), 4.6)
COMMON_FLAGS += -DANDROID_OLD_GCC
endif

SOURCES_C   :=
SOURCES_CXX :=
SOURCES_ASM :=
INCFLAGS    :=

include $(ROOT_DIR)/Makefile.common

LOCAL_SRC_FILES := $(SOURCES_CXX) $(SOURCES_C) $(SOURCES_ASM)

# Video Plugins

ifeq ($(HAVE_HWFBE), 1)
COMMON_FLAGS += -DHAVE_HWFBE
endif

ifeq ($(HAVE_SHARED_CONTEXT), 1)
COMMON_FLAGS += -DHAVE_SHARED_CONTEXT
endif

ifeq ($(SINGLE_THREAD), 1)
COMMON_FLAGS += -DSINGLE_THREAD
endif

PLATFORM_EXT := unix

ifeq ($(GLIDE64MK2),1)
COMMON_FLAGS += -DGLIDE64_MK2
endif

COMMON_FLAGS += -DM64P_CORE_PROTOTYPES -D_ENDUSER_RELEASE -DM64P_PLUGIN_API -D__LIBRETRO__ -DINLINE="inline" -DSDL_VIDEO_OPENGL_ES2=1 -DANDROID -DSINC_LOWER_QUALITY -DHAVE_LOGGER -DHAVE_COMBINE_EXT -fexceptions $(GLFLAGS) -DGLES
COMMON_OPTFLAGS = -O3 -ffast-math

LOCAL_CFLAGS += $(COMMON_OPTFLAGS) $(COMMON_FLAGS)
LOCAL_CXXFLAGS += $(COMMON_OPTFLAGS) $(COMMON_FLAGS)
LOCAL_LDLIBS += -llog -lGLESv2
LOCAL_C_INCLUDES = $(CORE_DIR)/src $(CORE_DIR)/src/api $(VIDEODIR_GLIDE)/Glitch64/inc $(LIBRETRO_DIR)/libco $(LIBRETRO_DIR)

ifeq ($(PERFTEST), 1)
LOCAL_CFLAGS += -DPERF_TEST
LOCAL_CXXFLAGS += -DPERF_TEST
endif

include $(BUILD_SHARED_LIBRARY)

