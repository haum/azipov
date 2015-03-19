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

/** POV emulation parameters **/
struct {
	bool animated;
	std::vector <float> leds;
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
	emu.animated = false;
	struct option generic_options[] = {
		{"animated", no_argument, 0, 0x01},
		{"width", required_argument, 0, 0x02},
		{"height", required_argument, 0, 0x03},

		{"led", required_argument, 0, 'l'},

		{0, 0, 0, 0}
	};
	while((c = getopt_long(argc, argv, "l:", generic_options, &option_index)) != -1) {
		unsigned long optvalul = strtoul(optarg, NULL, 10);
		float optvalf = strtof(optarg, NULL);
		if (c == 0x01) {
			emu.animated = true;

		} else if (c == 0x02 && optvalul) {
			screen.width = optvalul;

		} else if (c == 0x03 && optvalul) {
			screen.height = optvalul;

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
