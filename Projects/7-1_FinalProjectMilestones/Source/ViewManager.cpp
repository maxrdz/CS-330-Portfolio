///////////////////////////////////////////////////////////////////////////////
// viewmanager.cpp
// ============
// manage the viewing of 3D objects within the viewport - camera, projection
//
//  AUTHOR: Brian Battersby - SNHU Instructor / Computer Science
//	Created for CS-330-Computational Graphics and Visualization, Nov. 1st, 2023
///////////////////////////////////////////////////////////////////////////////

#include "ViewManager.h"

// GLM Math Header inclusions
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/type_ptr.hpp>    

// declaration of the global variables and defines
namespace
{
	// Variables for window width and height
	const int WINDOW_WIDTH = 1000;
	const int WINDOW_HEIGHT = 800;
	const char* g_ViewName = "view";
	const char* g_ProjectionName = "projection";

	// camera object used for viewing and interacting with
	// the 3D scene
	Camera* g_pCamera = nullptr;

	// these variables are used for mouse movement processing
	float gLastX = WINDOW_WIDTH / 2.0f;
	float gLastY = WINDOW_HEIGHT / 2.0f;
	bool gFirstMouse = true;

	// time between current frame and last frame
	float gDeltaTime = 0.0f; 
	float gLastFrame = 0.0f;

	// the following variable is false when orthographic projection
	// is off and true when it is on
	bool bOrthographicProjection = false;

	// the position of the camera when orthographic projection is enabled.
	glm::vec3 orthoCamPosition = glm::vec3(0.0f, 0.0f, 25.0f);

	// The last camera position before the user switched to orthographic projection.
	//
	// The ortho view will keep the camera in a set position, so when the user switches
	// back to the standard perspective view, we need to restore the camera position
	// to the coordinate saved here.
	glm::vec3 storedCamPosition = glm::vec3(0.0f, 3.0f, 20.0f);
	float storedCamYaw = -90.0f;
	float storedCamPitch = 0.0f;
}

/***********************************************************
 *  ViewManager()
 *
 *  The constructor for the class
 ***********************************************************/
ViewManager::ViewManager(
	ShaderManager *pShaderManager)
{
	// initialize the member variables
	m_pShaderManager = pShaderManager;
	m_pWindow = NULL;
	g_pCamera = new Camera();
	// default camera view parameters
	g_pCamera->Position = storedCamPosition;
	g_pCamera->Front = glm::vec3(0.0f, 0.0f, -2.0f);
	g_pCamera->Up = glm::vec3(0.0f, 1.0f, 0.0f);
	g_pCamera->Zoom = 80;
	g_pCamera->MovementSpeed = 20;
}

/***********************************************************
 *  ~ViewManager()
 *
 *  The destructor for the class
 ***********************************************************/
ViewManager::~ViewManager()
{
	// free up allocated memory
	m_pShaderManager = NULL;
	m_pWindow = NULL;
	if (NULL != g_pCamera)
	{
		delete g_pCamera;
		g_pCamera = NULL;
	}
}

/***********************************************************
 *  CreateDisplayWindow()
 *
 *  This method is used to create the main display window.
 ***********************************************************/
GLFWwindow* ViewManager::CreateDisplayWindow(const char* windowTitle)
{
	GLFWwindow* window = nullptr;

	// try to create the displayed OpenGL window
	window = glfwCreateWindow(
		WINDOW_WIDTH,
		WINDOW_HEIGHT,
		windowTitle,
		NULL, NULL);
	if (window == NULL)
	{
		std::cout << "Failed to create GLFW window" << std::endl;
		glfwTerminate();
		return NULL;
	}
	glfwMakeContextCurrent(window);

	// tell GLFW to capture all mouse events
	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

	// tell GLFW to call our static method on every scroll event, with new data.
	// this callback is used to adjust camera movement speed
	glfwSetScrollCallback(window, &ViewManager::Mouse_Scroll_Callback);

	// this callback is used to receive mouse moving events
	glfwSetCursorPosCallback(window, &ViewManager::Mouse_Position_Callback);

	// enable blending for supporting tranparent rendering
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	m_pWindow = window;

	return(window);
}

/*
	This static method is called by the GLFW library on every scroll wheel event.
	This is used by the user to slow down or speed up their camera movements.
*/
void ViewManager::Mouse_Scroll_Callback(GLFWwindow* window, double xOffset, double yOffset)
{
	// if we scroll up, speed up.
	if (0 < yOffset) {
		g_pCamera->MovementSpeed += 1;
	}
	// also handle slowing down.
	// and cap the movement speed to 1 as the floor, so we dont stop moving completely (or move inverted?)
	else if ((0 > yOffset) && (g_pCamera->MovementSpeed != 1)) {
		g_pCamera->MovementSpeed -= 1;
	}
}

/***********************************************************
 *  Mouse_Position_Callback()
 *
 *  This method is automatically called from GLFW whenever
 *  the mouse is moved within the active GLFW display window.
 ***********************************************************/
