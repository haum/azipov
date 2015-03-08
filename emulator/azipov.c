#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#include <vector>
#include <cmath>
#include <ctime>
#include <iostream>
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

/** POV emulation modes **/
enum ShapeMode {
	UNDEFINED,
	CYLINDER_RADIAL,
	EPITROCHOID,
	SHAPE_MODE_MAX = EPITROCHOID
};

/** POV emulation parameters **/
struct {
	ShapeMode shapemode;
	bool animated;
	std::vector <float> leds;
	union {
		struct {
			float r; // Radius
			float h; // Height
			float h_jump; // Led vertical spacing
			float adiv; // Circumference division
		} cr;
		struct {
			int turns; // Number of revolutions
			float r; // Extentricity of leds
			float h; // Height
			float h_jump; // Led vertical spacing
			float a; // Radius of inner circle
			float b; // Radius of outer circle
		} epi;
	};
} emu;

/** Display function called to redraw scene **/
void display() {
	// Init
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glPointSize(2.0);

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

	// Points
	switch (emu.shapemode) {
		case ShapeMode::CYLINDER_RADIAL:
			glBegin(GL_POINTS);
			glColor3d(1, 0, 0);
			for (float h = 0; h <= emu.cr.h; h += emu.cr.h_jump) {
				for (float a = 0; a < 360 * ani; a += 360.0/emu.cr.adiv) {
					for (auto &r : emu.leds) {
						glVertex3d(
							r * emu.cr.r * cos(a * M_PI / 180),
							r * emu.cr.r * sin(a * M_PI / 180),
							h
						);
					}
				}
			}
			glEnd();
			break;

		case ShapeMode::EPITROCHOID:
			glBegin(GL_POINTS);
			for (float h = 0; h <= emu.epi.h; h += emu.epi.h_jump) {
				glColor3d(0, 0, 1);
				float t = 2 * M_PI * emu.epi.turns * ani;
				glVertex3d(
					(emu.epi.a + emu.epi.r + 1) * cos(t),
					(emu.epi.a + emu.epi.r + 1) * sin(t),
					0
				);
				for (float t = 0; t < 2 * M_PI * emu.epi.turns * ani; t += M_PI / 100) {
					for (auto &r : emu.leds) {
						float r2 = emu.epi.a + emu.epi.b;
						glColor3d(1, 0, 0);
						glVertex3d(
							r2 * cos(t) - emu.epi.r * r * cos((r2/emu.epi.b) * t),
							r2 * sin(t) - emu.epi.r * r * sin((r2/emu.epi.b) * t),
							h
						);
					}
				}
			}
			glEnd();
			break;
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
	if (key == 27)
		exit(0);
	else if (key == 32)
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
		ani += 0.01;
		if (ani > 1)
			ani = 0;
	}
	glutPostRedisplay();
}

/** Parse options **/
int parse_options(int argc, char * argv[]) {
	// Generic parameters
	int c;
	int option_index = 0;
	emu.shapemode = ShapeMode::UNDEFINED;
	emu.animated = false;
	struct option generic_options[] = {
		{"mode", required_argument, 0, 0x00},
		{"animated", no_argument, 0, 0x01},
		{"width", required_argument, 0, 0x02},
		{"height", required_argument, 0, 0x03},
		{"turns", required_argument, 0, 0x04},

		{"r", required_argument, 0, 0x10},
		{"h", required_argument, 0, 0x11},
		{"h_jump", required_argument, 0, 0x12},
		{"adiv", required_argument, 0, 0x13},
		{"a", required_argument, 0, 0x14},
		{"b", required_argument, 0, 0x15},

		{"led", required_argument, 0, 'l'},

		{0, 0, 0, 0}
	};
	while((c = getopt_long(argc, argv, "l:", generic_options, &option_index)) != -1) {
		unsigned long optvalul = strtoul(optarg, NULL, 10);
		float optvalf = strtof(optarg, NULL);
		if (c == 0x00) {
			if (optvalul <= SHAPE_MODE_MAX) {
				emu.shapemode = (ShapeMode) optvalul;
				emu.leds.push_back(1.0f);
				if (emu.shapemode == ShapeMode::CYLINDER_RADIAL) {
					emu.cr.h = 8;
					emu.cr.r = 8;
					emu.cr.adiv = 50;
					emu.cr.h_jump = 1;

				} else if (emu.shapemode == ShapeMode::EPITROCHOID) {
					emu.epi.turns = 5;
					emu.epi.b = 1.0;
					emu.epi.a = 1.4;
					emu.epi.r = 2.4;
					emu.epi.h = 8;
					emu.epi.h_jump = 0.1;
				}
			} else {
				std::cerr << "Unknown mode" << std::endl;
				return 1;
			}

		} else if (c == 0x01) {
			emu.animated = true;

		} else if (c == 0x02 && optvalul) {
			screen.width = optvalul;

		} else if (c == 0x03 && optvalul) {
			screen.height = optvalul;

		} else if (c == 0x04) {
			if (emu.shapemode == ShapeMode::EPITROCHOID)
				emu.epi.turns = optvalul;

		} else if (c == 0x10) {
			if (emu.shapemode == ShapeMode::CYLINDER_RADIAL)
				emu.cr.r = optvalul;
			else if (emu.shapemode == ShapeMode::EPITROCHOID)
				emu.epi.r = optvalf;

		} else if (c == 0x11) {
			if (emu.shapemode == ShapeMode::CYLINDER_RADIAL)
				emu.cr.h = optvalul;
			else if (emu.shapemode == ShapeMode::EPITROCHOID)
				emu.epi.h = optvalf;

		} else if (c == 0x12) {
			if (emu.shapemode == ShapeMode::CYLINDER_RADIAL)
				emu.cr.h_jump = optvalf;
			else if (emu.shapemode == ShapeMode::EPITROCHOID)
				emu.epi.h_jump = optvalf;

		} else if (c == 0x13) {
			if (emu.shapemode == ShapeMode::CYLINDER_RADIAL)
				emu.cr.adiv = optvalul;

		} else if (c == 0x14) {
			if (emu.shapemode == ShapeMode::EPITROCHOID)
				emu.epi.a = optvalf;

		} else if (c == 0x15) {
			if (emu.shapemode == ShapeMode::EPITROCHOID)
				emu.epi.b = optvalf;

		} else if (c == 'l') {
			emu.leds.push_back(optvalf);
		}
	}

	return 0;
}

/** Main function used as entry point **/
int main(int argc, char * argv[]) {
	// Command line options
	int parsing = parse_options(argc, argv);
	if (parsing)
		return parsing;

	// Init glut
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(screen.width, screen.height);
	glutCreateWindow("HAUM AsiPOV");

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
