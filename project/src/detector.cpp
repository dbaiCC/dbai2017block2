#include "inclusion.hpp"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <cmath>
#include <iostream>
#include <dirent.h>

using namespace cv;
using namespace std; //unsure if this is necessary/desireable

struct pixel {
    double red;
    double green;
    double blue;
    double intensity;

    pixel(double r, double g, double b) : red(r), green(g), blue(b) {};
};

void gaussian_kernel(const int size, const double stddev, double * const kernel) {
    // values needed for the curve
    const double denom = 2.0 * stddev * stddev;
    const double g_denom = M_PI * denom;
    const double g_denom_recip = (1.0/g_denom);
    // accumulator so that values can be normalized to 1
    double sum = 0.0;

    // Build the template
    for(int i = 0; i < size; ++i) {
        for(int j = 0; j < size; ++j) {
            const double row_dist = i - (size/2);
            const double col_dist = j - (size/2);
            const double dist_sq = (row_dist * row_dist) + (col_dist * col_dist);
            const double value = g_denom_recip * exp((-dist_sq)/denom);
            kernel[i + (j*size)] = value;
            sum += value;
        }
    }

    // Normalize
    const double recip_sum = 1.0 / sum;
    for(int i = 0; i < size; ++i) {
        for(int j = 0; j < size; ++j) {
            kernel[i + (j*size)] *= recip_sum;
        }
    }
}

void apply_blur(int radius, const double stddev, const int x1, const int y1, const int x2, const int y2, const int rows, const int cols, pixel * const in, pixel * const out) {
    //make sure 0 <= x1 < x2 <= cols and 0 <= y1 < y2 <= rows ??
    const int dim = radius*2+1;
    double kernel[dim*dim];
    gaussian_kernel(dim, stddev, kernel);

    // #pragma omp parallel for
    for(int i = y1; i < y2; ++i) {
        for(int j = x1; j < x2; ++j) {
            const int out_offset = i + (j*rows);
            //NOTE: small change from adapted code
            //TODO: Mostly works, but check edge cases (see example in misc)
            out[out_offset].red   = 0;
            out[out_offset].green = 0;
            out[out_offset].blue  = 0;
            for(int x = i - radius, kx = 0; x <= i + radius; ++x, ++kx) {
                for(int y = j - radius, ky = 0; y <= j + radius; ++y, ++ky) {
                    //TODO: consider the edge cases instead of treating them as 0s (by not adding)
                    if(x >= 0 && x < rows && y >= 0 && y < cols) {
                        // Acculate intensities in the output pixel
                        const int in_offset = x + (y*rows);
                        const int k_offset = kx + (ky*dim);
                        out[out_offset].red   += kernel[k_offset] * in[in_offset].red;
                        out[out_offset].green += kernel[k_offset] * in[in_offset].green;
                        out[out_offset].blue  += kernel[k_offset] * in[in_offset].blue;
                    }
                }
            }
        }
    }
}

int main(int argc, char **argv){
    if(argc != 3) {
        printf("Usage: %s inputFolderName outputFolderName\n", argv[0]);
        return 1;
    }

    // Obtained from: https://github.com/opencv/opencv/blob/master/data/haarcascades/haarcascade_frontalface_alt.xml
    CascadeClassifier faceDetector = CascadeClassifier("haarcascade_frontalface_alt.xml");
    // faceDetector.load("./haarcascade_face.xml");

    //from https://stackoverflow.com/questions/612097/how-can-i-get-the-list-of-files-in-a-directory-using-c-or-c
    DIR *dir;
    struct dirent *ent;
    if ( (dir = opendir (argv[1])) == NULL ) {
        printf("Error attempting to access input directory %s!", argv[1]);
        return -1;
    }
    int count = 0;
    while ( (ent = readdir(dir)) != NULL ) {
        char *f_ext = strrchr(ent->d_name, '.'); //https://stackoverflow.com/questions/10347689/how-can-i-check-whether-a-string-ends-with-csv-in-c
        if ( f_ext && !strcmp(f_ext, ".jpg") ){
          count++;
        }
    }
    char f_names[count][256]; //hardcoded buffer size of 100 chars

    rewinddir(dir);
    count = 0;
    while ( (ent = readdir(dir)) != NULL ){
        //Saving filenames to array (can parallel process chunks)
        char *f_ext = strrchr(ent->d_name, '.');
        if ( f_ext && !strcmp(f_ext, ".jpg") ){
            char f_name[256];
            // sprintf(f_name, "%s/%s", argv[1], ent->d_name);
            // f_names[count] = f_name;
            f_names[count] = ent->d_name;
            count++;
        }
    }
    closedir(dir);

    for(int i = 0; i < count; i++){
        printf("%s/%s", argv[1], f_names[count]);
        printf("%s/%s", argv[2], f_names[count]);
    }

    //for loop here
        //above prints are for testing, as is below line. Everything else commented out to come back
        // Mat image = imread(argv[1]+ent->d_name, CV_LOAD_IMAGE_COLOR);
        // if(image.empty()){
        //     printf("Empty or bad file\n");
        //     return -1;
        // }
        //
        // const int rows = image.rows;
        // const int cols = image.cols;
        // pixel * inPixels = (pixel *) malloc(rows * cols * sizeof(pixel));
        // for(int i = 0; i < rows; ++i) {
        //     for(int j = 0; j < cols; ++j) {
        //         Vec3b p = image.at<Vec3b>(i, j);
        //         inPixels[i + (j*rows)] = pixel(p[0]/255.0,p[1]/255.0,p[2]/255.0);
        //     }
        // }
        //
        // //create output pixels
        // pixel * outPixels = (pixel *) malloc(rows * cols * sizeof(pixel));
        // memcpy(outPixels, inPixels, sizeof(pixel)*rows*cols);
        //
        // // https://stackoverflow.com/questions/15893591/confusion-between-opencv4android-and-c-data-types
        // vector <Rect> faceDetections;
        // faceDetector.detectMultiScale(image, faceDetections);
        //
        // if(faceDetections.size() > 0 ){ //might not need this statement
        //     for ( vector <Rect>::iterator rect_iter = faceDetections.begin(); rect_iter != faceDetections.end(); ++rect_iter) {
        //         apply_blur(10, 1024.0, rect_iter->x, rect_iter->y, rect_iter->x + rect_iter->width, rect_iter->y + rect_iter->height, rows, cols, inPixels, outPixels);
        //     }
        // }
        //
        // Mat dest(rows, cols, CV_8UC3);
        // for(int i = 0; i < rows; ++i) {
        //     for(int j = 0; j < cols; ++j) {
        //         const size_t offset = i + (j*rows);
        //         dest.at<Vec3b>(i, j) = Vec3b(floor(outPixels[offset].red * 255.0),
        //                                      floor(outPixels[offset].green * 255.0),
        //                                      floor(outPixels[offset].blue * 255.0));
        //     }
        // }
        // //TODO: check if imwrite can create folders... look at https://stackoverflow.com/questions/9235679/create-a-directory-if-it-doesnt-exist maybe?
        // manually create output folder
        // imwrite(argv[2]+"/"+ent->d_name, dest);
        //
        // free(inPixels);
        // free(outPixels);
    return 1;
}
