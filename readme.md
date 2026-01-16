# Build

## Web

CMake configure and initial build

```
emcmake cmake -S . -B build/web -GNinja
cmake --build build/web
```

Run httpd (automatically rebuilds JS + WASM):

```
python toold/dev-httpd.py
```

Or do it manually

```
cp resources/html/* build/web
python -m http.server -d build/web
```