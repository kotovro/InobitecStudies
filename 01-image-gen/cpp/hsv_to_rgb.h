#ifndef KV_HSV_TO_RGB_H
#define KV_HSV_TO_RGB_H

struct RGB {
    int r;
    int g;
    int b;
};

RGB hsv_to_rgb(double hue, double saturation, double value);

#endif // KV_HSV_TO_RGB_H
