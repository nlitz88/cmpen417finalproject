#include <stdio.h>
#include <stdlib.h>
#include "fpga417_final.h"

int test_top();
int test_cordic();
int test_top_fir();

int main(void)
{

	test_top();

	return 0;
}

int test_top() {

	// 1. Define an array of real and imaginary KERNEL coefficients as specified in lab handout for testing FIR filter.
	static int kernel_real[KERNEL_SIZE] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
	static int kernel_img[KERNEL_SIZE] = {25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};

	// 2. Define sample inputs.
	int input_real[INPUT_LENGTH] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
	int input_img[INPUT_LENGTH] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	for (int i = 0; i < KERNEL_SIZE; i++) {
		kernel_real[i] = 5;
		kernel_img[i] = -5;
	}

	for (int i = 0; i < INPUT_LENGTH; i++) {
		input_real[i] = 50;
		input_img[i] = 20;
	}

	// 3. Define float output arrays.
	float output_mag[INPUT_LENGTH];
	float output_angle[INPUT_LENGTH];

	// 4. Call fpga417_fir.
	fpga417_fir(input_real, input_img, kernel_real, kernel_img, output_mag, output_angle, INPUT_LENGTH);

	// 5. Print out its output values from each output stream.
	for (int i = 0; i < INPUT_LENGTH; i++) {
		printf("Output %d: Magnitude: %.6f, Phase: %.6f\n", i, output_mag[i], output_angle[i]);
	}

	return 0;

}

int test_cordic() {

	int x;
	int y;
	float mag;
	float angle;

	x = 3;
	y = 4;

	cordic(x, y, &mag, &angle);
	printf("Cartesian Form: Real Part: %d, Imag Part: %d --> Polar Form: Magnitude: %.3f Angle: %.3f\n", x, y, mag, angle);

	return 0;
}

int test_top_fir() {

	// 1. Define an array of real and imaginary KERNEL coefficients as specified in lab handout for testing FIR filter.
	static int kernel_real[KERNEL_SIZE] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25};
	static int kernel_img[KERNEL_SIZE] = {25,24,23,22,21,20,19,18,17,16,15,14,13,12,11,10,9,8,7,6,5,4,3,2,1};

	// 2. Define sample inputs.
	int input_real[INPUT_LENGTH] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1};
	int input_img[INPUT_LENGTH] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	// 3. Define test output stream.
	hls::stream<int> output_real;
	hls::stream<int> output_img;

	// 4. Call the top_fir function.
	top_fir(input_real, input_img, kernel_real, kernel_img, output_real, output_img, INPUT_LENGTH);

	// 5. Print out its output values from each output stream.
	for (int i = 0; i < INPUT_LENGTH; i++) {
		printf("FIR Output %d: Real Part: %d Imag Part: %d\n", i, output_real.read(), output_img.read());
	}

	return 0;
}
