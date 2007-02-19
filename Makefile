VERSION   = 1.50BETA
DEFINES   = -DMC_VERSION=\"$(VERSION)\" -DIS_BETA=1 -DDO_TRACING

INC_PATH  = ..\Winamp SDK 5.02
LIB_PATH  = ..\Winamp SDK 5.02\Build
CPP       = g++
CC_OPTS   = -DWIN32 -D_WIN32_IE=0x0500 -D_WINDOWS $(DEFINES) -I"$(INC_PATH)" -O0 -g -Wall -c -fmessage-length=0
LIBS      = -L"$(LIB_PATH)" -lwinamp -lwsock32 -lgdi32
LD_OPTS   = $(LIBS) -shared
NSIS      = makensis /V1 /DMC_VERSION=$(VERSION)
BUILD_DIR = Build

OBJS :=\
	$(BUILD_DIR)/CasterLibrary.o\
	$(BUILD_DIR)/Configuration.o\
	$(BUILD_DIR)/date.o\
	$(BUILD_DIR)/DisplayList.o\
	$(BUILD_DIR)/Http.o\
	$(BUILD_DIR)/MediaCaster.o\
	$(BUILD_DIR)/PlayLists.o\
	$(BUILD_DIR)/Process.o\
	$(BUILD_DIR)/Search.o\
	$(BUILD_DIR)/Song.o\
	$(BUILD_DIR)/SongList.o\
	$(BUILD_DIR)/TimerThread.o\
	$(BUILD_DIR)/Trace.o\
	$(BUILD_DIR)/Upgrade.o\
	$(BUILD_DIR)/default.o


all: $(BUILD_DIR)/ml_mcaster.dll


installer: $(BUILD_DIR)/MediaCaster.exe


$(BUILD_DIR):
	mkdir $(BUILD_DIR)
	
	
$(BUILD_DIR)/MediaCaster.exe: $(BUILD_DIR)/ml_mcaster.dll MediaCaster.nsi
	$(NSIS) MediaCaster.nsi


$(BUILD_DIR)/ml_mcaster.dll: $(BUILD_DIR) $(OBJS)
	@echo 'Creating DLL: $@'
	$(CPP) $(OBJS) $(LD_OPTS) -o$@
	@echo ' '

clean:
	rm -f $(OBJS) $(BUILD_DIR)/ml_mcaster.dll $(BUILD_DIR)/MediaCaster.exe


$(BUILD_DIR)/default.o: default.rc
	@echo 'Invoking: windres'
	windres -o$@ $<
	@echo ' '
	
	
$(BUILD_DIR)/%.o: %.cpp
	@echo 'Building file: $<'
	$(CPP) $(CC_OPTS) -o$@ $<
	@echo ' '