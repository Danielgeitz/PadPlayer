TARGET = Main

CPP_SOURCES = Main.cpp \
              Display.cpp \
              SdHandler.cpp \
              Setlist.cpp \
              AudioPlayer.cpp \
              PadBrowser.cpp

USE_FATFS = 1
OPT = -Os

LIBDAISY_DIR = ../../libDaisy/
DAISYSP_DIR  = ../../DaisySP/

SYSTEM_FILES_DIR = $(LIBDAISY_DIR)/core
include $(SYSTEM_FILES_DIR)/Makefile