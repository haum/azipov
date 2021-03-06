#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <ctime>
#include <cstring>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <getopt.h>

/** Timespec for FPS limiting **/
struct timespec wakeup;

/** Animation variable **/
float ani = 1;

/** Screen data **/
struct {
	int width = 800;
	int height = 600;
} screen;

/** Camera data **/
struct {
	float distance = 20.0f;
	float angle_y = 90.0f;
	float angle_z = 0.0f;
} camera;

struct Led {
	int wheel_nr; // Number of wheel on which the led bar is present
	float r; // Radius of led bar position
	float alpha; // Angle of lef bar position
};

/** POV emulation parameters **/
struct {
	bool animated; // Is it animated ?
	bool trace; // Should a trace to be printed ?
	std::vector <Led> leds; // List of leds
	int turns; // Number of turns to show
	int nr; // Number of wheels
	float da; // Angular step to display
	float a; // Radius of inner circle
	float b; // Radius of outer circle
	float dh; // Height step
	float h; // Height
} emu;

/** Color **/
struct color {
	uint8_t r; // Red
	uint8_t g; // Green
	uint8_t b; // Blue
};

/** Colors Buffer **/
#define PICTURE_X 24
#define PICTURE_Y 24
#define PICTURE_Z 24
color picture[PICTURE_X][PICTURE_Y][PICTURE_Z];

/** Gives a color depending on led position
  * @param [in] x X position in -1..1 range
  * @param [in] y Y position in -1..1 range
  * @param [in] z Z position in 0..1 range
  * @return color to set
  */
color color_chooser(float x, float y, float z) {
	if (x < -1) x = -1;
	if (x > 1) x = 1;
	if (y < -1) y = -1;
	if (y > 1) y = 1;
	if (z < 0) z = 0;
	if (z > 1) z = 1;

	int ix, iy, iz;
	ix = (x + 1) / 2 * (PICTURE_X - 1);
	iy = (y + 1) / 2 * (PICTURE_Y - 1);
	iz = z * (PICTURE_Z - 1);

	return picture[ix][iy][iz];
}

/** Draw all leds of a wheel, and optionnaly the wheel itself
  * @param [in] wheel_nr Wheel number
  * @param [in] angle    Current angle of the wheel
  * @param [in] circle   Should wheel be printed
  */
void draw_leds(int wheel_nr, float angle, bool circle = false) {
	glPushMatrix();

	// Global position
	glRotatef(angle, 0, 0, 1);
	glTranslatef(emu.a + emu.b, 0, 0);
	glRotatef(-angle, 0, 0, 1);
	glRotatef(angle * (emu.a+emu.b)/(emu.b), 0, 0, 1);

	// Draw circle
	if (circle) {
		int circle_pts = emu.b * 9;
		glBegin(GL_LINE_LOOP);
		glColor3d(0, 0, 0.3f);
		for (int i = 0; i < circle_pts; ++i)
			glVertex3d(
				emu.b * cos(i * 2 * M_PI / circle_pts),
				emu.b * sin(i * 2 * M_PI / circle_pts),
				0
			);
		glEnd();
		for (Led & led: emu.leds) {
			if (wheel_nr != led.wheel_nr)
				continue;

			glBegin(GL_LINES);
			glVertex3d(0, 0, 0);
			glVertex3d(
				led.r * cos(led.alpha * M_PI / 180),
				led.r * sin(led.alpha * M_PI / 180),
				0
			);
			glEnd();
		}
	}

	// Draw leds
	static float max_x = 1, max_y = 1;
	for (Led & led: emu.leds) {
		if (wheel_nr != led.wheel_nr)
			continue;

		glBegin(GL_POINTS);
		for (float h = 0; h <= emu.h; h += emu.dh) {
			float x, y, z;
			color c;
			// Hum, there is probably a better way to compute sin/cos with matrix since we do it in incremental way
			x = (emu.a + emu.b) * sin(angle * M_PI / 180 ) + led.r * sin(((emu.a+emu.b)/(emu.b) * angle + led.alpha) * M_PI / 180);
			y = (emu.a + emu.b) * cos(angle * M_PI / 180 ) + led.r * cos(((emu.a+emu.b)/(emu.b) * angle + led.alpha) * M_PI / 180);
			if (fabs(x) > max_x) max_x = fabs(x);
			if (fabs(y) > max_y) max_y = fabs(y);
			x /= max_x;
			y /= max_y;
			z = (emu.h > 0) ? h / emu.h : h;
			c = color_chooser(x, y, z);
			if (c.r || c.g || c.b) {
				glColor3d(c.r*(1.0/255), c.g*(1.0/255), c.b*(1.0/255));
				glVertex3d(
					led.r * cos(led.alpha * M_PI / 180),
					led.r * sin(led.alpha * M_PI / 180),
					h
				);
			}
		}
		glEnd();
	}

	glPopMatrix();
}

