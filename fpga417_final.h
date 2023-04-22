
#define DATA_LENGTH 25
#define FILTER_LENGTH 25

// Complex multiplication prototype.
void cmplx(int ar, int ai, int br, int bi, int* or, int* oi);

// FIR function prototype.
void fir(int input_r, int input_i, int* output_r, int filter_r[FILTER_LENGTH], int filter_i[FILTER_LENGTH], int* output_i);

// Prototype for hardware accelerated FIR filter created with HLS.
void fpga417_fir(int* input, int* filter);
