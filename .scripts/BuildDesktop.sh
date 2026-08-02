echo "OpenGL backend to use:"
read -p "1)GLFW 2)SDL 3)RGFW 4)DRM/RPi 5)CPU/SOFTWARE: " PLAT
if [ "$PLAT" != 5 ]; then
    echo "Force a specific OpenGL version? (UNTESTED but should work fine) :"
    read -p "1)No 2)1.1 3)2.1 4)3.3 5)4.3: " GL
fi


case "$PLAT" in
    [1]*)#GLFW
        FORM=Desktop
        sed -i 's/^TARGET_BACKEND=.*/TARGET_BACKEND=BACKEND_GLFW/' .scripts/config.sh
        ;;
    [2]*)#SDL
        FORM=SDL
        sed -i 's/^TARGET_BACKEND=.*/TARGET_BACKEND=BACKEND_SDL/' .scripts/config.sh
        ;;
    [3]*)#RGFW
        FORM=RGFW
        sed -i 's/^TARGET_BACKEND=.*/TARGET_BACKEND=BACKEND_RGFW/' .scripts/config.sh
        ;;
    [4]*)#RPi
        FORM=DRM
        sed -i 's/^TARGET_BACKEND=.*/TARGET_BACKEND=BACKEND_DRM/' .scripts/config.sh
        ;;
    [5]*)#SOFTWARE, which only works through SDL and DRM atm, its also quite crash prone
        FORM=SDL
        sed -i 's/^TARGET_BACKEND=.*/TARGET_BACKEND=BACKEND_SDL/' .scripts/config.sh
        BACKEND=Software
        ;;
    *) #Panic, use GLFW
        echo "invalid input, using base GLFW"
        FORM=Desktop
        sed -i 's/^TARGET_BACKEND=.*/TARGET_BACKEND=BACKEND_GLFW/' .scripts/config.sh
        ;;
esac
if [ "$PLAT" != 5 ]; then
    case "$GL" in
        [1]*) BACKEND=OFF;;
        [2]*) BACKEND=1.1;;
        [3]*) BACKEND=2.1;;
        [4]*) BACKEND=3.3;;
        [5]*) BACKEND=4.3;;
        *) echo "Panic, use default"
            BACKEND=OFF
            ;;
    esac
fi
# echo "SDL Version to use: "
# read -p "1)SDL1 (UNSUPPORTED?) 2)SDL2 3)SDL3 : " SDL_VER
#case "$SDL_VER" in
#esac

cd raylib
mkdir --parents build
cd build
cmake -DBUILD_SHARED_LIBS=OFF -DPLATFORM=$FORM -DOPENGL_VERSION=$BACKEND -DBUILD_EXAMPLES=OFF ..
make
mkdir --parents ../../platform/desktop/lib
mv raylib/libraylib.a ../../platform/desktop/lib
mkdir --parents ../../platform/desktop/include
mv raylib/include/* ../../platform/desktop/include
