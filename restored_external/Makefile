all: restored_external

restored_external:
	@rm -rf tools/;
	@echo Creating tools/ directory…;
	@echo Compiling…;
	@xcrun -sdk iphoneos clang --sysroot /Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk restored_external.c -arch arm64 -framework IOKit -framework CoreFoundation -Wall -o restored_external -miphoneos-version-min=6.0;
	@echo Signing with task_for_pid_0 property list…;
	@ldid -Srestored_external.plist ./restored_external;
	@echo Built hacked restored_external binary. Can be found in tools/.;