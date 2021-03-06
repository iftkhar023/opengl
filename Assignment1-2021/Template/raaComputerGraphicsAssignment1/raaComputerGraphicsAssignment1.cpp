#include <windows.h>
#include <gl/gl.h>
#include <gl/glu.h>
#include <gl/glut.h>
#include <map>
#include <conio.h>

#include <raaCamera/raaCamera.h>
#include <raaUtilities/raaUtilities.h>
#include <raaMaths/raaMaths.h>
#include <raaMaths/raaVector.h>
#include <raaSystem/raaSystem.h>
#include <raaPajParser/raaPajParser.h>
#include <raaText/raaText.h>

#include "raaConstants.h"
#include "raaParse.h"
#include "raaControl.h"

// NOTES
// look should look through the libraries and additional files I have provided to familarise yourselves with the functionallity and code.
// The data is loaded into the data structure, managed by the linked list library, and defined in the raaSystem library.
// You will need to expand the definitions for raaNode and raaArc in the raaSystem library to include additional attributes for the siumulation process
// If you wish to implement the mouse selection part of the assignment you may find the camProject and camUnProject functions usefull


// core system global data
raaCameraInput g_Input; // structure to hadle input to the camera comming from mouse/keyboard events
raaCamera g_Camera; // structure holding the camera position and orientation attributes
raaSystem g_System; // data structure holding the imported graph of data - you may need to modify and extend this to support your functionallity
raaControl g_Control; // set of flag controls used in my implmentation to retain state of key actions

// global var: parameter name for the file to load
const static char csg_acFileParam[] = {"-input"};

// global var: file to load data from
char g_acFile[256];

//global var to toggle simulation
bool simulation;

static bool show_menu = false;
static float menu_x = 50;
static float menu_y = 50;


// core functions -> reduce to just the ones needed by glut as pointers to functions to fulfill tasks
void display(); // The rendering function. This is called once for each frame and you should put rendering code here
void idle(); // The idle function is called at least once per frame and is where all simulation and operational code should be placed
void reshape(int iWidth, int iHeight); // called each time the window is moved or resived
void keyboard(unsigned char c, int iXPos, int iYPos); // called for each keyboard press with a standard ascii key
void keyboardUp(unsigned char c, int iXPos, int iYPos); // called for each keyboard release with a standard ascii key
void sKeyboard(int iC, int iXPos, int iYPos); // called for each keyboard press with a non ascii key (eg shift)
void sKeyboardUp(int iC, int iXPos, int iYPos); // called for each keyboard release with a non ascii key (eg shift)
void mouse(int iKey, int iEvent, int iXPos, int iYPos); // called for each mouse key event
void motion(int iXPos, int iYPos); // called for each mouse motion event

// Non glut functions
void myInit(); // the myinit function runs once, before rendering starts and should be used for setup
void nodeDisplay(raaNode *pNode); // callled by the display function to draw nodes
void arcDisplay(raaArc *pArc); // called by the display function to draw arcs
void buildGrid(); // 
void nodeSimulation(raaNode *pNode);
void arcSimulation(raaArc *pArc);
void resetforces(raaNode *pNode);

void nodeDisplay(raaNode *pNode) // function to render a node (called from display())
{
	// put your node rendering (ogl) code here
	float* position = pNode->m_afPosition;
	unsigned int continent = pNode->m_uiContinent;

	glPushMatrix();
	glPushAttrib(GL_ALL_ATTRIB_BITS);

	struct vec4
	{
		float x, y, z, w;
	};

	vec4 colors[] =
	{
		{ 0.0f, 1.0f, 0.0f, 1.0f },
		{ 2.0f, 1.0f, 0.0f, 1.0f },
		{ 0.0f, 1.0f, 1.0f, 1.0f },
		{ 2.0f, 0.0f, 2.0f, 1.0f },
		{ 0.0f, 0.0f, 2.0f, 2.0f },
		{ 2.0f, 2.0f, 2.0f, 2.0f }
	};

	//if statement for different continent colours
	utilitiesColourToMat(&colors[pNode->m_uiContinent].x, 2.0f);

	glTranslatef(position[0], position[1], position[2]);

	//if statement to replace solid sphere line, if world system = 1,2... draw glut solid sphere, cone, cube etc
	if (pNode->m_uiWorldSystem == 1)
	{
		glutSolidSphere(mathsRadiusOfSphereFromVolume(pNode->m_fMass), 15, 15);

	}
	else if (pNode->m_uiWorldSystem == 2)
	{
		glRotatef(-90, 1, 0, 0);
		glutSolidCone(mathsRadiusOfConeFromVolume(pNode->m_fMass), 15, 15, 15);
		glRotatef(90, 1, 0, 0);
	}
	else if (pNode->m_uiWorldSystem == 3)
	{
		glutSolidCube(mathsDimensionOfCubeFromVolume(pNode->m_fMass));
	}

	//glTranslatef(0, 25, 0);
	glMultMatrixf(camRotMatInv(g_Camera));
	glScalef(16, 16, 0.1f);
	outlinePrint(pNode->m_acName, true);

	glPopMatrix();
	glPopAttrib();
}

