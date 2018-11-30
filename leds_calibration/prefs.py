input_image_rect = [ # X, Y
        [850, 1000], # Top left
        [2850, 1000], # Top right
        [2850, 1570], # Bot right
        [850, 1570]  # Bot left
        ]

output_image_size = [1000, 500]

out_point_padding = [50, 70]

import numpy as np

def do_norm_coords(coords):
    x_norm = (coords[0] - out_point_padding[0]) / (output_image_size[0] - out_point_padding[0]*2)
    y_norm = (coords[1] - out_point_padding[1]) / (output_image_size[1] - out_point_padding[1]*2)
    return [np.clip(x_norm, 0.0, 1.0), np.clip(y_norm, 0.0, 1.0)]
