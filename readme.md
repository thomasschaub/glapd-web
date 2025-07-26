# Build

## Web

```
emcmake cmake -S . -B build/web -GNinja
cmake --build build/web
```

```
cp resources/html/* build/web
python -m http.server -d build/web
```