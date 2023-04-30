#include "cordic.h"
#include "stdio.h"

int main()
{
	double theta_double=0.785;//45 degrees



FIXED_POINT theta=theta_double;// standard double to fixed point


double cos_double;
double sin_double;
FIXED_POINT cos;
FIXED_POINT sin;


cordic(theta,&cos,&sin);

cos_double=cos.to_double();
sin_double=sin.to_double();//remember double is 64 bit, float is 32 bit


printf("fixed_point print cos= %s ,sin=%s \n", cos.to_string(10).c_str(),sin.to_string(10).c_str());
printf("standard double print cos=%f, sin=%f \n",cos.to_double(),sin.to_double());

}