/** Display function called to redraw scene **/
void display() {
	// Init
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPointSize(3.0);

	// Camera
	gluLookAt(
		camera.distance, 0, 0, // Eye
		0, 0, 0, // Target
		0, 0, 1 // Up
	);
	glRotated(camera.angle_y, 0, 1, 0);
	glRotated(camera.angle_z, 0, 0, 1);

	// Ground
	glBegin(GL_QUADS);
	glColor3d(0.7f, 0.7f, 0.7f);
	glVertex3d(-10, -10, 0);
	glVertex3d(-10, 10, 0);
	glColor3d(1.0f, 1.0f, 1.0f);
	glVertex3d(10, 10, 0);
	glVertex3d(10, -10, 0);
	glEnd();

	// Circle A
	int circle_pts = emu.a * 9;
	glBegin(GL_LINE_LOOP);
	glColor3d(0, 0.3f, 0);
	for (int i = 0; i < circle_pts; ++i)
		glVertex3d(
			emu.a * cos(i * 2 * M_PI / circle_pts),
			emu.a * sin(i * 2 * M_PI / circle_pts),
			0
		);
	glEnd();

	// Leds
	if (emu.trace) {
		for (float a = 0; a < ani * emu.turns * 360; a += emu.da) {
			for (int n = 0; n < emu.nr; ++n) {
				draw_leds(n, a + 360 * n / emu.nr, ((a + emu.da) > (ani * emu.turns * 360)));
			}
		}
	} else {
		float a = ani * emu.turns * 360;
		for (int n = 0; n < emu.nr; ++n) {
			draw_leds(n, a + 360 * n / emu.nr, ((a + emu.da) > (ani * emu.turns * 360)));
		}
	}

	// Flush
	glFlush();
	glutSwapBuffers();
}

/** Reshape function called when window is resized  **/
void reshape(int width, int height) {
	screen.width = width;
	screen.height = height;
	glViewport(0, 0, screen.width, screen.height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(70, (double) screen.width / screen.height, 1, 1000);
	glMatrixMode(GL_DEPTH_TEST);
}

/** Passive motion function called when mouse moves **/
void pmotion(int x, int y) {
	if (x < 0 || x > screen.width || y < 0 || y > screen.height) return;
	camera.angle_y = (((double)(screen.height - y)/screen.height)) * 90;
	camera.angle_z = -(((double)(screen.width - x)/screen.width) - 0.5) * 360;
}

/** Motion function called when mouse moves while clicking **/
void motion(int x, int y) {
	if (x < 0 || x > screen.width || y < 0 || y > screen.height) return;
	camera.distance = (1 - ((double)(screen.height - y)/screen.height)) * 40 + 2;
}

/** Keyboard function called when ordinary key is pressed **/
void keyboard(unsigned char key, int x, int y) {
	if (key == 27) // escape
		exit(0);
	else if (key == 111) // o
		emu.animated = false, emu.trace = true, ani = 1;
	else if (key == 116) // t
		emu.trace = !emu.trace;
	else if (key == 32) // space
		emu.animated = !emu.animated;
}

/** Idle function used to limit framerate **/
void idle() {
	float anim_intervalle = 40e6;
	wakeup.tv_nsec += anim_intervalle;
	if (wakeup.tv_nsec > 1e9) {
		wakeup.tv_nsec -= 1e9;
		wakeup.tv_sec += 1;
	}
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &wakeup, NULL);
	if (emu.animated) {
		ani += 0.01 / emu.turns;
		if (ani > 1)
			ani = 0;
	}
	glutPostRedisplay();
}

