#here to prove emsdk is stupid

echo "Emscripten to use"
read -p "1)Use System Package 2)Use Latest (Bleeding Edge): " EMSCR

case "$EMSCR" in
    [1]*)#use installed system package (if one exists, otherwise error)
        sed -i 's/^EMSCRIPTEN_USE_GIT=.*/EMSCRIPTEN_USE_GIT=FALSE/' .scripts/config.sh
        if command -v emcc &>/dev/null; then
            cd raylib
            mkdir --parents build
            cd build
            emcmake cmake -DBUILD_SHARED_LIBS=OFF -DPLATFORM=Web -DOPENGL_VERSION=OFF -DBUILD_EXAMPLES=OFF ..
            emmake make
            mkdir --parents ../../platform/web/lib
            mv raylib/libraylib.a ../../platform/web/lib
            mkdir --parents ../../platform/web/include
            mv raylib/include/* ../../platform/web/include
        else
            echo "emscripten not installed, install it through your distro's package manager and restart your computer, otherwise use git version"
        fi
        ;;
    [2]*)#GIT version
        if [ ! -d "platform/external/emscripten" ]; then
            echo "Repository not found locally. Cloning..."
            mkdir --parents platform/external
            cd platform/external
            git clone https://github.com/emscripten-core/emscripten.git
            cd ../..
        else
            echo "checking updates..."
            #get in and check for updates
            cd platform/external/emscripten
            git fetch origin
            CURRENT_COMMIT=$(git rev-parse HEAD)
            LATEST_COMMIT=$(git rev-parse origin/main)

            if [ "$CURRENT_COMMIT" = "$LATEST_COMMIT" ]; then
                echo "Latest commit version detected, doing nothing"
            else
                echo "New commit detected, downloading latest version"
                git pull
            fi
            #get out
            cd ../../..
        fi
        sed -i 's/^EMSCRIPTEN_USE_GIT=.*/EMSCRIPTEN_USE_GIT=TRUE/' .scripts/config.sh
        cd platform/external/emscripten
        ./bootstrap.py
        cd ../../..
        #make raylib for web
        cd raylib
        mkdir --parents build
        cd build
        ../../platform/external/emscripten/emcmake.py cmake -DBUILD_SHARED_LIBS=OFF -DPLATFORM=Web -DOPENGL_VERSION=OFF -DBUILD_EXAMPLES=OFF ..
        ../../platform/external/emscripten/emmake.py make
        mkdir --parents ../../platform/web/lib
        mv raylib/libraylib.a ../../platform/web/lib
        mkdir --parents ../../platform/web/include
        mv raylib/include/* ../../platform/web/include
        ;;
esac
