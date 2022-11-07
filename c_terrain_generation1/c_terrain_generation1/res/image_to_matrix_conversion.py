import cv2
import numpy as np

img = cv2.imread("island3.png", 0) # read image as grayscale. Set second parameter to 1 if rgb is required 

string = "{"
with open("dat.txt", 'w') as f:
    for k in range(1024):
        for i in img[k] :
            string += str(i) + ","
        string.rstrip(string[-1])
        string += "\n"
    string += "};"
    f.write(string)