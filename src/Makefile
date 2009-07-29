# type either "make linux", "make win32" or "make os2" to compile

MAKE = make

help:
	@echo You can use this Makefile in the following ways:
	@echo make linux ............ Make Linux binaries
	@echo make win32 ............ Make Win32 binaries
	@echo make os2   ............ Make OS/2 binaries (EMX)
	@echo make cleanlinux ....... Remove object files under Linux
	@echo make cleanwin32 ....... Remove object files under Win32
	@echo make cleanos2 ......... Remove object files under OS/2 (EMX)

linux :
	$(MAKE) -C jamlib -f Makefile.linux
	$(MAKE) -f Makefile.linux

win32 :
	$(MAKE) -C jamlib -f Makefile.win32
	$(MAKE) -f Makefile.win32

os2 :
	$(MAKE) -C jamlib -f Makefile.os2
	$(MAKE) -f Makefile.os2

cleanlinux :
	$(MAKE) -C jamlib -f Makefile.linux clean
	$(MAKE) -f Makefile.linux clean

cleanwin32 :
	$(MAKE) -C jamlib -f Makefile.win32 clean
	$(MAKE) -f Makefile.win32 clean

cleanos2 :
	$(MAKE) -C jamlib -f Makefile.os2 clean
	$(MAKE) -f Makefile.os2 clean

