import shutil

# Makes a linear sequence of images the way FFMPEG wants it. Used to make fancy gifs

i = 0
for x in range(7):
    for y in range(60):
        shutil.copy("out/{}_{}.jpg".format(x, y), "seq/{}.jpg".format(i))
        i += 1