void arcDisplay(raaArc *pArc) // function to render an arc (called from display())
{
	// put your arc rendering (ogl) code here
	raaNode* m_pNode0 = pArc->m_pNode0;
	raaNode* m_pNode1 = pArc->m_pNode1;

	float* arcpos0 = m_pNode0->m_afPosition;
	float* arcpos1 = m_pNode1->m_afPosition;

	glEnable(GL_COLOR_MATERIAL);
	glDisable(GL_LIGHTING);
	glBegin(GL_LINES);
		glColor3f(1.0f, 0.0f, 0.0f);
		glVertex3f(arcpos0[0], arcpos0[1], arcpos0[2]);
		glColor3f(0.0f, 1.0f, 0.0f);
		glVertex3f(arcpos1[0], arcpos1[1], arcpos1[2]);
	glEnd();
}

// draw the scene. Called once per frame and should only deal with scene drawing (not updating the simulator)
void display() 
{
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glClear(GL_DEPTH_BUFFER_BIT|GL_COLOR_BUFFER_BIT); // clear the rendering buffers

	glLoadIdentity(); // clear the current transformation state
	glMultMatrixf(camObjMat(g_Camera)); // apply the current camera transform

	// draw the grid if the control flag for it is true	
	if (controlActive(g_Control, csg_uiControlDrawGrid)) glCallList(gs_uiGridDisplayList);

	glPushAttrib(GL_ALL_ATTRIB_BITS); // push attribute state to enable constrained state changes
	visitNodes(&g_System, nodeDisplay); // loop through all of the nodes and draw them with the nodeDisplay function
	visitArcs(&g_System, arcDisplay); // loop through all of the arcs and draw them with the arcDisplay function
	glPopAttrib();


	// draw a simple sphere
	float afCol[] = {0.3f, 1.0f, 0.5f, 1.0f};
	utilitiesColourToMat(afCol, 2.0f);

	glPushMatrix();
	glTranslatef(0.0f, 30.0f, 0.0f);
	glutSolidSphere(5.0f, 10, 10);
	glPopMatrix();

	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();
	glMatrixMode(GL_PROJECTION);
	glPopMatrix();

	if(show_menu)
	{
		float w = 200;
		float h = 200;

		glDisable(GL_DEPTH_TEST);
		
		glMatrixMode(GL_PROJECTION);
		glPushMatrix();
		glLoadIdentity();

		GLint viewport[4];
		glGetIntegerv(GL_VIEWPORT, viewport);

		glOrtho(0, viewport[2], 0, viewport[3], -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();

		float afCol[] = { 1, 1, 1, 1 };
		utilitiesColourToMat(afCol, 2.0f);

		glTranslatef(menu_x, menu_y, 0);
		glBegin(GL_TRIANGLE_STRIP);
		glVertex2f(0, h);
		glVertex2f(0, 0);
		glVertex2f(w, h);
		glVertex2f(w, 0);
		glEnd();

		glEnable(GL_DEPTH_TEST);

		glMatrixMode(GL_MODELVIEW);
		glPopMatrix();
		glMatrixMode(GL_PROJECTION);
		glPopMatrix();
	}

	

	// glFlush(); // ensure all the ogl instructions have been processed
	glutSwapBuffers(); // present the rendered scene to the screen
}

// processing of system and camera data outside of the renderng loop
void idle() 
{
	if (simulation)
	{
		visitNodes(&g_System, resetforces);
		visitArcs(&g_System, arcSimulation);
		visitNodes(&g_System, nodeSimulation);
	}

	controlChangeResetAll(g_Control); // re-set the update status for all of the control flags
	camProcessInput(g_Input, g_Camera); // update the camera pos/ori based on changes since last render
	camResetViewportChanged(g_Camera); // re-set the camera's viwport changed flag after all events have been processed
	glutPostRedisplay();// ask glut to update the screen	
}

void nodeSimulation(raaNode* pNode)
{
	//calculating acceleration (newtons second law)
	//calculating acceleration
	float acceleration[3] = { 0, 0, 0 };
	acceleration[0] = pNode->force[0] / pNode->m_fMass;
	acceleration[1] = pNode->force[1] / pNode->m_fMass;
	acceleration[2] = pNode->force[2] / pNode->m_fMass;

	//calculating motion using equation
	float time = 0.5f;
	float motion[3] = { 0,0,0 };
	motion[0] = (pNode->velocity[0] * time) * ((acceleration[0] * (time * time)) / 2);
	motion[1] = (pNode->velocity[1] * time) * ((acceleration[1] * (time * time)) / 2);
	motion[2] = (pNode->velocity[2] * time) * ((acceleration[2] * (time * time)) / 2);

	//new positioning
	resetforces(pNode);

	pNode->m_afPosition[0] = pNode->m_afPosition[0] + pNode->displacement[0];
	pNode->m_afPosition[1] = pNode->m_afPosition[1] + pNode->displacement[1];
	pNode->m_afPosition[2] = pNode->m_afPosition[2] + pNode->displacement[2];

	//pNode->m_afPosition[0] = pNode->m_afPosition[0] + pNode->displacement[0];
	//pNode->m_afPosition[1] = pNode->m_afPosition[1] + pNode->displacement[1];
	//pNode->m_afPosition[2] = pNode->m_afPosition[2] + pNode->displacement[2];

	//calculating new velocity
	pNode->velocity[0] = pNode->displacement[0] / time;
	pNode->velocity[1] = pNode->displacement[1] / time;
	pNode->velocity[2] = pNode->displacement[2] / time;

	//apply damping
	float dampingCoef = 0.5f;
	pNode->velocity[0] = pNode->velocity[0] + (1.0 - dampingCoef);
	pNode->velocity[1] = pNode->velocity[1] + (1.0 - dampingCoef);
	pNode->velocity[2] = pNode->velocity[2] + (1.0 - dampingCoef);

	//kinetic energy calculation
	float kineticEngery;
	float scalarVelocity;

	for (int i = 0; i < 4; i++)
	{
		scalarVelocity = pNode->velocity[i] * pNode->velocity[i];
		kineticEngery = pNode->m_fMass * scalarVelocity;
	}
}

void arcSimulation(raaArc* pArc)
{
	//calculating the distance between the vectors
	float vectorDis = vecDistance(pArc->m_pNode0->m_afPosition, pArc->m_pNode1->m_afPosition);

	//calculating the directon of vectors
	float vectorDir[3] = { 0, 0, 0 };
	vecSub(pArc->m_pNode0->m_afPosition, pArc->m_pNode1->m_afPosition, vectorDir);

	//normalised vectors
	float vectorNorm[3] = { 0, 0, 0 };
	vecNormalise(vectorDir, vectorNorm);

	//calculating hookes law
	float extention = vectorDis - pArc->m_fIdealLen;
	float k = 0.02f;

	float hooke = extention * -k;

	//claculating vector force
	float vectorForce[3] = { 0, 0, 0 };
	vecScalarProduct(vectorDir, hooke, vectorForce);

	float negativeVectorForce[3] = { 0, 0, 0 };
	vecScalarProduct(vectorDir, -hooke, negativeVectorForce);

	vecAdd(pArc->m_pNode0->force, vectorForce, pArc->m_pNode0->force);
	vecAdd(pArc->m_pNode1->force, negativeVectorForce, pArc->m_pNode1->force);
}

void resetforces(raaNode* pNode)
{
	pNode->force[0] = 0;
	pNode->force[1] = 0;
	pNode->force[2] = 0;
	pNode->velocity[0] = 0;
	pNode->velocity[1] = 0;
	pNode->velocity[2] = 0;

}

// respond to a change in window position or shape
void reshape(int iWidth, int iHeight)  
{
	glViewport(0, 0, iWidth, iHeight);  // re-size the rendering context to match window
	camSetViewport(g_Camera, 0, 0, iWidth, iHeight); // inform the camera of the new rendering context size
	glMatrixMode(GL_PROJECTION); // switch to the projection matrix stack 
	glLoadIdentity(); // clear the current projection matrix state
	gluPerspective(csg_fCameraViewAngle, ((float)iWidth)/((float)iHeight), csg_fNearClip, csg_fFarClip); // apply new state based on re-sized window
	glMatrixMode(GL_MODELVIEW); // swap back to the model view matrix stac
	glGetFloatv(GL_PROJECTION_MATRIX, g_Camera.m_afProjMat); // get the current projection matrix and sort in the camera model
	glutPostRedisplay(); // ask glut to update the screen
}

// detect key presses and assign them to actions
void keyboard(unsigned char c, int iXPos, int iYPos)
{
	if (c == '\t')
	{
		show_menu = !show_menu;
	}
	if (show_menu) return;

	switch(c)
	{
	case 'w':
		camInputTravel(g_Input, tri_pos); // mouse zoom
		break;
	case 's':
		camInputTravel(g_Input, tri_neg); // mouse zoom
		break;
	case 'c':
		camPrint(g_Camera); // print the camera data to the console
		break;
	case 'g':
		controlToggle(g_Control, csg_uiControlDrawGrid); // toggle the drawing of the grid
		break;
	case 'm': //toggles the simulation to be true or false
		simulation = !simulation;
		break;

	}
}

// detect standard key releases
void keyboardUp(unsigned char c, int iXPos, int iYPos) 
{
	if (show_menu) return;

	switch(c)
	{
		// end the camera zoom action
		case 'w': 
		case 's':
			camInputTravel(g_Input, tri_null);
			break;
	}
}

void sKeyboard(int iC, int iXPos, int iYPos)
{
	if (show_menu) return;

	// detect the pressing of arrow keys for ouse zoom and record the state for processing by the camera
	switch(iC)
	{
		case GLUT_KEY_UP:
			camInputTravel(g_Input, tri_pos);
			break;
		case GLUT_KEY_DOWN:
			camInputTravel(g_Input, tri_neg);
			break;
	}
}

void sKeyboardUp(int iC, int iXPos, int iYPos)
{
	if (show_menu) return;

	// detect when mouse zoom action (arrow keys) has ended
	switch(iC)
	{
		case GLUT_KEY_UP:
		case GLUT_KEY_DOWN:
			camInputTravel(g_Input, tri_null);
			break;
	}
}

void mouse(int iKey, int iEvent, int iXPos, int iYPos)
{
	if (show_menu) return;

	// capture the mouse events for the camera motion and record in the current mouse input state
	if (iKey == GLUT_LEFT_BUTTON)
	{
		camInputMouse(g_Input, (iEvent == GLUT_DOWN) ? true : false);
		if (iEvent == GLUT_DOWN)camInputSetMouseStart(g_Input, iXPos, iYPos);
	}
	else if (iKey == GLUT_MIDDLE_BUTTON)
	{
		camInputMousePan(g_Input, (iEvent == GLUT_DOWN) ? true : false);
		if (iEvent == GLUT_DOWN)camInputSetMouseStart(g_Input, iXPos, iYPos);
	}
}

void motion(int iXPos, int iYPos)
{

	// if mouse is in a mode that tracks motion pass this to the camera model
	if(g_Input.m_bMouse || g_Input.m_bMousePan) camInputSetMouseLast(g_Input, iXPos, iYPos);
}


void myInit()
{
	// setup my event control structure
	controlInit(g_Control);

	// initalise the maths library
	initMaths();

	// Camera setup
	camInit(g_Camera); // initalise the camera model
	camInputInit(g_Input); // initialise the persistant camera input data 
	camInputExplore(g_Input, true); // define the camera navigation mode

	// opengl setup - this is a basic default for all rendering in the render loop
	glClearColor(csg_afColourClear[0], csg_afColourClear[1], csg_afColourClear[2], csg_afColourClear[3]); // set the window background colour
	glEnable(GL_DEPTH_TEST); // enables occusion of rendered primatives in the window
	glEnable(GL_LIGHT0); // switch on the primary light
	glEnable(GL_LIGHTING); // enable lighting calculations to take place
	glEnable(GL_BLEND); // allows transparency and fine lines to be drawn
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // defines a basic transparency blending mode
	glEnable(GL_NORMALIZE); // normalises the normal vectors used for lighting - you may be able to switch this iff (performance gain) is you normalise all normals your self
	glEnable(GL_CULL_FACE); // switch on culling of unseen faces
	glCullFace(GL_BACK); // set culling to not draw the backfaces of primatives

	// build the grid display list - display list are a performance optimization 
	buildGrid();

	// initialise the data system and load the data file
	initSystem(&g_System);
	parse(g_acFile, parseSection, parseNetwork, parseArc, parsePartition, parseVector);
}

int main(int argc, char* argv[])
{
	// check parameters to pull out the path and file name for the data file
	for (int i = 0; i<argc; i++) if (!strcmp(argv[i], csg_acFileParam)) sprintf_s(g_acFile, "%s", argv[++i]);


	if (strlen(g_acFile)) 
	{ 
		// if there is a data file

		glutInit(&argc, (char**)argv); // start glut (opengl window and rendering manager)

		glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA); // define buffers to use in ogl
		glutInitWindowPosition(csg_uiWindowDefinition[csg_uiX], csg_uiWindowDefinition[csg_uiY]);  // set rendering window position
		glutInitWindowSize(csg_uiWindowDefinition[csg_uiWidth], csg_uiWindowDefinition[csg_uiHeight]); // set rendering window size
		glutCreateWindow("raaAssignment1-2017");  // create rendering window and give it a name

		buildFont(); // setup text rendering (use outline print function to render 3D text


		myInit(); // application specific initialisation

		// provide glut with callback functions to enact tasks within the event loop
		glutDisplayFunc(display);
		glutIdleFunc(idle);
		glutReshapeFunc(reshape);
		glutKeyboardFunc(keyboard);
		glutKeyboardUpFunc(keyboardUp);
		glutSpecialFunc(sKeyboard);
		glutSpecialUpFunc(sKeyboardUp);
		glutMouseFunc(mouse);
		glutMotionFunc(motion);
		glutMainLoop(); // start the rendering loop running, this will only ext when the rendering window is closed 

		killFont(); // cleanup the text rendering process

		return 0; // return a null error code to show everything worked
	}
	else
	{
		// if there isn't a data file 

		printf("The data file cannot be found, press any key to exit...\n");
		_getch();
		return 1; // error code
	}
}