void ViewManager::Mouse_Position_Callback(GLFWwindow* window, double xMousePos, double yMousePos)
{
	// short-circuit all mouse logic when in orthographic view.
	if (bOrthographicProjection == true)
	{
		return;
	}

	// when the first mouse move event is received, this needs to be recorded so that
	// all subsequent mouse moves can correctly calculate the X position offset and Y
	// position offset for proper operation
	if (gFirstMouse)
	{
		gLastX = xMousePos;
		gLastY = yMousePos;
		gFirstMouse = false;
	}

	// calculate the X offset and Y offset values for moving the 3D camera accordingly
	float xOffset = xMousePos - gLastX;
	float yOffset = gLastY - yMousePos; // reversed since y-coordinates go from bottom to top

	// set the current positions into the last position variables
	gLastX = xMousePos;
	gLastY = yMousePos;

	// move the 3D camera according to the calculated offsets
	g_pCamera->ProcessMouseMovement(xOffset, yOffset);
}

/***********************************************************
 *  ProcessKeyboardEvents()
 *
 *  This method is called to process any keyboard events
 *  that may be waiting in the event queue.
 ***********************************************************/
void ViewManager::ProcessKeyboardEvents()
{
	// close the window if the escape key has been pressed
	if (glfwGetKey(m_pWindow, GLFW_KEY_ESCAPE) == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(m_pWindow, true);
	}

	// process camera zooming in and out
	if (glfwGetKey(m_pWindow, GLFW_KEY_W) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(FORWARD, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(BACKWARD, gDeltaTime);
	}

	// process camera panning left and right
	if (glfwGetKey(m_pWindow, GLFW_KEY_A) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(LEFT, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(RIGHT, gDeltaTime);
	}

	// check if Q/E keys are being pressed during this frame.
	// if so, adjust the camera elevation
	if (glfwGetKey(m_pWindow, GLFW_KEY_Q) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(UP, gDeltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_E) == GLFW_PRESS)
	{
		g_pCamera->ProcessKeyboard(DOWN, gDeltaTime);
	}

	// Allow switching between the Perspective / Orthographic
	// projection types for the camera via the P/O keys.
	if (glfwGetKey(m_pWindow, GLFW_KEY_P) == GLFW_PRESS)
	{
		if (bOrthographicProjection == true)
		{
			g_pCamera->Position = storedCamPosition;
			g_pCamera->Yaw = storedCamYaw;
			g_pCamera->Pitch = storedCamPitch;
			// call this method to force an update on camera vectors
			g_pCamera->ProcessMouseMovement(0.0f, 0.0f);
			bOrthographicProjection = false;
		}
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_O) == GLFW_PRESS)
	{
		if (bOrthographicProjection == false)
		{
			// store the last position of the camera before going into ortho view
			storedCamPosition = g_pCamera->Position;
			storedCamYaw = g_pCamera->Yaw;
			storedCamPitch = g_pCamera->Pitch;

			g_pCamera->Position = orthoCamPosition;
			g_pCamera->Yaw = -90.0f;
			g_pCamera->Pitch = 0.0f;
			// call this method to force an update on camera vectors
			g_pCamera->ProcessMouseMovement(0.0f, 0.0f);
			bOrthographicProjection = true;
		}
	}
}

/***********************************************************
 *  PrepareSceneView()
 *
 *  This method is used for preparing the 3D scene by loading
 *  the shapes, textures in memory to support the 3D scene 
 *  rendering
 ***********************************************************/
void ViewManager::PrepareSceneView()
{
	glm::mat4 view;
	glm::mat4 projection;

	// per-frame timing
	float currentFrame = glfwGetTime();
	gDeltaTime = currentFrame - gLastFrame;
	gLastFrame = currentFrame;

	// process any keyboard events that may be waiting in the 
	// event queue
	ProcessKeyboardEvents();

	// get the current view matrix from the camera
	view = g_pCamera->GetViewMatrix();

	// define the current projection matrix
	if (bOrthographicProjection == false) {
		projection = glm::perspective(glm::radians(g_pCamera->Zoom), (GLfloat)WINDOW_WIDTH / (GLfloat)WINDOW_HEIGHT, 0.1f, 100.0f);
	}
	else {
		projection = glm::ortho(-15.0f, 15.0f, -12.0f, 12.0f, -100.0f, 100.0f);
	}
	// if the shader manager object is valid
	if (NULL != m_pShaderManager)
	{
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ViewName, view);
		// set the view matrix into the shader for proper rendering
		m_pShaderManager->setMat4Value(g_ProjectionName, projection);
		// set the view position of the camera into the shader for proper rendering
		m_pShaderManager->setVec3Value("viewPosition", g_pCamera->Position);
		m_pShaderManager->setVec3Value("spotLight.position", g_pCamera->Position);
		m_pShaderManager->setVec3Value("spotLight.direction", g_pCamera->Front);
	}
}