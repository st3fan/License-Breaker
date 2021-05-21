LicenseBreaker: Makefile LicenseBreaker.cpp LicenseBreaker.h ElfUtils.cpp ElfUtils.h SetupView.cpp SetupView.h _APP_
	gcc -o LicenseBreaker -g LicenseBreaker.cpp ElfUtils.cpp SetupView.cpp -lbe -lscreensaver _APP_ -ludis86

_APP_:
	ln -s /system/preferences/ScreenSaver _APP_

test: LicenseBreaker
	cp LicenseBreaker ~/config/non-packaged/add-ons/Screen\ Savers/
	ScreenSaver
