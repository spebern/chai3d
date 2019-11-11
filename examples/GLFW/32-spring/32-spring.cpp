#include "chai3d.h"
#include <GLFW/glfw3.h>
#include "Network.h"
#include "Spring.h"
#include "Master.h"
#include "Slave.h"
#include "chrono"
#include "Config.h"
#include "array"
#include "ToolTip.h"
#include "haptic_db_ffi.h"
#include "vector"
#include "TrialController.h"
#include "JNDTrialController.h"

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
cPositionalLight* light;

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

// springs the haptic device interacts with
array<Spring*, 4> springs;

// rating labels displaying the quality of the haptic feedback
array<cLabel*, 4> ratingLabels;

// labels displaying the algorithm used
array<cLabel*, 4> algorithmLabels;

// label that displays the packet rate
cLabel* packetRateLabel;

// label that displays the delay
cLabel* delayLabel;

// position of the device on the slave side
ToolTip* toolTip;

// the network model used for passing messages between master and slave
Network* network;

// the configuration of master and slave (e.g. control algorithm)
Config* config;

// a wall that fixes the spring
cMesh* wall;

// master environment interacting with a haptic device
Master* master;

// database to save session data and messages between slave and master
DB* db;

// controller for the trial (4 springs and their current rating)
TrialController* trialController;

// controller for evaluating jnds of the subject
JNDTrialController* jndTrialController;

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
void keyCallbackJND(GLFWwindow* window, int key, int scancode, int action, int mods);

// callback when a key is pressed
void keyCallbackTrials(GLFWwindow* window, int key, int scancode, int action, int mods);

// this function renders the scene
void updateGraphics();

// this function contains the main haptics simulation loop
void updateHaptics();

// this function closes the application
void close();

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

	glfwSetKeyCallback(window, keyCallbackJND);

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

	cBackground* background = new cBackground();
	camera->m_backLayer->addChild(background);

	light = new cPositionalLight(world);
	world->addChild(light);
	light->setEnabled(true);

	cVector3d springPos(0, -0.05, 0.15);
	cVector3d ratingLabelPos(1100, 870, 0);
	cVector3d algorithmLabelPos(900, 890, 0);
	for (auto i = 0; i < 4; i++)
	{
		springs[i] = new Spring(springPos);
		world->addChild(springs[i]->animation());
		springPos.z(springPos.z() - 0.1);

		ratingLabels[i] = new cLabel(NEW_CFONTCALIBRI28());
		ratingLabels[i]->m_fontColor.setBlack();
		ratingLabels[i]->setFontScale(4.0);
		ratingLabels[i]->setLocalPos(ratingLabelPos);
		ratingLabelPos.y(ratingLabelPos.y() - 260);
		camera->m_frontLayer->addChild(ratingLabels[i]);

		algorithmLabels[i] = new cLabel(NEW_CFONTCALIBRI16());
		algorithmLabels[i]->m_fontColor.setBlack();
		algorithmLabels[i]->setFontScale(4.0);
		algorithmLabels[i]->setLocalPos(algorithmLabelPos);
		algorithmLabelPos.y(algorithmLabelPos.y() - 260);
		camera->m_frontLayer->addChild(algorithmLabels[i]);
	}

	packetRateLabel = new cLabel(NEW_CFONTCALIBRI28());
	packetRateLabel->m_fontColor.setBlack();
	packetRateLabel->setFontScale(4.0);
	packetRateLabel->setLocalPos(900, 0, 0);
	camera->m_frontLayer->addChild(packetRateLabel);

	delayLabel = new cLabel(NEW_CFONTCALIBRI28());
	delayLabel->m_fontColor.setBlack();
	delayLabel->setFontScale(4.0);
	delayLabel->setLocalPos(1300, 0, 0);
	camera->m_frontLayer->addChild(delayLabel);

	wall = createWall();
	world->addChild(wall);
	wall->setLocalPos(0, 0, 0);

	toolTip = new ToolTip();
	world->addChild(toolTip->animation());
}

string readNickname()
{
	string input;
	do
	{
		cout << "please enter your nickname: ";
		cin >> input;
		input = trim(input);
	} while (input.size() == 0 || input.size() > 20);
	return input;
}


Gender readGender()
{
	string input;
	do
	{
		cout << "please enter your gender (male|female): ";
		cin >> input;
		input = trim(input);
	} while (input != "male" && input != "female");
	return input == "male" ? Gender::Male : Gender::Female;
}

