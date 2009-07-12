# type either "make linux" or "make win32" to compile

MAKE = make

help:
	@echo You can use this Makefile in the following ways:
	@echo make linux ............ Make Linux binaries
	@echo make win32 ............ Make Win32 binaries
	@echo make cleanlinux ....... Remove object files under Linux
	@echo make cleanwin32 ....... Remove object files under Win32

linux :
	$(MAKE) -C jamlib -f Makefile.linux
	$(MAKE) -f Makefile.linux

win32 :
	$(MAKE) -C jamlib -f Makefile.win32
	$(MAKE) -f Makefile.win32

cleanlinux :
	$(MAKE) -C jamlib -f Makefile.linux clean
	$(MAKE) -f Makefile.linux clean

cleanwin32 :
	$(MAKE) -C jamlib -f Makefile.win32 clean
	$(MAKE) -f Makefile.win32 clean
