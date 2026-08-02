RAYLIB="https://github.com/raysan5/raylib.git"
RAYDIR="raylib"

#does raylib exist?
if [ ! -d "$RAYDIR" ]; then
    echo "Repository not found locally. Cloning..."
    git clone "$RAYLIB"
    #temporary til i find a better idea
    mkdir --parents build
else
    echo "checking updates..."
    #get in and check for updates
    cd raylib
    git fetch origin
    CURRENT_COMMIT=$(git rev-parse HEAD)
    LATEST_COMMIT=$(git rev-parse origin/master)

    if [ "$CURRENT_COMMIT" = "$LATEST_COMMIT" ]; then
        echo "Latest commit version detected, doing nothing"
    else
        echo "New commit detected, downloading latest version"
        git pull
    fi
    #get out
    cd ..
fi
