# type either "make linux" or "make win32" to compile

help:
	@echo You can use this Makefile in the following ways:
	@echo make linux ............ Make Linux binaries
	@echo make win32 ............ Make Win32 binaries
	@echo make cleanlinux ....... Remove object files under Linux
	@echo make cleanwin32 ....... Remove object files under Win32

linux :
	make -C jamlib -f Makefile.linux
	make -f Makefile.linux

win32 :
	make -C jamlib -f Makefile.win32
	make -f Makefile.win32

cleanlinux :
	make -C jamlib -f Makefile.linux clean
	make -f Makefile.linux clean

cleanwin32 :
	make -C jamlib -f Makefile.win32 clean
	make -f Makefile.win32 clean
