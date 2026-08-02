echo "Android is currently fairly buggy from all my attempts of compiling so here's the setup i found worked best"

#set up directories
mkdir --parents platform/android/assets platform/android/lib/armeabi-v7a platform/android/lib/arm64-v8a platform/android/lib/x86 platform/android/lib/x86_64 platform/android/include
mkdir --parents platform/android/build
#get the ndk
cd platform/android
if [ ! -d "ndk" ]; then
    [[ -e android-ndk.zip ]] || wget https://dl.google.com/android/repository/android-ndk-r29-linux.zip -O android-ndk.zip
    unzip android-ndk
    rename 'android-ndk-r29' ndk android-ndk-r29
fi
#still at the android folder, get the latest sdk. there's no easy way to get the latest SDK,
#because there's no just getting the latest, you have to get it manually, thanks google. so the numbers are completely random instead of there just being a number denoting a latest version
if [ ! -d "sdk" ]; then
    mkdir sdk
    cd sdk
    [[ -e android-sdk.zip ]] || wget https://dl.google.com/android/repository/commandlinetools-linux-13114758_latest.zip -O android-sdk.zip
    unzip android-sdk
    cd cmdline-tools/bin
    ./sdkmanager --update --sdk_root=../..
    yes | ./sdkmanager --install "build-tools;29.0.3" --sdk_root=../.. #pesky license agreements
    ./sdkmanager --install "platform-tools" --sdk_root=../..
    ./sdkmanager --install "platforms;android-29" --sdk_root=../..
    cd ../../..
fi
#cd back to main directory and into raylib
cd ../../raylib/src
cp raylib.h ../../platform/android/include/raylib.h
make clean
    make PLATFORM=PLATFORM_ANDROID ANDROID_NDK=../../platform/android/ndk ANDROID_ARCH=arm ANDROID_API_VERSION=34
        mv libraylib.a ../../platform/android/lib/armeabi-v7a
#this one errors in a very strange way compared to the rest, its like one if its include paths gets glitched into usr/include halfway through
make clean
    make PLATFORM=PLATFORM_ANDROID TARGET_PLATFORM=PLATFORM_ANDROID ANDROID_NDK=../../platform/android/ndk ANDROID_ARCH=arm64 ANDROID_API_VERSION=34
        mv libraylib.a ../../platform/android/lib/arm64-v8a
make clean
    make PLATFORM=PLATFORM_ANDROID ANDROID_NDK=../../platform/android/ndk ANDROID_ARCH=x86 ANDROID_API_VERSION=34
        mv libraylib.a ../../platform/android/lib/x86
make clean
    make PLATFORM=PLATFORM_ANDROID ANDROID_NDK=../../platform/android/ndk ANDROID_ARCH=x86_64 ANDROID_API_VERSION=34
        mv libraylib.a ../../platform/android/lib/x86_64
make clean
#back to main directory
cd ../..
#icons
cp raylib/logo/raylib_36x36.png platform/android/assets/icon_ldpi.png
cp raylib/logo/raylib_48x48.png platform/android/assets/icon_mdpi.png
cp raylib/logo/raylib_72x72.png platform/android/assets/icon_hdpi.png
cp raylib/logo/raylib_96x96.png platform/android/assets/icon_xhdpi.png
#this part you'd need to modify for your own usecase, if you're using this for a legit game
#or at least i think, never published anything on google play
cd platform/android
keytool -genkeypair -validity 1000 -dname "CN=raylib,O=Android,C=ES" -keystore raylib.keystore -storepass 'raylib' -keypass 'raylib' -alias projectKey -keyalg RSA
cd ../..
#the part that reads the program
mkdir --parents platform/android/build/src/com/raylib/game
NL=platform/android/build/src/com/raylib/game/NativeLoader.java
touch "$NL"
echo "package com.raylib.game;"                                       >  "$NL"
echo "public class NativeLoader extends android.app.NativeActivity {" >> "$NL"
echo "    static {"                                                   >> "$NL"
echo '        System.loadLibrary("main");'                            >> "$NL"
echo "    }"                                                          >> "$NL"
echo "}"                                                              >> "$NL"

#the android manifest
AM=platform/android/build/AndroidManifest.xml
touch "$AM"
echo '<?xml version="1.0" encoding="utf-8"?>'                                                           >  "$AM"
echo '<manifest xmlns:android="http://schemas.android.com/apk/res/android"'                             >> "$AM"
echo '        package="com.raylib.game"'                                                                >> "$AM"
echo '        android:versionCode="1" android:versionName="1.0" >'                                      >> "$AM"
echo '    <uses-sdk android:minSdkVersion="23" android:targetSdkVersion="34"/>'                         >> "$AM"
echo '    <uses-feature android:glEsVersion="0x00020000" android:required="true"/>'                     >> "$AM"
echo '    <uses-permission android:name="android.permission.WRITE_EXTERNAL_STORAGE"/>'                  >> "$AM"
echo '    <application android:allowBackup="false" android:label="Game" android:icon="@drawable/icon">' >> "$AM"
echo '        <activity android:name="com.raylib.game.NativeLoader"'                                    >> "$AM"
echo '            android:theme="@android:style/Theme.NoTitleBar.Fullscreen"'                           >> "$AM"
echo '            android:configChanges="orientation|keyboardHidden|screenSize"'                        >> "$AM"
echo '            android:exported="true"'                                                              >> "$AM"
echo '            android:screenOrientation="landscape" android:launchMode="singleTask"'                >> "$AM"
echo '            android:clearTaskOnLaunch="true">'                                                    >> "$AM"
echo '            <meta-data android:name="android.app.lib_name" android:value="main"/>'                >> "$AM"
echo '            <intent-filter>'                                                                      >> "$AM"
echo '                <action android:name="android.intent.action.MAIN"/>'                              >> "$AM"
echo '                <category android:name="android.intent.category.LAUNCHER"/>'                      >> "$AM"
echo '            </intent-filter>'                                                                     >> "$AM"
echo '        </activity>'                                                                              >> "$AM"
echo '    </application>'                                                                               >> "$AM"
echo '</manifest>'                                                                                      >> "$AM"
#all prepped and ready hopefully
