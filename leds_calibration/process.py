import cv2
import numpy as np
from matplotlib import pyplot as plt
from prefs import *

def do_process(image_name):
    img0 = cv2.imread(image_name)
    gray = cv2.cvtColor(img0, cv2.COLOR_RGB2GRAY)
    h, w = gray.shape[:2]

    perspSrc = np.array(input_image_rect, dtype=np.float32)
    perspDst = np.array([[0, 0], [output_image_size[0]-1, 0], [output_image_size[0]-1, output_image_size[1]-1], [0, output_image_size[1]-1]], dtype=np.float32)
    perspectiveTransform = cv2.getPerspectiveTransform(perspSrc, perspDst)
    img1 = cv2.warpPerspective(gray, perspectiveTransform, (output_image_size[0], output_image_size[1]))

    #perspSrc = np.array([ # X, Y
    #    [850, 1000], # Top left
    #    [2850, 1000], # Top right
    #    [2850, 1550], # Bot right
    #    [850, 1550]  # Bot left
    #    ], dtype=np.float32)
    #perspDst = np.array([[0, 0], [1000-1, 0], [1000-1, 500-1], [0, 500-1]], dtype=np.float32)
    #perspectiveTransform = cv2.getPerspectiveTransform(perspSrc, perspDst)
    #img1 = cv2.warpPerspective(gray, perspectiveTransform, (1000, 500))

    (thr, img2) = cv2.threshold(img1, 200, 255, cv2.THRESH_BINARY)

    moments = cv2.moments(img2, True)
    mm10 = moments["m10"]
    mm01 = moments["m01"]
    mm00 = moments["m00"]
    if mm00 == 0.0:
        return (0, 0)
    return (int(mm10 / mm00), int(mm01 / mm00))

with open("leds.txt", "w") as f:
    for i in range(7):
        for k in range(60):
            out = do_process("out/{}_{}.jpg".format(i, k))

            #x_norm = (out[0] - out_point_padding[0]) / (output_image_size[0] - out_point_padding[0]*2)
            #y_norm = (out[1] - out_point_padding[1]) / (output_image_size[1] - out_point_padding[1]*2)

            text = "{}_{}: {} {}\n".format(i, k, out[0], out[1])
            f.write(text)
            print(text)