Handedness readHandedness()
{
	string input;
	do
	{
		cout << "please enter your handedness (right|left): ";
		cin >> input;
		input = trim(input);
	} while (input != "right" && input != "left");
	return input == "right" ? Handedness::Right : Handedness::Left;
}

int32_t readAge()
{
	auto age = 0;
	do
	{
		cout << "please enter your age: ";
		cin >> age;
	} while (age == 0);
	return age;
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

	db = db_new();

	vector<int32_t> packetRates;
	//packetRates.push_back(10);
	//packetRates.push_back(15);
	//packetRates.push_back(20);
	//packetRates.push_back(25);
	packetRates.push_back(1000);
	//packetRates.push_back(35);
	//packetRates.push_back(40);
	//packetRates.push_back(45);
	//packetRates.push_back(50);
	//packetRates.push_back(60);
	//packetRates.push_back(70);

	const auto nickname = readNickname();
	const auto age = readAge();
	const auto gender = readGender();
	const auto handedness = readHandedness();
	db_new_session(db, nickname.c_str(), age, gender, handedness, packetRates.data(), packetRates.size(), 1);

	initOpenGL();

	initWorld();

	const std::chrono::microseconds delay(0);
	const std::chrono::microseconds varDelay(0);
	config = new Config();
	network = new Network(delay, varDelay);
	master = new Master(network, hapticDevice, config, db);
	slave = new Slave(network, springs[0], config, toolTip, db, info.m_maxLinearStiffness);

	jndTrialController = new JNDTrialController(slave, master, config, network, db, ratingLabels, springs, algorithmLabels);

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

void keyCallbackJND(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if ((action != GLFW_PRESS) && (action != GLFW_REPEAT))
        return;

	switch (key)
	{
	case 83: // s
	case KEY_DOWN:
		jndTrialController->nextSubTrial();
		break;
	case 87: //  w
	case KEY_UP:
		jndTrialController->previousSubTrial();
		break;
		break;
	case 65: //  a
	case KEY_LEFT:
		jndTrialController->decreasePacketRate();
		break;
	case 68: //  d
	case KEY_RIGHT:
		jndTrialController->increasePacketRate();
		break;
	case 257: // enter
	{
		jndTrialController->submitJNDs();
		trialController = new TrialController(slave, master, config, network, db, ratingLabels, springs, algorithmLabels, packetRateLabel, delayLabel);
		glfwSetKeyCallback(window, keyCallbackTrials);
	}
	case 86: // 'v'
		jndTrialController->toggleConfig();
		break;
	case 82: // '3'
		jndTrialController->toggleReference();
		break;
	default: ;
		std::cout << "unused key " << key << std::endl;
	}
}

void keyCallbackTrials(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if ((action != GLFW_PRESS) && (action != GLFW_REPEAT))
        return;

	switch (key)
	{
	case 83: // s
	case KEY_DOWN:
		trialController->nextSubTrial();
		break;
	case 87: //  w
	case KEY_UP:
		trialController->previousSubTrial();
		break;
	case KEY_LEFT:
		network->decreaseDelay(chrono::microseconds(1000));
		std::cout << chrono::duration_cast<chrono::milliseconds>(network->delay()).count() << std::endl;
		break;
	case KEY_RIGHT:
		network->increaseDelay(chrono::microseconds(1000));
		std::cout << chrono::duration_cast<chrono::milliseconds>(network->delay()).count() << std::endl;
		break;
	case '1':
		trialController->rate(1);
		break;
	case '2':
		trialController->rate(2);
		break;
	case '3':
		trialController->rate(3);
		break;
	case '4':
		trialController->rate(4);
		break;
	case '5':
		trialController->rate(5);
		break;
	case 257: // enter
	{
		const auto trialLeft = trialController->submitRatings();
		if (!trialLeft)
			exit(0);
		break;
	}
	case 86: // 'v'
		trialController->toggleConfig();
		break;
	case 82: // '3'
		trialController->toggleReference();
		break;
	default: ;
		std::cout << "unused key " << key << std::endl;
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
	db_free(db);
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

	for (auto i = 0; i < 200; i++)
	{
		cVector3d ignore;
		hapticDevice->getPosition(ignore);
		hapticDevice->getLinearVelocity(ignore);
	}

	while (simulationRunning)
	{
		slave->spin();
		master->spin();
	}

	simulationFinished = true;
}
