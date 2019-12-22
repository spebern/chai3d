#include <random>
#include "chai3d.h"
#include <GLFW/glfw3.h>
#include "Network.h"
#include "Master.h"
#include "Slave.h"
#include "chrono"
#include "Config.h"
#include "haptic_db_ffi.h"
#include "vector"
#include "CompareController.h"
#include "Controller.h"
#include "thread"
#include "Environment.h"


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

// a haptic device handler
cHapticDeviceHandler* handler;

// a pointer to the current haptic device
cGenericHapticDevicePtr hapticDevice;

// a flag to indicate if the haptic simulation currently running
std::atomic<bool> simulationRunning = true;

// haptic thread
cThread* hapticsThread;

// a handle to window display context
GLFWwindow* window = nullptr;

// the network model used for passing messages between master and slave
Network* network;

// the configuration of master and slave (e.g. control algorithm)
Config* config;

// master environment interacting with a haptic device
Master* master;

// database to save session data and messages between slave and master
DB* db;

// all controllers
vector<Controller*> controllers;

// iterator for the controllers
vector<Controller*>::iterator controllerIt;

// the current controller
Controller* controller;

// remote environment controlled by the master
Slave* slave;

// environment containing all the animations
Environment* env;

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

// this function closes the application
void close();

cMesh* createWall();

void runSlave(const std::chrono::microseconds interval)
{
	while (simulationRunning.load())
	{
		auto now = std::chrono::system_clock::now();
		slave->spin();
		auto d = std::chrono::system_clock::now() - now;
		std::this_thread::sleep_for(interval - d);
	}
}

void runMaster(const std::chrono::microseconds interval)
{
	// ignore the first couple of measurements
	for (auto i = 0; i < 300; i++)
	{
		cVector3d ignore;
		hapticDevice->getPosition(ignore);
		hapticDevice->getLinearVelocity(ignore);
	}

	while (simulationRunning.load())
	{
		auto now = std::chrono::system_clock::now();
		master->spin();
		auto d = std::chrono::system_clock::now() - now;
		std::this_thread::sleep_for(interval - d);
	}
}

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
	// random seed
	std::srand(std::time(nullptr));

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

	db = db_new(4);

	const auto nickname = readNickname();
	const auto age = readAge();
	const auto gender = readGender();
	const auto handedness = readHandedness();
	db_new_session(
		db, 
		nickname.c_str(), 
		age, 
		gender, 
		handedness
	);

	initOpenGL();

	env = new Environment(width, height, db);

	const std::chrono::microseconds delay(0);
	config = new Config();
	network = new Network(delay, 0.0);
	master = new Master(network, hapticDevice, config, db, env);
	slave = new Slave(network, config, db,info.m_maxLinearStiffness, env);

	vector<TrialConfig> trials;
	trials.push_back(
		TrialConfig {
			OptimisationParameter::Delay,
			ControlAlgorithm::ISS,
			0,
			400
		}
	);
	trials.push_back(
		TrialConfig{
			OptimisationParameter::Delay,
			ControlAlgorithm::WAVE,
			0,
			400
		}
	);
	trials.push_back(
		TrialConfig{
			OptimisationParameter::PacketRate,
			ControlAlgorithm::ISS,
			0,
			0
		}
	);
	trials.push_back(
		TrialConfig{
			OptimisationParameter::PacketRate,
			ControlAlgorithm::WAVE,
			0,
			0
		}
	);

	random_shuffle(trials.begin(), trials.end());

	controllers.push_back(
		new CompareController(slave, master, config, network, db, env, trials)
	);

	controllerIt = controllers.begin();
	controller = *controllerIt;

	controller->init();

	// create a thread which starts the main haptics rendering loop
	const auto interval = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::milliseconds(1));
	std::thread masterThread(runMaster, interval);
	std::thread slaveThread(runSlave, interval);
	slaveThread.detach();
	masterThread.detach();

	atexit(close);

	windowSizeCallback(window, width, height);

	// main graphic loop
	while (!glfwWindowShouldClose(window))
	{
		glfwGetWindowSize(window, &width, &height);

		env->updateGraphics();

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
    if ((action != GLFW_PRESS) && (action != GLFW_REPEAT))
        return;

	switch (key)
	{
	case 83: // s
	case KEY_DOWN:
		controller->downKey();
		break;
	case 87: //  w
	case KEY_UP:
		controller->upKey();
		break;
	case 65: //  a
	case KEY_LEFT:
		controller->leftKey();
		break;
	case 68: //  d
	case KEY_RIGHT:
		controller->rightKey();
		break;
	case 257: // enter
	{
		const auto done = controller->saveToDb();
		if (done)
		{
			++controllerIt;
			if (controllerIt == controllers.end())
				exit(0);
			controller = *controllerIt;
			controller->init();
		}
		break;
	}
	case 86: // 'v'
		controller->toggleConfig();
		break;
	case 82: // '3'
		controller->toggleReference();
		break;
	case '1':
		controller->rate(1);
		break;
	case '2':
		controller->rate(2);
		break;
	case '3':
		controller->rate(3);
		break;
	case '4':
		controller->rate(4);
		break;
	case '5':
		controller->rate(5);
		break;
	case 67: // 'c'
		env->showTargetChallenge(config->springIdx());
		break;
	case 32: // space
	{
		const auto forceFeedback = config->forceFeedback();
		config->forceFeedback(!forceFeedback);
		break;
	}
	default: ;
		std::cout << "unused key " << key << std::endl;
	}
}

void close(void)
{
	// stop the simulation
	simulationRunning.store(false);

	// wait for graphics and haptics loops to terminate
	cSleepMs(1000);

	hapticDevice->close();

	// delete resources
	delete hapticsThread;
	delete handler;
	db_free(db);
}