void buildGrid()
{
	if (!gs_uiGridDisplayList) gs_uiGridDisplayList= glGenLists(1); // create a display list

	glNewList(gs_uiGridDisplayList, GL_COMPILE); // start recording display list

	glPushAttrib(GL_ALL_ATTRIB_BITS); // push attrib marker
	glDisable(GL_LIGHTING); // switch of lighting to render lines

	glColor4fv(csg_afDisplayListGridColour); // set line colour

	// draw the grid lines
	glBegin(GL_LINES);
	for (int i = (int)csg_fDisplayListGridMin; i <= (int)csg_fDisplayListGridMax; i++)
	{
		glVertex3f(((float)i)*csg_fDisplayListGridSpace, 0.0f, csg_fDisplayListGridMin*csg_fDisplayListGridSpace);
		glVertex3f(((float)i)*csg_fDisplayListGridSpace, 0.0f, csg_fDisplayListGridMax*csg_fDisplayListGridSpace);
		glVertex3f(csg_fDisplayListGridMin*csg_fDisplayListGridSpace, 0.0f, ((float)i)*csg_fDisplayListGridSpace);
		glVertex3f(csg_fDisplayListGridMax*csg_fDisplayListGridSpace, 0.0f, ((float)i)*csg_fDisplayListGridSpace);
	}
	glEnd(); // end line drawing

	glPopAttrib(); // pop attrib marker (undo switching off lighting)

	glEndList(); // finish recording the displaylist
}