# really just some handy scripts...

KEXT=GenericUSBXHCI.kext
DIST=RehabMan-Generic-USB3

ifeq ($(findstring 32,$(BITS)),32)
OPTIONS:=$(OPTIONS) -arch i386
endif

ifeq ($(findstring 64,$(BITS)),64)
OPTIONS:=$(OPTIONS) -arch x86_64
endif

.PHONY: all
all:
	xcodebuild build $(OPTIONS) -configuration Release
	xcodebuild build $(OPTIONS) -configuration Debug
	make -f xhcdump.mak

.PHONY: clean
clean:
	xcodebuild clean $(OPTIONS) -configuration Release
	xcodebuild clean $(OPTIONS) -configuration Debug
	rm ./xhcdump

.PHONY: update_kernelcache
update_kernelcache:
	sudo touch /System/Library/Extensions
	sudo kextcache -update-volume /

.PHONY: install_debug
install_debug:
	sudo cp -R ./Build/Debug/$(KEXT) /System/Library/Extensions
	make update_kernelcache

.PHONY: install
install:
	sudo cp -R ./Build/Release/$(KEXT) /System/Library/Extensions
	make update_kernelcache

.PHONY: distribute
distribute:
	if [ -e ./Distribute ]; then rm -r ./Distribute; fi
	mkdir ./Distribute
	#cp -R ./Build/Debug ./Distribute
	cp -R ./Build/Release ./Distribute
	cp ./xhcdump ./Distribute
	find ./Distribute -path *.DS_Store -delete
	find ./Distribute -path *.dSYM -exec echo rm -r {} \; >/tmp/org.voodoo.rm.dsym.sh
	chmod +x /tmp/org.voodoo.rm.dsym.sh
	/tmp/org.voodoo.rm.dsym.sh
	rm /tmp/org.voodoo.rm.dsym.sh
	ditto -c -k --sequesterRsrc --zlibCompressionLevel 9 ./Distribute ./Archive.zip
	mv ./Archive.zip ./Distribute/`date +$(DIST)-%Y-%m%d.zip`
