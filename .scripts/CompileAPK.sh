#!/bin/sh
# ______________________________________________________________________________
#
#  Compile raylib project for Android
# ______________________________________________________________________________

# stop on error and display each command as it gets executed. Optional step but helpful in catching where errors happen if they do.
set -xe

# NOTE: If you excluded any ABIs in the previous steps, remove them from this list too
ABIS="arm64-v8a armeabi-v7a x86 x86_64"

#needs manual per version changing for now until that can be figured out
BUILD_TOOLS=platform/android/sdk/build-tools/29.0.3
TOOLCHAIN=platform/android/ndk/toolchains/llvm/prebuilt/linux-x86_64
NATIVE_APP_GLUE=platform/android/ndk/sources/android/native_app_glue

#some of these flags are auto applied by the clang macro, while others don't
#some of these can be removed however its not recommended
FLAGS="-ffunction-sections -funwind-tables -fstack-protector-strong -fPIC -Wall \
	-Wformat -Werror=format-security -no-canonical-prefixes \
	-DANDROID -DPLATFORM_ANDROID -D__ANDROID_API__=34"

INCLUDES="-I. -Iinclude -Iplatform/android/include -I$NATIVE_APP_GLUE -I$TOOLCHAIN/sysroot/usr/include"

# Copy icons
#you know its probably possible to completely axe this, and do it directly in the prep stage, but i can't bother right now
	mkdir --parents platform/android/build/res/drawable-ldpi
cp platform/android/assets/icon_ldpi.png platform/android/build/res/drawable-ldpi/icon.png
	mkdir --parents platform/android/build/res/drawable-mdpi
cp platform/android/assets/icon_mdpi.png platform/android/build/res/drawable-mdpi/icon.png
	mkdir --parents platform/android/build/res/drawable-hdpi
cp platform/android/assets/icon_hdpi.png platform/android/build/res/drawable-hdpi/icon.png
	mkdir --parents platform/android/build/res/drawable-xhdpi
cp platform/android/assets/icon_xhdpi.png platform/android/build/res/drawable-xhdpi/icon.png

# Copy other assets, including the icons again?
	mkdir --parents platform/android/build/assets
cp platform/android/assets/* platform/android/build/assets

# ______________________________________________________________________________
#
#  Compile
# ______________________________________________________________________________
#
for ABI in $ABIS; do
	case "$ABI" in
		"armeabi-v7a")
			CCTYPE="armv7a-linux-androideabi"
			ARCH="arm"
			LIBPATH="arm-linux-androideabi"
			ABI_FLAGS="-std=c99 -march=armv7-a -mfloat-abi=softfp -mfpu=vfpv3-d16"
			;;

		"arm64-v8a")
			CCTYPE="aarch64-linux-android"
			ARCH="aarch64"
			LIBPATH="aarch64-linux-android"
			ABI_FLAGS="-std=c99 -mfix-cortex-a53-835769"
			;;

		"x86")#are there even any x86 and x64 android devices in circulation?
			CCTYPE="i686-linux-android"
			ARCH="i386"
			LIBPATH="i686-linux-android"
			ABI_FLAGS=""
			;;

		"x86_64")
			CCTYPE="x86_64-linux-android"
			ARCH="x86_64"
			LIBPATH="x86_64-linux-android"
			ABI_FLAGS=""
			;;
	esac
	CC="$TOOLCHAIN/bin/${CCTYPE}34-clang"

	# Compile native app glue
	# .c -> .o
	$CC -c $NATIVE_APP_GLUE/android_native_app_glue.c -o $NATIVE_APP_GLUE/native_app_glue.o \
		$INCLUDES -I$TOOLCHAIN/sysroot/usr/include/$CCTYPE $FLAGS $ABI_FLAGS
	# .o -> .a
	$TOOLCHAIN/bin/llvm-ar rcs platform/android/lib/$ABI/libnative_app_glue.a $NATIVE_APP_GLUE/native_app_glue.o


	# Compile project
	for file in src/*.c; do
		$CC -c $file -o "$file".o \
			$INCLUDES -I$TOOLCHAIN/sysroot/usr/include/$CCTYPE $FLAGS $ABI_FLAGS
	done

	mkdir --parents platform/android/build/lib/armeabi-v7a
	mkdir --parents platform/android/build/lib/arm64-v8a
	mkdir --parents platform/android/build/lib/x86
	mkdir --parents platform/android/build/lib/x86_64

		#at some point in one of these ndk updates they removed libc from the
        # Link the project with toolchain specific linker to avoid relocations issue.
        #
	$TOOLCHAIN/bin/ld.lld src/*.o -o platform/android/build/lib/$ABI/libmain.so -shared \
		--exclude-libs libatomic.a --build-id \
		-z noexecstack -z relro -z now \
		--warn-shared-textrel --fatal-warnings -u ANativeActivity_onCreate \
		-L$TOOLCHAIN/sysroot/usr/lib/$LIBPATH/34 \
		-L$TOOLCHAIN/lib/clang/21/lib/linux/$ARCH \
		-L. -Landroid/build/obj -Llib/$ABI \
		-llog -landroid -lEGL -lGLESv2 -lOpenSLES -lc -lm -ldl platform/android/lib/$ABI/libnative_app_glue.a platform/android/lib/$ABI/libraylib.a
		# -lraylib -lnative_app_glue -llog -landroid -lEGL -lGLESv2 -lOpenSLES -latomic -lc -lm -ldl
		#calls libatomic despite it being listed in --exclude-libs, removing causes it the program to compile, why the fuck
done

# ______________________________________________________________________________
#
#  Build APK
# ______________________________________________________________________________
#
#prepare final directories
mkdir --parents platform/android/build/dex

$BUILD_TOOLS/aapt package -f -m \
	-S platform/android/build/res -J platform/android/build/src -M platform/android/build/AndroidManifest.xml \
	-I platform/android/sdk/platforms/android-29/android.jar

# Compile NativeLoader.java
javac -verbose -source 1.8 -target 1.8 -d platform/android/build/obj \
	-bootclasspath jre/lib/rt.jar \
	-classpath platform/android/sdk/platforms/android-29/android.jar:android/build/obj \
	-sourcepath src platform/android/build/src/com/raylib/game/R.java \
	platform/android/build/src/com/raylib/game/NativeLoader.java

$BUILD_TOOLS/dx --verbose --dex --output=platform/android/build/dex/classes.dex platform/android/build/obj

# Add resources and assets to APK
$BUILD_TOOLS/aapt package -f \
	-M platform/android/build/AndroidManifest.xml -S platform/android/build/res -A platform/android/assets \
	-I platform/android/sdk/platforms/android-29/android.jar -F game.apk platform/android/build/dex

# Add libraries to APK
cd platform/android/build
for ABI in $ABIS; do
	../../../$BUILD_TOOLS/aapt add ../../../game.apk lib/$ABI/libmain.so
done
cd ../../..

# Zipalign APK and sign
# NOTE: If you changed the storepass and keypass in the setup process, change them here too
$BUILD_TOOLS/zipalign -f 4 game.apk game.final.apk
mv -f game.final.apk game.apk

# Install apksigner with `sudo apt install apksigner`
$BUILD_TOOLS/apksigner sign  --ks platform/android/raylib.keystore --out my-app-release.apk --ks-pass pass:raylib game.apk
mv my-app-release.apk build/game.apk

#done somewhere else
# # Install to device or emulator
# platform/android/sdk/platform-tools/adb install -r game.apk
