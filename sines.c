#include <math.h>
#include <stdio.h>

int main() {
	for (int i = 0; i <= 90; i = i+6) {
		printf("%9.7f\t//%2i\n", sinf(i*M_PI/180), i);
	}
}
