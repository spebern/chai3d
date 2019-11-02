#include "chai3d.h"
#include <GLFW/glfw3.h>
#include "Network.h"
#include "Spring.h"
#include "Master.h"
#include "Slave.h"
#include "chrono"
#include "Config.h"

using namespace chai3d;
using namespace std;

//------------------------------------------------------------------------------
// GENERAL SETTINGS
//------------------------------------------------------------------------------

// stereo Mode
/*
    C_STEREO_DISABLED:            Stereo is disabled 
    C_STEREO_ACTIVE:              Active stereo for OpenGL NVDIA QUADRO cards
    C_STEREO_PASSIVE_LEFT_RIGHT:  Passive stereo where L/R images are rendered next to each other
    C_STEREO_PASSIVE_TOP_BOTTOM:  Passive stereo where L/R images are rendered above each other
*/
cStereoMode stereoMode = C_STEREO_DISABLED;

// fullscreen mode
bool fullscreen = false;

// mirrored display
bool mirroredDisplay = false;

//------------------------------------------------------------------------------
// DECLARED VARIABLES
//------------------------------------------------------------------------------

#define KEY_DOWN  264
#define KEY_UP    265
#define KEY_LEFT  263
#define KEY_RIGHT 262

// a world that contains all objects of the virtual environment
cWorld* world;

// a camera to render the world in the window display
cCamera* camera;

// a light source to illuminate the objects in the world
cDirectionalLight* light;

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the current haptic device
cGenericHapticDevicePtr hapticDevice;

// a flag to indicate if the haptic simulation currently running
bool simulationRunning = false;

// a flag to indicate if the haptic simulation has terminated
bool simulationFinished = true;

// haptic thread
cThread* hapticsThread;

// a handle to window display context
GLFWwindow* window = nullptr;

// a spring the haptic device interacts with
Spring* spring;

// the network model used for passing messages between master and slave
Network* network;

// the configuration of master and slave (e.g. control algorithm)
Config* config;

// a wall that fixes the spring
cMesh* wall;

// master environment interacting with a haptic device
Master* master;

// remote environment controlled by the master
Slave* slave;

// current width of window
int width = 0;

// current height of window
int height = 0;

// swap interval for the display context (vertical synchronization)
int swapInterval = 1;


//------------------------------------------------------------------------------
// DECLARED FUNCTIONS
//------------------------------------------------------------------------------

// callback when the window display is resized
void windowSizeCallback(GLFWwindow* window, int width, int height);

// callback when an error GLFW occurs
void errorCallback(int error, const char* description);

// callback when a key is pressed
void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);

// this function renders the scene
void updateGraphics();

// this function contains the main haptics simulation loop
void updateHaptics();

// this function closes the application
void close();

cMesh* createSpringMesh(double size, double height);

cMesh* createWall();

void initHapticDevice()
{
	handler = new cHapticDeviceHandler();
	handler->getDevice(hapticDevice, 0);
	hapticDevice->open();
	hapticDevice->calibrate();
}

void initOpenGL()
{
	if (!glfwInit())
	{
		cout << "failed initialization" << endl;
		cSleepMs(1000);
		exit(1);
	}

	glfwSetErrorCallback(errorCallback);

	// compute desired size of window
	const auto mode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	const int w = mode->height * 0.8;
	const int h = mode->height * 0.5;
	const int x = (mode->width - w) * 0.5;
	const int y = (mode->height - h) * 0.5;

	// set OpenGL version
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);

	if (stereoMode == C_STEREO_ACTIVE)
	{
		glfwWindowHint(GLFW_STEREO, GL_TRUE);
	}
	else
	{
		glfwWindowHint(GLFW_STEREO, GL_FALSE);
	}

	// create display context
	window = glfwCreateWindow(w, h, "CHAI3D", nullptr, nullptr);
	if (!window)
	{
		cout << "failed to create window" << endl;
		cSleepMs(1000);
		glfwTerminate();
		exit(1);
	}

	glfwGetWindowSize(window, &width, &height);
	glfwSetWindowPos(window, x, y);

	glfwSetKeyCallback(window, keyCallback);

	glfwSetWindowSizeCallback(window, windowSizeCallback);

	glfwMakeContextCurrent(window);

	glfwSwapInterval(swapInterval);

