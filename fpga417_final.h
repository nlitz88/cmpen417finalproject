#include <ap_fixed.h>
#include <ap_int.h>
#include <hls_math.h>
#include <hls_stream.h>

#define KERNEL_SIZE 25

// Prototype for hardware FIR filter. Performs a complex convolution and produces cartesian outputs.
void top_fir(int* input_real, int* input_img, int kernel_real[KERNEL_SIZE], int kernel_img[KERNEL_SIZE], hls::stream<int>&output_real, hls::stream<int>&output_img, int length);

// Prototype for CORDIC rotator used to convert cartesian outputs of top_fir to polar form.

// Prototype for overarching hardware FIR filter that takes in complex inputs and produces convolution
// result values in polar form. Does this by connecting top_fir and top_cordic_rotator.
