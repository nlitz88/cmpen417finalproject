#include <ap_fixed.h>
#include <ap_int.h>
#include <hls_math.h>
#include <hls_stream.h>

// Custom Arbitrary Fixed Point type for use by the CORDIC rotator.
//typedef ap_fixed <32,20> FIXED_POINT; // <12,2> used in example--possible to get down to this level? Probably not that low.
typedef ap_fixed <25, 12> FIXED_POINT;
// Custom Fixed Point types for FIR filter.
typedef ap_int <7> INP_INT; // 7 bit integer needed to represent input values and kernel coefficients.
typedef ap_int <14> OUT_INT; // 14 bit integer needed to represent FIR output values.

// FIR constants.
#define KERNEL_SIZE 25
#define INPUT_LENGTH 25

// CORDIC constants.
//const int NUM_CORDIC_ITERATIONS=32;
const int NUM_CORDIC_ITERATIONS=5; // 1/2^12 gives us 4 decimal points of precision, paper says to have 4 + 1 == 5 iterations.
const FIXED_POINT CORDIC_GAIN = 0.60735;	// For NUM_CORDIC_ITERATIONS > 5, cordic_gain converges to 1.64. 1/1.64 == ~0.60735.
// Array of angles that the input vector will be iteratively rotated by.
static FIXED_POINT cordic_phase[64]={0.78539816339744828000,0.46364760900080609000,0.24497866312686414000,0.12435499454676144000,0.06241880999595735000,0.03123983343026827700,0.01562372862047683100,0.00781234106010111110,0.00390623013196697180,0.00195312251647881880,0.00097656218955931946,0.00048828121119489829,0.00024414062014936177,0.00012207031189367021,0.00006103515617420877,0.00003051757811552610,0.00001525878906131576,0.00000762939453110197,0.00000381469726560650,0.00000190734863281019,0.00000095367431640596,0.00000047683715820309,0.00000023841857910156,0.00000011920928955078,0.00000005960464477539,0.00000002980232238770,0.00000001490116119385,0.00000000745058059692,0.00000000372529029846,0.00000000186264514923,0.00000000093132257462,0.00000000046566128731,0.00000000023283064365,0.00000000011641532183,0.00000000005820766091,0.00000000002910383046,0.00000000001455191523,0.00000000000727595761,0.00000000000363797881,0.00000000000181898940,0.00000000000090949470,0.00000000000045474735,0.00000000000022737368,0.00000000000011368684,0.00000000000005684342,0.00000000000002842171,0.00000000000001421085,0.00000000000000710543,0.00000000000000355271,0.00000000000000177636,0.00000000000000088818,0.00000000000000044409,0.00000000000000022204,0.00000000000000011102,0.00000000000000005551,0.00000000000000002776,0.00000000000000001388,0.00000000000000000694,0.00000000000000000347,0.00000000000000000173,0.00000000000000000087,0.00000000000000000043,0.00000000000000000022,0.00000000000000000011};

// Create function that performs complex multiplication.
void complex_mult(int ar, int ai, int br, int bi, int* o_r, int* o_i);

// Function that performs a single step of convolution == a dot product with the kernel
// and the current values of the input within the sliding window.
void fir(int input_r, int input_i, int filter_r[KERNEL_SIZE], int filter_i[KERNEL_SIZE], int* output_r, int* output_i);

// Prototype for hardware FIR filter. Performs a complex convolution and produces cartesian outputs.
void top_fir(int* input_real, int* input_img, int kernel_real[KERNEL_SIZE], int kernel_img[KERNEL_SIZE], hls::stream<int>&output_real, hls::stream<int>&output_img, int length);

// Function to perform each CORDIC rotation.
void cordic(int cos, int sin, float *output_mag, float *output_angle);

// Prototype for CORDIC rotator used to convert cartesian outputs of top_fir to polar form.
void top_cordic_rotator(hls::stream<int>&input_real, hls::stream<int>&input_img, float* output_mag, float* output_angle, int length);

// Prototype for overarching hardware FIR filter that takes in complex inputs and produces convolution
// result values in polar form. Does this by connecting top_fir and top_cordic_rotator.
void fpga417_fir(int* input_real, int* input_img, int* kernel_real, int* kernel_img, float* output_mag, float* output_angle, int input_length);