/** Print usage message **/
void usage() {
	std::cout << "This is a little AziPOV emulator" << std::endl
	          << "    --animated      starts program with animation in play" << std::endl
	          << "    --no-trace      starts program with led trace disabled" << std::endl
	          << "    --width <w>     starts window with a specified width" << std::endl
	          << "    --height <h>    starts window with a specified height" << std::endl
	          << "    --turns <t>     number of turns to show" << std::endl
	          << std::endl
	          << "    --da <da>       angular resolution in degrees" << std::endl
	          << "    --a <a>         size of inner wheel" << std::endl
	          << "    --b <b>         size of outer wheel" << std::endl
	          << "    --dh <dh>       vertical resolution" << std::endl
	          << "    --h <h>         length of led bars" << std::endl
	          << "    --nr <nr>       number of wheels" << std::endl
	          << std::endl
	          << "    --led|-l <led>  add a led (see below)" << std::endl
	          << "    --pic|-p <p>    read from picture file p" << std::endl
	          << std::endl
	          << std::endl
	          << "A led is described in following syntax: [wheel:]radius[@angle]" << std::endl
	          << "e.g. 1:5@120 is a led on wheel number 1 located at a distance of 5 and an angle of 120 degrees" << std::endl
	          << std::endl
	          << "Sample command line: --h 0 --animated --no-trace --a 2 --b 2 --nr 3 -l 4@120 -l 1:4@240 -l 2:4 --turns 1" << std::endl
	          << std::endl
	          << "Orientation is chosen by draging mouse on window" << std::endl
	          << "Zoom is chosen by clicking on window (more zoom on top of window)" << std::endl
	          << "Key \"t\" changes trace status" << std::endl
	          << "Key \"o\" stops animation with trace at 100%" << std::endl
	          << "Key \"ESC\" closes emulator" << std::endl
	          << "Key \"SPACE\" changes animation status" << std::endl
	          << std::endl
	          << "Have fun" << std::endl;
}