#ifdef GLEW_VERSION
	if (glewInit() != GLEW_OK)
	{
		cout << "failed to initialize GLEW library" << endl;
		glfwTerminate();
		exit(1);
	}
#endif
}

void initWorld()
{
	world = new cWorld();

	world->setBackgroundColor(0.5, 0.5, 0.5);

	world->m_backgroundColor.setBlack();

	camera = new cCamera(world);
	world->addChild(camera);

	camera->set(cVector3d(0.5, 0.0, 0.0), // camera position (eye)
	            cVector3d(0.0, 0.0, 0.0), // look at position (target)
	            cVector3d(0.0, 0.0, 1.0)); // direction of the (up) vector

	camera->setClippingPlanes(0.01, 10.0);

	camera->setStereoMode(stereoMode);

	camera->setStereoEyeSeparation(0.01);
	camera->setStereoFocalLength(0.5);

	camera->setMirrorVertical(mirroredDisplay);

	light = new cDirectionalLight(world);
	world->addChild(light);
	light->setEnabled(true);

	light->setDir(-1.0, 0.0, 0.0);

	cMesh* springMesh = createSpringMesh(0.05, 0.085);
	world->addChild(springMesh);
	spring = new Spring(springMesh);

	wall = createWall();
	world->addChild(wall);
	wall->setLocalPos(0, 0, 0);
}

