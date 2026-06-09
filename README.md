![img](snap.png)

snap is a minimal screenshot tool for X11

It captures screen contents and writes a raw PPM (P6) image to stdout.it does not save files by itself and does not perform image conversion.

## Building
```
make
sudo make install
```
- If you dont like to use `sudo`, install it somewhere in your home directory

```
make install PREFIX=$HOME/.local
```
## Dependencies
- XLib (required)

## Usage

- snap always writes image data to stdout.
- Redirect the output to a file or pipe it to another program for conversion or storage.

### capture the fullscreen 
```
snap -f > img.ppm
```

### capture the current active window
```
snap -w > img.ppm 
```

### capture selection

```
snap -s > image.ppm
```

## Convert to PNG using ImageMagick
```
snap | magick ppm:- image.png
```