/** Parse options **/
int parse_options(int argc, char * argv[]) {
	// Generic parameters
	char * picturename = nullptr;
	int c;
	int option_index = 0;
	emu.animated = false;
	emu.trace = true;
	emu.nr = 2;
	emu.turns = 5;
	emu.da = 2;
	emu.a = 2;
	emu.b = 2.5;
	emu.dh = 0.7;
	emu.h = 11.2;
	for (int ix = 0; ix < PICTURE_X; ++ix) {
		for (int iy = 0; iy < PICTURE_Y; ++iy) {
			for (int iz = 0; iz < PICTURE_Z; ++iz) {
				color c = {255, 0, 0};
				picture[ix][iy][iz] = c;
			}
		}
	}
	struct option generic_options[] = {
		{"animated", no_argument, 0, 0x01},
		{"no-trace", no_argument, 0, 0x02},
		{"width", required_argument, 0, 0x03},
		{"height", required_argument, 0, 0x04},
		{"turns", required_argument, 0, 0x07},
		{"help", no_argument, 0, 0x08},

		{"da", required_argument, 0, 0x05},
		{"a", required_argument, 0, 'a'},
		{"b", required_argument, 0, 'b'},
		{"dh", required_argument, 0, 0x06},
		{"h", required_argument, 0, 'h'},
		{"nr", required_argument, 0, 'n'},

		{"led", required_argument, 0, 'l'},
		{"pic", required_argument, 0, 'p'},

		{0, 0, 0, 0}
	};

	while((c = getopt_long(argc, argv, "a:b:h:l:p:", generic_options, &option_index)) != -1) {
		if (c == '?') {
			usage();
			return 1;
		}

		unsigned long optvalul = 0;
		float optvalf = 0;
		if (optarg) {
			optvalul = strtoul(optarg, NULL, 10);
			optvalf = strtof(optarg, NULL);
		}

		if (c == 0x01) {
			emu.animated = true;

		} else if (c == 0x02) {
			emu.trace = false;

		} else if (c == 0x03 && optvalul) {
			screen.width = optvalul;

		} else if (c == 0x04 && optvalul) {
			screen.height = optvalul;

		} else if (c == 0x05) {
			emu.da = optvalf;

		} else if (c == 'h') {
			emu.h = optvalf;

		} else if (c == 0x06) {
			emu.dh = optvalf;

		} else if (c == 0x07) {
			emu.turns = optvalul;

		} else if (c == 0x08) {
			usage();
			return 2;

		} else if (c == 'a') {
			emu.a = optvalf;

		} else if (c == 'b') {
			emu.b = optvalf;

		} else if (c == 'n') {
			emu.nr = optvalul;

		} else if (c == 'l') {
			char *str, *t1, *t2, *t3;
			str = optarg;
			t1 = strpbrk(str, ":");
			if (t1) {
				*t1 = 0;
				str = t1 + 1;
				t1 = optarg;
			}
			t2 = strpbrk(str, "@");
			if (t2) {
				*t2 = 0;
				t3 = t2 + 1;
				t2 = str;
			} else {
				t2 = str;
				t3 = 0;
			}

			Led l;
			l.wheel_nr = (!t1 || !*t1) ? 0 : strtoul(t1, NULL, 10);
			l.r = (!t2 || !*t2) ? emu.b : strtof(t2, NULL);
			l.alpha = (!t3 || !*t3) ? 0 : strtof(t3, NULL);
			emu.leds.push_back(l);
		} else if (c == 'p') {
			if (picturename != nullptr)
				std::cerr << "WARNING " << picturename << " will not be used because another picture option is set" << std::endl;
			picturename = optarg;
		}
	}

	if (emu.leds.size() == 0) {
		for (int n = 0; n < emu.nr; ++n) {
			Led l;
			const float dephasage = 76;
			l.wheel_nr = n;
			l.r = emu.a + emu.b;
			l.alpha = 0 + n * dephasage / emu.nr;
			emu.leds.push_back(l);
			l.alpha = 120 + n * dephasage / emu.nr;
			emu.leds.push_back(l);
			l.alpha = 240 + n * dephasage / emu.nr;
			emu.leds.push_back(l);
		}
	}

	if (picturename != nullptr) {
		int x, y, z;
		std::ifstream f(picturename);
		for (x = 0; x < PICTURE_X; ++x) {
			for (y = 0; y < PICTURE_Y; ++y) {
				for (z = 0; z < PICTURE_Z; ++z) {
					picture[x][y][z].r = f.get();
					picture[x][y][z].g = f.get();
					picture[x][y][z].b = f.get();
				}
			}
		}
		f.close();
	}

	return 0;
}

/** Main function used as entry point **/
int main(int argc, char * argv[]) {
	// Command line options
	int parsing = parse_options(argc, argv);
	if (parsing)
		return parsing;
	std::cout << "animated: " << emu.animated << std::endl
	          << "turns: " << emu.turns << std::endl
	          << "da: " << emu.da << std::endl
	          << "a: " << emu.a << std::endl
	          << "b: " << emu.b << std::endl
	          << "dh: " << emu.dh << std::endl
	          << "h: " << emu.h << std::endl
	          << "nr: " << emu.nr << std::endl;

	// Init glut
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(screen.width, screen.height);
	glutCreateWindow("HAUM AziPOV");

	// Register callbacks
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutMotionFunc(motion);
	glutPassiveMotionFunc(pmotion);
	glutKeyboardFunc(keyboard);
	glutIdleFunc(idle);

	// Start loop
	clock_gettime(CLOCK_MONOTONIC, &wakeup);
	glutMainLoop();

	return 0;
}
