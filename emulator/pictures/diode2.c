 #include <stdio.h>

#define PICTURE_X 24
#define PICTURE_Y 24
#define PICTURE_Z 24

int pixel(int x, int y, int z) {
	int paint = 0;
	if (z >= 10 && z <= 12) {
/*
		if (((x == (PICTURE_X-1)/2) && (z == (PICTURE_Y-1)/2)) ||
		    ((x == (PICTURE_X-1)/2) && (y == (PICTURE_Z-1)/2)) ||
		    ((z == (PICTURE_Y-1)/2) && (y == (PICTURE_Z-1)/2))) {
			done = 1;
		}
*/

		if (y < 4) {
			if (x >= 10 && x <= 12)
				paint = 1;
		} else if (y < 7) {
			paint = 1;
		} else if (y < 17) {
			if (y - 6 == x || y - 5 == x || y - 4 == x)
				paint = 1;
			if (y == 26 - x || y == 27 - x || y == 28 - x)
				paint = 1;
		} else if (y < 20) {
			paint = 1;
		} else {
			if (x >= 10 && x <= 12)
				paint = 1;
		}
	}

	if (paint) {
		putchar(0xff);
		putchar(0x00);
		putchar(0x00);
		return 1;
	}
	return 0; // OK draw
}

int main() {
	int x, y, z;
	for (x = 0; x < PICTURE_X; ++x) {
		for (y = 0; y < PICTURE_Y; ++y) {
			for (z = 0; z < PICTURE_Z; ++z) {
				int done = pixel(x, y, z);
				if (!done) {
					putchar(0x00);
					putchar(0x00);
					putchar(0x00);
				}
			}
		}
	}
	return 0;
}
