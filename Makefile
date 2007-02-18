SDK_PATH = ..\Winamp SDK 5.02
DEFINES  = -DMC_VERSION=\"1.50BETA\" -DIS_BETA=1 -DDO_TRACING 

CPP      = g++
CC_OPTS  = -DWIN32 -D_WIN32_IE=0x0500 -D_WINDOWS $(DEFINES) -I"$(SDK_PATH)" -O0 -g -Wall -c -fmessage-length=0
LIBS     = -L"$(SDK_PATH)" -lwinamp -lwsock32 -lgdi32
LD_OPTS  = $(LIBS) -shared

OBJS :=\
	CasterLibrary.o\
	Configuration.o\
	date.o\
	DisplayList.o\
	Http.o\
	MediaCaster.o\
	PlayLists.o\
	Process.o\
	Search.o\
	Song.o\
	SongList.o\
	TimerThread.o\
	Trace.o\
	Upgrade.o\
	default.o


all: ml_mcaster.dll

ml_mcaster.dll: $(OBJS)
	@echo 'Creating DLL: $@'
	$(CPP) $(OBJS) $(LD_OPTS) -o$@
	@echo ' '

clean:
	rm -f $(OBJS) ml_mcaster.dll

default.o: default.rc
	@echo 'Invoking: windres'
	windres -o$@ $<
	@echo ' '
	
%.o: %.cpp
	@echo 'Building file: $<'
	$(CPP) $(CC_OPTS) -o$@ $<
	@echo ' '