int main(int argc, char* argv[])
{
	cout << endl;
	cout << "-----------------------------------" << endl;
	cout << "CHAI3D" << endl;
	cout << "Demo: 32-spring" << endl;
	cout << "Copyright 2003-2016" << endl;
	cout << "-----------------------------------" << endl << endl << endl;
	cout << "Keyboard Options:" << endl << endl;
	cout << "[1] - Enable/Disable potential field" << endl;
	cout << "[2] - Enable/Disable damping" << endl;
	cout << "[f] - Enable/Disable full screen mode" << endl;
	cout << "[m] - Enable/Disable vertical mirroring" << endl;
	cout << "[q] - Exit application" << endl;
	cout << endl << endl;

	initHapticDevice();

	const auto info = hapticDevice->getSpecifications();

	initOpenGL();

	initWorld();

	std::chrono::microseconds delay(0);
	std::chrono::microseconds varDelay(0);
	config = new Config();
	network = new Network(delay, varDelay);
	master = new Master(network, hapticDevice, config);
	slave = new Slave(network, spring, config);

	// create a thread which starts the main haptics rendering loop
	hapticsThread = new cThread();
	hapticsThread->start(updateHaptics, CTHREAD_PRIORITY_HAPTICS);

	atexit(close);

	windowSizeCallback(window, width, height);

	// main graphic loop
	while (!glfwWindowShouldClose(window))
	{
		glfwGetWindowSize(window, &width, &height);

		updateGraphics();

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	glfwDestroyWindow(window);

	glfwTerminate();

	return 0;
}

void windowSizeCallback(GLFWwindow* window, const int newWidth, int newHeight)
{
	width = newWidth;
	height = newHeight;
}

void errorCallback(int error, const char* description)
{
	cout << "Error: " << description << endl;
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	switch (key)
	{
	case KEY_DOWN:
		master->decreasePacketRate(10.0);
		slave->decreasePacketRate(10.0);
		break;
	case KEY_UP:
		master->increasePacketRate(10.0);
		slave->increasePacketRate(10.0);
		break;
	case KEY_LEFT:
		network->decreaseDelay(chrono::microseconds(1000));
		break;
	case KEY_RIGHT:
		network->increaseDelay(chrono::microseconds(1000));
		break;
	default: ;
	}
}

void close(void)
{
	// stop the simulation
	simulationRunning = false;

	// wait for graphics and haptics loops to terminate
	while (!simulationFinished) { cSleepMs(100); }

	hapticDevice->close();

	// delete resources
	delete hapticsThread;
	delete world;
	delete handler;
}

cMesh* createSpringMesh(const double size, const double height)
{
	auto mesh = new cMesh();
	const auto base = 0.0;
	const auto halfSize = size / 2.0;
	int vertices[6][6];

	// face -x
	vertices[0][0] = mesh->newVertex(-halfSize, base, halfSize);
	vertices[0][1] = mesh->newVertex(-halfSize, base, -halfSize);
	vertices[0][2] = mesh->newVertex(-halfSize, -height, -halfSize);
	vertices[0][3] = mesh->newVertex(-halfSize, -height, halfSize);

	// face +x
	vertices[1][0] = mesh->newVertex(halfSize, base, -halfSize);
	vertices[1][1] = mesh->newVertex(halfSize, base, halfSize);
	vertices[1][2] = mesh->newVertex(halfSize, -height, halfSize);
	vertices[1][3] = mesh->newVertex(halfSize, -height, -halfSize);

	// face -y
	vertices[2][0] = mesh->newVertex(-halfSize, -height, halfSize);
	vertices[2][1] = mesh->newVertex(-halfSize, -height, -halfSize);
	vertices[2][2] = mesh->newVertex(halfSize, -height, -halfSize);
	vertices[2][3] = mesh->newVertex(halfSize, -height, halfSize);

	// face +y
	vertices[3][0] = mesh->newVertex(halfSize, base, halfSize);
	vertices[3][1] = mesh->newVertex(halfSize, base, -halfSize);
	vertices[3][2] = mesh->newVertex(-halfSize, base, halfSize);
	vertices[3][3] = mesh->newVertex(-halfSize, base, -halfSize);

	// face -z
	vertices[4][0] = mesh->newVertex(halfSize, base, -halfSize);
	vertices[4][1] = mesh->newVertex(-halfSize, base, -halfSize);
	vertices[4][2] = mesh->newVertex(halfSize, -height, -halfSize);
	vertices[4][3] = mesh->newVertex(-halfSize, -height, -halfSize);

	// face +z
	vertices[5][0] = mesh->newVertex(halfSize, base, halfSize);
	vertices[5][1] = mesh->newVertex(-halfSize, base, halfSize);
	vertices[5][2] = mesh->newVertex(halfSize, -height, halfSize);
	vertices[5][3] = mesh->newVertex(-halfSize, -height, halfSize);

	// create triangles
	for (auto& v : vertices)
	{
		mesh->newTriangle(v[0], v[1], v[2]);
		mesh->newTriangle(v[0], v[2], v[3]);
	}

	mesh->m_material->m_ambient.set(0.5, 0.5, 0.5, 1.0);
	mesh->m_material->m_diffuse.set(0.7, 0.7, 0.7, 1.0);
	mesh->m_material->m_specular.set(1.0, 1.0, 1.0, 1.0);
	mesh->m_material->m_emission.set(0.0, 0.0, 0.0, 1.0);
	mesh->m_material->setStiffness(50);
	mesh->m_material->setDynamicFriction(0.8);
	mesh->m_material->setStaticFriction(0.8);

	mesh->setLocalPos(0, 0, 0);

	return mesh;
}

cMesh* createWall()
{
	auto wall = new cMesh();

	cVector3d edges[4];
	edges[0].set(0.00, 0.000, 0.5);
	edges[1].set(0.03, 1.2, 0.5);
	edges[2].set(0.03, 1.2, -0.5);
	edges[3].set(0.03, 0.000, -0.5);

	wall->newTriangle(edges[3], edges[1], edges[0]);
	wall->newTriangle(edges[3], edges[2], edges[1]);

	return wall;
}

void updateGraphics()
{
	world->updateShadowMaps(false, mirroredDisplay);

	camera->renderView(width, height);

	glFinish();

	// check for any OpenGL errors
	GLenum err;
	err = glGetError();
	if (err != GL_NO_ERROR) cout << "Error:  %s\n" << gluErrorString(err);
}

void updateHaptics()
{
	simulationRunning = true;
	simulationFinished = false;

	while (simulationRunning)
	{
		slave->spin();
		master->spin();
	}

	simulationFinished = true;
}
