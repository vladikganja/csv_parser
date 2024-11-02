#/usr/bin/bash

if [[ "$1" == "rm" ]]; then
    echo "Deleting build..."
    rm -rf ./.build
fi

if ! test -d .build; then
    mkdir .build
fi
cd .build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --config Release -j 8


