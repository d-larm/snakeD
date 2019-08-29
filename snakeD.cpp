#ifdef __APPLE__
#include <GLUT/glut.h> 
#else
#include <GL/glut.h> 
#include <unistd.h>
#include <cmath>
#endif
#include <fstream>
#include <iostream> 
#include <string>
#include <chrono>
#include <stdlib.h>
#include <vector>
#include <stddef.h>
#include "draw_text.h" 
#include "load_and_bind_texture.h"

using namespace std;
 
struct snakeBox //Snake element
{
	float x; 
	float y;
	float z;
};

struct obstacle //Obstacle
{ 
	float x;
	float y;
	float z;
};

struct food //Food
{
	float x;
	float y;
	float z;
	float special;
};

//Screen dimensions
float screenW;
float screenH;

//Last key pressed
char lastkey;

//Camera location
float cameraX;
float cameraY;
float cameraZ;
float cameraLerp = 0.1f;

bool canTurn = false; //Flag that determines whether the snake can turn or not for gridless movement


float worldRotation = 0;
int plane = -1; //Represents the current plane (-1 is bottom, 1 is top)
int planeDistance = 25; //Distance between planes
float yOffset = 0; //Distance from the snake y position


float minCameraY = 2;
float maxCameraY = planeDistance-minCameraY;

bool switchingPlanes = false; //Flag for if switching between planes
int topScore = 0; //Top score variable
bool gameOver = false; //Flag for if the player has lost
bool isTurning = false;  //Flag for if the snake is turning
 
const float moveSpeedDefault = 0.1; //Default moving speed of snake
float moveSpeed = moveSpeedDefault;



const int levelWidth = 30;//Determines the dimensions of the world in coordinates
const int levelLength = 30;
//Determines the size of the boundary blocks
const float boundaryBlockSize = 1;
const float levelTileSize = 1 ;   
//Determines the starting location of the world (0,0) is the starting coordinate within the boundary
int startX = 1;
int startZ = 1;
//Determines the size of the snake's cubes
float snakeCubeSize = 1 ;
//Determines the number of cubes attached to the snake
int snakeSize = 0;
int snakeScore = 0;
 //Determines front of the snake in terms of North (0), South (2), East (1) and West (4)
int front = 0;

//Snake location
float snakeX = levelWidth/2;
float snakeZ = levelLength/2;
float snakeY = 0;

float textColor = 1.0f; //Color of text in menu
float foodRadius = 0.8f; //Size of the food
long timer = 0; //Timer for polling input

const int maxObstacles = (levelLength+levelWidth)/2; //Max number of obstacles in the world
vector<obstacle> obstacleList; //List of obstacles
const int maxFood = levelWidth/2; //Max number of food in the world
food foodList[maxFood]; //List of food

vector<unsigned char> inputQueue; //Queue of inputs

vector<snakeBox> snakeBody; //Body of the snake

bool gameStarted = false; //Flag for if the game is running or in the menu 

const float fov = levelWidth+levelLength; //Field of view
const int snakeGrowth = levelWidth*levelLength/(levelWidth+levelLength); //Growth of snake when eating food

//Properties for the different materials
const GLfloat snakeColor[] = {0.3f, 0.1f, 0.3f,1.0f};
const GLfloat snakeColorTrans[] = {0.4f, 0.4f, 0.8f,0.25f};
const GLfloat levelColor[] = {0.0f, 0.0f,0.0f,0.7f};
const GLfloat borderColor[] = {1.0f, 1.0f,1.0f,0.9f};
const GLfloat foodColor[] = {0.8f, 0.2f, 0.3f};
const GLfloat obsColor[] = {0.3f, 0.5f, 0.6f,0.7f};
const GLfloat obsShadowColor[] = {0.15f, 0.15f, 0.15f,0.2f};
const GLfloat mat_specular[] = {0.8,0.8,0.8,1.0};
const GLfloat resetSpecular[] = {0,0,0,0};
const GLfloat mat_shine[] = {25};
const GLfloat resetShine[] = {0};

unsigned int background = 0; //Background texture

#include "lights_snake.h" 

void updateHighScore() { //Updates the top high score and writes it to a file to save it
    fstream bestScore;
    bestScore.open ("score.snkd");
    
    string highscore = "-1";
    if (bestScore.is_open()) {
        getline(bestScore, highscore);
    }
    int lastHighscore=0;
    try {
        lastHighscore = atoi(highscore.c_str());
    }catch (const std::invalid_argument& ia) {}
    if (snakeScore>lastHighscore) {
        ofstream scoreRewrite ("score.snkd");
        scoreRewrite << to_string(snakeScore) << std::endl;
        scoreRewrite.close();
        lastHighscore=snakeScore;
    }
    topScore=lastHighscore;
    
    bestScore.close();
}
    
int random(int a, int b){ //Function which selects randomly a single value out of two given values
   
      int r = rand()%2;
      if(r==0)
            return a;
      else
            return b;
}

int mod(int a,int b){ //Modulus function which does not output negative values
	return ((a%b)+b)%b;
}

float getRatio(){ //Gets the aspect ratio of the screen
	if(screenH == 0)
        screenH = 1;
    return 1.0* screenW / screenH;
}

bool inRange(float val1, float val2, float range){
	//Checks if number is just below or just above integer value due to float error
	float remainder = abs(val1-val2);
	if(remainder <= range)
		return true;
	else
		return false;         
} 

void drawLevel(){ //Draws the world and boundaries
    // glColor3f(0.96f,0.8f,0.45f);
    // glEnable(GL_LIGHTING);    

    glMaterialfv(GL_FRONT,GL_DIFFUSE, levelColor);
    if(!gameStarted){
    	glMaterialfv(GL_FRONT,GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT,GL_SHININESS, mat_shine);  
    }
    
   
   	glEnable(GL_BLEND); //Enables blending for transparency
	
	glDepthMask ( GL_FALSE ) ; 

	//Draws the level split into multiple quads
	for(float i=0;i<levelWidth;i+=levelTileSize){ 
		for(float j=0;j<levelLength;j+=levelTileSize){
			glPushMatrix();
				glTranslatef(i+levelTileSize/2,0,j+levelTileSize/2);
				glBegin(GL_QUADS); //Draws a quadrilateral as the base of the bottom plane
					glNormal3f(0,1,0);
					glVertex3f(0,0,0); //Bottom left corner
					glVertex3f(levelTileSize,0,0); //Bottom right corner
					glVertex3f(levelTileSize,0,levelTileSize); //Top right corner
					glVertex3f(0,0,levelTileSize); //Top left corner
					
				glEnd(); 
			glPopMatrix();

			glPushMatrix();
				glTranslatef(i+levelTileSize/2,planeDistance,j+levelTileSize/2);
				glBegin(GL_QUADS); //Draws a quadrilateral as the base of the bottom plane
					glNormal3f(0,1,0);
					glVertex3f(0,0,0); //Bottom left corner
					glVertex3f(levelTileSize,0,0); //Bottom right corner
					glVertex3f(levelTileSize,0,levelTileSize); //Top right corner
					glVertex3f(0,0,levelTileSize); //Top left corner

				glEnd(); 
			glPopMatrix();
		}
	} 
	//Disables blending and resets specular properties of materials
	glDepthMask ( GL_TRUE ) ; 
	glMaterialfv(GL_FRONT,GL_SPECULAR, resetSpecular);
	glMaterialfv(GL_FRONT,GL_SHININESS, resetShine);
	glDisable(GL_BLEND);

}

void drawBox(int size,bool texEnable) { //Draws a cube
	if(texEnable)
  		glEnable(GL_TEXTURE_2D);
  //BACK
  glBegin(GL_POLYGON);
  glNormal3f(0,0,-1);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(  0.5*size, -0.5*size, -0.5*size );
  glTexCoord2f(1.0f, 0.0f); glVertex3f(  0.5*size,  0.5*size, -0.5*size );
  glTexCoord2f(1.0f, 1.0f); glVertex3f( -0.5*size,  0.5*size, -0.5*size );
  glTexCoord2f(0.0f, 1.0f); glVertex3f( -0.5*size, -0.5*size, -0.5*size );
  glEnd();

  //FRONT
  glBegin(GL_POLYGON);
  glNormal3f(0,0,1);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(  0.5*size, -0.5*size, 0.5*size );
  glTexCoord2f(1.0f, 0.0f); glVertex3f(  0.5*size,  0.5*size, 0.5*size );
  glTexCoord2f(1.0f, 1.0f); glVertex3f( -0.5*size,  0.5*size, 0.5*size );
  glTexCoord2f(0.0f, 1.0f); glVertex3f( -0.5*size, -0.5*size, 0.5*size );
  glEnd();

  //RIGHT
  glBegin(GL_POLYGON);
  glNormal3f(1,0,0);
  glTexCoord2f(0.0f, 0.0f); glVertex3f( 0.5*size, -0.5*size, -0.5*size );
  glTexCoord2f(1.0f, 0.0f); glVertex3f( 0.5*size,  0.5*size, -0.5*size );
  glTexCoord2f(1.0f, 1.0f); glVertex3f( 0.5*size,  0.5*size,  0.5*size );
  glTexCoord2f(0.0f, 1.0f); glVertex3f( 0.5*size, -0.5*size,  0.5*size );
  glEnd();

  //LEFT
  glBegin(GL_POLYGON);
  glNormal3f(-1,0,0);
  glTexCoord2f(0.0f, 0.0f); glVertex3f( -0.5*size, -0.5*size,  0.5*size );
  glTexCoord2f(1.0f, 0.0f); glVertex3f( -0.5*size,  0.5*size,  0.5*size );
  glTexCoord2f(1.0f, 1.0f); glVertex3f( -0.5*size,  0.5*size, -0.5*size );
  glTexCoord2f(0.0f, 1.0f); glVertex3f( -0.5*size, -0.5*size, -0.5*size );
  glEnd();

  // TOP
  glBegin(GL_POLYGON);
  glNormal3f(0,1,0);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(  0.5*size,  0.5*size,  0.5*size );
  glTexCoord2f(1.0f, 0.0f); glVertex3f(  0.5*size,  0.5*size, -0.5*size );
  glTexCoord2f(1.0f, 1.0f); glVertex3f( -0.5*size,  0.5*size, -0.5*size );
  glTexCoord2f(0.0f, 1.0f); glVertex3f( -0.5*size,  0.5*size,  0.5*size );
  glEnd();

  //BOTTOM
  glBegin(GL_POLYGON);
  glNormal3f(0,1,0);
  glTexCoord2f(0.0f, 0.0f); glVertex3f(  0.5*size, -0.5*size, -0.5*size );
  glTexCoord2f(1.0f, 0.0f); glVertex3f(  0.5*size, -0.5*size,  0.5*size );
  glTexCoord2f(1.0f, 1.0f); glVertex3f( -0.5*size, -0.5*size,  0.5*size );
  glTexCoord2f(0.0f, 1.0f); glVertex3f( -0.5*size, -0.5*size, -0.5*size );
  glEnd();
  glDisable(GL_TEXTURE_2D);
}

void addBody(){
	//Adds snake body element to the snake body array
	snakeBox newBody;
	if(snakeSize == 0)
	{
		newBody = {snakeX,snakeY,snakeZ};
	}
	else
	{
		newBody = {snakeBody[snakeSize-1].x, snakeBody[snakeSize-1].y, snakeBody[snakeSize-1].z};
	}
	snakeBody.push_back(newBody); //Adds body element to the end of the snake body vector
	snakeSize++;
}

void drawLabel(float x,float y, string text){
	glDisable(GL_LIGHTING);
		// Draws the text to the screen
		draw_text(x, y, text.c_str());
	glEnable(GL_LIGHTING);
}

void drawFood(){
	//Draws all the food objects in the world
    glMaterialfv(GL_FRONT,GL_DIFFUSE, foodColor);
	for(int i=0;i<maxFood;i++)
	{
		glPushMatrix();
			glTranslatef(foodList[i].x,foodList[i].y,foodList[i].z);
			//glDisable(GL_LIGHTING);
				glutSolidSphere(foodRadius/3, 50, 50);
			glEnable(GL_LIGHTING);
		glPopMatrix();
	}
}

void drawObstacles(){
	//Draws all the obstacles in the world
    
    glEnable(GL_BLEND); //Enables blending
	glDepthMask ( GL_FALSE ) ; 
	for(int i=0;i<obstacleList.size();i++)
	{

        glMaterialfv(GL_FRONT,GL_DIFFUSE, obsColor); 
		glPushMatrix();
			glTranslatef(obstacleList[i].x,obstacleList[i].y,obstacleList[i].z);
				drawBox(boundaryBlockSize,false);
				glNormal3f(0,plane*-1,0);
			glEnable(GL_LIGHTING);
		glPopMatrix();


		 //Draws the shadow of the obstacle in the opposite plane
		if(obstacleList[i].y > 1)
		{
			glMaterialfv(GL_FRONT,GL_DIFFUSE, obsShadowColor);
			glPushMatrix();
				glTranslatef(obstacleList[i].x - (boundaryBlockSize/2),0.01f,obstacleList[i].z - (boundaryBlockSize/2));
				glBegin(GL_QUADS); // draw lines between pairs of points
					glNormal3f(0,1,0);
					glVertex3f(0,-boundaryBlockSize/2,0); // from endpoint
					glVertex3f(boundaryBlockSize,-boundaryBlockSize/2,0); // to endpoint
					glVertex3f(boundaryBlockSize,-boundaryBlockSize/2,boundaryBlockSize); // from endpoint
					glVertex3f(0,-boundaryBlockSize/2,boundaryBlockSize); // to endpoint
				glEnd();
			glPopMatrix();
		}
		else
		{

			GLfloat k[] = {obsShadowColor[0],obsShadowColor[1],obsShadowColor[2],0.4};  //Less transparent shadows at the top plane for higher visibility
			glMaterialfv(GL_FRONT,GL_DIFFUSE, k);
			glPushMatrix();
				glTranslatef(obstacleList[i].x - (boundaryBlockSize/2),planeDistance-0.01f,obstacleList[i].z - (boundaryBlockSize/2));
				glBegin(GL_QUADS); // draw lines between pairs of points
					glNormal3f(0,1,0);
					glVertex3f(0,boundaryBlockSize/2,0); // from endpoint
					glVertex3f(boundaryBlockSize,boundaryBlockSize/2,0); // to endpoint
					glVertex3f(boundaryBlockSize,boundaryBlockSize/2,boundaryBlockSize); // from endpoint
					glVertex3f(0,boundaryBlockSize/2,boundaryBlockSize); // to endpoint
				glEnd();
			glPopMatrix();
		}
	}
	glDisable(GL_BLEND);
		glDepthMask ( GL_TRUE ) ; 
}

void relocateFood(int n){
	bool validPos;
	do{
		//Find a new random location for the food
		validPos = true;
		foodList[n].x = rand() % (int)(levelWidth-boundaryBlockSize) + startX;
		foodList[n].y = random(0,planeDistance);
		foodList[n].z = rand() % (int)(levelLength-boundaryBlockSize) + startZ;
		for(int i=0;i<obstacleList.size();i++)
			if(obstacleList[i].x == foodList[n].x  && obstacleList[i].y == foodList[n].y && obstacleList[i].z == foodList[n].z){ //Checks if food is in obstacle position
				validPos = false;
				break;
			}
	}while(validPos == false);
}

void initialiseFood(){
	//Initialises the positions of all food in the world
	for(int n=0;n<maxFood;n++)
	{
		relocateFood(n);
	}
}

void generateFood(){
	//Generates food at random locations if the snake eats it
	
	float precision = foodRadius/2;
	for(int n=0;n<maxFood;n++)
	{
		//If snake has eaten food
		if(inRange(snakeX,foodList[n].x,precision) && inRange(foodList[n].z,snakeZ,precision) && (int)snakeY == (int)foodList[n].y) 
		{
			snakeScore++;
			for(int i=0;i<snakeGrowth;i++) //Increases size of the snake
				addBody();
			relocateFood(n);
		}
		else 
		{
			//Check if food is located within body of snake
			for(int i=0;i<snakeSize;i++)
			{
				if(inRange(snakeBody[i].x,foodList[n].x,precision) && inRange(foodList[n].z,snakeBody[i].z,precision) && (int)snakeY == (int)foodList[n].y)  //If food spawns within the body of the snake
				{
					snakeScore++;
					for(int i=0;i<snakeGrowth;i++) //Increases size of snake
						addBody();
					relocateFood(n);
				}
			}
		}
	}
}

void generateObstacles(){
	//Initialises the locations of all obstacles in the world
	obstacleList.clear();
	for(int i=0;i<maxObstacles;i++)
	{
		float obsX;
		float obsY;
		float obsZ;
		bool unique;
		do{
			unique = true;
			obsX = (rand() % (int)(levelWidth-5)) + 5;
			obsY = random(0,planeDistance);
			obsZ = (rand() % (int)(levelLength-5)) + 5;

			for(int j=0;j<obstacleList.size();j++)
			{	
				if(j==i)
					continue;
				else
					if(obsX == obstacleList[i].x && obsY == obstacleList[i].y && obsZ == obstacleList[i].z)
					{
						unique = false;
						break;
					}
			}
		}while(unique == false);
		obstacleList.push_back({obsX,obsY,obsZ});
	}
}

void endGame(){
	//Ends the game and restart
	gameStarted = false;
	snakeBody.clear();
	updateHighScore();
}

void controlCamera(){ //Smooths world and camera rotations and translations
	if(gameStarted) //Sets camera to follow snake if the game has started
	{
		float closeness = 1.3;
		if(front == 0)
		{
			cameraX += (snakeX/closeness - cameraX) * cameraLerp;
			cameraZ += (snakeZ - cameraZ) * cameraLerp;
		}
		else if(front == 1)
		{
			cameraX += (snakeX - cameraX) * cameraLerp;
			cameraZ += (snakeZ/closeness - cameraZ) * cameraLerp;
		}
		else if(front == 2)
		{
			cameraX += (snakeX*closeness - cameraX) * cameraLerp;
			cameraZ += (snakeZ - cameraZ) * cameraLerp;
		}
		else if(front == 3)
		{
			cameraX += (snakeX - cameraX) * cameraLerp;
			cameraZ += (snakeZ*closeness - cameraZ) * cameraLerp;
		}
		cameraY += (((planeDistance/2)+(plane*yOffset))  - cameraY) * (cameraLerp/2); //Sets smooth camera position to Y center of planes with the offset
	}
	else //Sets the camera to look at the entire level if the game has not yet begun
	{
		cameraX += (-8*levelWidth/10 - cameraX) * cameraLerp/2;
		cameraZ += (-8*levelWidth/10 - cameraZ) * cameraLerp/2;
		cameraY += (planeDistance/2  - cameraY) * (cameraLerp/4);
	}
}

void checkBoundaries(){
	//Checks if the snake reaches the bounds of the world to wrap around
	if(snakeX > levelWidth){
		snakeX = startX;
		cameraX = 0; //Sets camera to follow
	}
	else if(snakeX < startX){
		snakeX = levelWidth;
		cameraX = levelWidth; //Sets camera to follow
	}

	if(snakeZ > levelLength){
		snakeZ = startZ;
		cameraZ = 0; //Sets camera to follow
	}
	else if(snakeZ < startZ){
		snakeZ = levelLength;
		cameraZ = levelLength; //Sets camera to follow
	}	
}

void checkCollision(){
	checkBoundaries();
	float range = 0;
	// Checks if the snake has collided with itself
	for(int i=snakeGrowth*1.5;i<snakeBody.size();i++)
	{
		if(inRange((int)snakeX,(int)snakeBody[i].x,range) && inRange((int)snakeZ,(int)snakeBody[i].z,range) && (int)snakeY == (int)snakeBody[i].y)
			endGame();
	}
	range = boundaryBlockSize / 1.2f;
	for(int i=0;i<obstacleList.size();i++) //Checks if hitting an obstacle
	{
		if(inRange(snakeX,obstacleList[i].x,range) && inRange(snakeZ,obstacleList[i].z,range) && (int)snakeY == (int)obstacleList[i].y)
			endGame();
	}
}

void drawBody(){
	//Draws the body of the snake onto the world
    glMaterialfv(GL_FRONT,GL_DIFFUSE, snakeColor);
	
	
	for(int i=0;i<snakeSize;i+=1)
	{	
		snakeBox box = snakeBody[i];
		float xval = box.x;
		float zval = box.z; 
		float yval = box.y;
		
		if(yval < planeDistance-0.2 && yval > 0.2){ //Sets transparency if the snake is transitioning between planes
			glEnable(GL_BLEND); //Enables blending
			glDepthMask ( GL_FALSE ) ; 
        	GLfloat k[] = {snakeColor[0]+(i*0.4/snakeSize),snakeColor[1],snakeColor[2]+(i*0.2 /snakeSize),0.25f}; //Sets transparent material with gradient change
        	 glMaterialfv(GL_FRONT,GL_DIFFUSE, k);
        	 
		}
        else{
        	GLfloat k[] = {snakeColor[0]+(i*0.4/snakeSize),snakeColor[1],snakeColor[2]+(i*0.2 /snakeSize),snakeColor[3]}; //Sets snake material color with a gradient
        	 glMaterialfv(GL_FRONT,GL_DIFFUSE, k);
        }

		glPushMatrix();
			glTranslatef((xval),yval,(zval));
			drawBox(snakeCubeSize,false);
		glPopMatrix();

		glDisable(GL_BLEND); //Disables blending
       	glDepthMask(GL_TRUE);
	}
}

void drawSnake(){
	//Draws the snake into the world

	if(snakeY < planeDistance-0.2 && snakeY > 0.2){ //Sets transparency if the snake is transitioning between planes
			glEnable(GL_BLEND); //Enables blending
			glDepthMask ( GL_FALSE ) ; 
        	GLfloat k[] = {snakeColor[0],snakeColor[1],snakeColor[2],0.25f};
        	 glMaterialfv(GL_FRONT,GL_DIFFUSE, k);
        	 
		}
        else{
        	 glMaterialfv(GL_FRONT,GL_DIFFUSE, snakeColor);
        }
	glPushMatrix();
		glTranslatef((snakeX),snakeY,(snakeZ));
		drawBox(snakeCubeSize,false);
		glEnable(GL_LIGHTING);
	glPopMatrix();

	glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);

	drawBody();
}

void moveBody(){
	//Moves body of the snake
	if(snakeSize > 0) // If snake has body
	{	
		snakeBody[0].x = snakeX;
		snakeBody[0].z = snakeZ;
		for(int i=snakeSize-1;i>-1;i--) //Propogates positions of snake body elements up the list
		{
			
			snakeBody[i].x = snakeBody[i-1].x;
			snakeBody[i].y = snakeBody[i-1].y;
			snakeBody[i].z = snakeBody[i-1].z;

			if(i==0)
			{
				snakeBody[i].x = snakeX;
				snakeBody[i].z = snakeZ;
				snakeBody[i].y = snakeY;
			}
		}
	}
}	

void checkSteps(){
	float precision = 0.0001; //Precision of which the fractional position has to be close to a whole number
	float fractX,fractZ, intpart;
	fractX = abs(modf(snakeX,&intpart)); //fractional part of the x position
	fractZ = abs(modf(snakeZ,&intpart)); //fractional part of the z position
	if((fractX < precision || fractX > 1-precision) && (front == 0 || front == 2)) //Checks if the fractional part of the X position is close to any whole number if moving along X axis (North or South)
		canTurn = true;
	else if((fractZ < precision || fractZ > 1-precision) && (front == 1 || front == 3)) //Checks if the fractional part of the Z position is close to any whole number if moving along Z axis (East or West)
		canTurn = true;
	else
		canTurn=false; //Cannot turn if position is not close to a whole number

	if(canTurn == true) //If able to turn
	{
		//Changes snake speed based on score
		if(snakeScore == 25)
			moveSpeed = 0.125f;
		else if(snakeScore == 50)
			moveSpeed = 0.1666666667f;
		else if(snakeScore == 100)
			moveSpeed = 0.2;

		if(inputQueue.size() > 0 && !isTurning) //Checaaks if there are any queued inputs in the input queue
		{
			switch (inputQueue.front()) //Gets the front element of the queue
			{
				case 'q': exit(1); break;
				case 'a': front = mod(front-1,4);break; //Turns snake left and shifts compass
				case 'd':front = mod(front+1,4);break; //Turns snake right and shifts compass
				case ' ': if(switchingPlanes == false){ plane *= -1; switchingPlanes = true;}  break;
			}

				 if(inputQueue.front() == lastkey){ //Prevents the snake from turning into itself backwards if going the same direction multiple times
				 	isTurning = true;
				 	timer = 0;
				 }

		lastkey = inputQueue.front(); 
		inputQueue.erase(inputQueue.begin());//Removes front element of the queue
		}	
	
	}		
}

void updateInput(int i){ //Timer used to poll the input when the same input is spammed
	if(timer%6 == 0 && timer !=0)
		isTurning = false;
	else
		timer++;
	glutTimerFunc(1,updateInput,1);
}

void moveSnake(){
	checkSteps(); //Checks if the snake is changing direction and speed
	moveBody(); //Moves the body of the snake to follow the head
	if(switchingPlanes == true){ //Checks if the snake is transitioning between planes
		if(((int) snakeY < planeDistance && plane == 1) || (snakeY > 0  && plane == -1)) //Checks if the snake has reached the other plane
		{
			snakeY+= plane*moveSpeed*2;
		}
		else //Snake has reached the opposite plane and is no longer transitioning
		{
			switchingPlanes = false;
		}
	}
	else
	{
		if( front % 2 == 0 ){//If facing north or south
			if(front == 0) //If north
			{
				snakeX += moveSpeed; //Move forawrds along x 
			}
			else //If south
			{
				snakeX -= moveSpeed; //Move backwards along x 
			}
		}
		else
			if(front == 1) //If east
			{
				snakeZ += moveSpeed; //Move forawrds along z
			}
			else //If west
			{
				snakeZ -= moveSpeed; //Move backwards along z
			}
	}
	glutPostRedisplay();
}

void changeWorld(){
	controlCamera(); //Controls camera position
    if(gameStarted) //Game loop for when the game has started
    {
		checkCollision();
		generateFood();
		checkSteps();
		moveSnake();
    }
	glutPostRedisplay();
	
}

int dir = 1; //Direction of color change
float textpos = 600; //Position of score value on screen
int textdir; //Direction of score text movement
void display(){
	changeWorld(); //Idle function in display so game pauses when minimised
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glColor3f(1.0,1.0,1.0);
	//Work using the projection matrix to set the orthogonal view
	glMatrixMode(GL_PROJECTION); 
	glLoadIdentity();
	glOrtho(0,screenW,0,screenH,-1,1);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable( GL_TEXTURE_2D );
	glDisable(GL_LIGHTING);
	//Switch to model view to draw the quad across the screen with the background texture
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
    glBindTexture( GL_TEXTURE_2D, background );
    glPushMatrix();
	glBegin (GL_QUADS);
	    glTexCoord2d(0.0,0.0); glVertex2d(0.0,0.0);
	    glTexCoord2d(1.0,0.0); glVertex2d(screenW,0.0);
	    glTexCoord2d(1.0,1.0); glVertex2d(screenW,screenH);
	    glTexCoord2d(0.0,1.0); glVertex2d(0.0,screenH);
    glEnd();
    glPopMatrix();
    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING); 
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);

    //Switches back to projection matrix to set the perspective view
    glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(fov*1.2f, getRatio(), 0.2f, (levelWidth+levelLength)*1.5f);

	//Switches to modelview mode to begin drawing world objects using perspective view
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	
	if(!gameStarted)
	{   
		gluLookAt(cameraX,cameraY, cameraZ, //Positions camera at X,Y,Z which is far from the world
			 levelWidth/2, planeDistance/2, levelLength/2 , //Sets camera reference point to the center of the world
			  0, 1, 0  // Sets up vector to Y axis
			);
		
		//Rotates world in start screen by translating to center of world then rotating and undoing the translation
		glTranslatef(levelWidth/2,planeDistance/2,levelLength/2);
		glRotatef(worldRotation+=0.5, 0.0f, 1.0f, 0.0f);
		glTranslatef(-levelWidth/2,-planeDistance/2,-levelLength/2);
		drawLevel();
		drawObstacles();
		//Labels
		drawLabel(370, 850,"High Score: "+ to_string((int)topScore));
		drawLabel(325,500,"Snake - Dimensions");
		glColor3f(textColor,textColor-0.2f,textColor-0.2f);
		drawLabel(320,450,"Press SPACE to play");
		glColor3f(1.0,1.0,1.0);
		drawLabel(410,150,"Controls");
		drawLabel(50,100,"A/Left, D/Right, SPACE: Switch Planes, ScrollWheel: View");	
		//Makes the "Press SPACE to play" text blink by changing the color
		if(textColor >= 1)
			dir = -1;
		else if(textColor <= 0)
			dir = 1;
		textColor+= dir*0.01f;
	}
	else
	{
		gluLookAt(cameraX,cameraY, cameraZ, //Positions camera at X,Y,Z which is near the snake
			 snakeX, snakeY, snakeZ , //Sets camera reference point to the position of the snake
			  0, 1, 0  //Sets up vector to Y axis
			); 
		drawLevel();
		drawSnake();
		drawFood();
		drawObstacles();	
		string scoreLabel = "Score : ";
		drawLabel(450,screenH-50,scoreLabel);
		if(snakeScore > topScore){ //Draws the score in purple and animates it if high score is beaten
			glColor3f(0.7f,0.2f,0.8f);
			if(textpos >=600)
			dir = -1;
			else if(textpos <= 580)
			dir = 1;
		}
		else{ //Draws score normally if score below high score
			glColor3f(1.0f,1.0f,1.0f);
			textpos = 600;
		}
		textpos+= dir; //Oscillator
		drawLabel(textpos,screenH-50,to_string((int)snakeScore));
	}
    init_lights();  
	glutSwapBuffers(); 
} 


void startGame(){
	timer = 0;
	gameStarted = true;
	moveSpeed = moveSpeedDefault;
	//Initialise snake in center of the world
	snakeX = levelLength/2;
	snakeZ = levelWidth/2;
	//Initialises snake on random plane and sets Y coordinate appropriately
	plane = random(1,-1);
	if(plane == -1)
		snakeY = 0;
	else
		snakeY = planeDistance;
	//Initialises game variables
	front = 0; //Resets to north facing
	snakeSize = 0; //Resets the size of the snake to 0
	worldRotation = 0; //Resets the world rotation to 0
	snakeScore = 0; //Resets snake score to 0

	generateObstacles(); //Generates obstacles
	initialiseFood(); //Intialises food positions
}

void keyboard(unsigned char key, int, int){
	if(!gameStarted && key == ' ') 
		startGame(); //Allows any key press to start the game
	else
		inputQueue.push_back(key); //Adds key press to the input stack

	if(inputQueue.size() > 2)
        		inputQueue.erase(inputQueue.begin());
	glutPostRedisplay();//Refreshes display
}

void mouse(int button, int state, int x, int y){
	switch (button)//Scroll to change camera y position
	{
		case 3: if(planeDistance/2 + yOffset < maxCameraY) yOffset+=1; break;
		case 4: if(planeDistance/2 + yOffset > minCameraY) yOffset-=1; break;
	}
}

void SpecialInput(int key, int x, int y)
{
	switch(key)
        {        
        case GLUT_KEY_LEFT: inputQueue.push_back('a'); break;
        
        case GLUT_KEY_RIGHT: inputQueue.push_back('d');  break;
    }
	 //Removes inputs from the queue if there are more than 2 to prevent spam
	if(inputQueue.size() > 2)
        		inputQueue.erase(inputQueue.begin());

glutPostRedisplay();
}

void reshape(int w, int h){

	screenW = w;
	screenH = h;

    // Use the Projection Matrix
    glMatrixMode(GL_PROJECTION);

        // Reset Matrix
    glLoadIdentity();

    // Set the viewport to be the entire window
    glViewport(0, 0, w, h);

    // Set the correct perspective.
    gluPerspective(fov*1.2f, getRatio(), 0.2f, (levelWidth+levelLength)*1.5f);

    // Get Back to the Modelview
    glMatrixMode(GL_MODELVIEW);
}

void init(){

	glEnable(GL_LINE_SMOOTH); //Enables anti-aliasing of lines
	glHint(GL_POLYGON_SMOOTH_HINT, GL_NICEST);
	// glEnable(GL_POLYGON_SMOOTH); //Enables anti-aliasing of polygons
	glEnable(GL_POINT_SMOOTH);
	srand(time(NULL));
	background = load_and_bind_texture("purple.png");
	glBlendFunc ( GL_SRC_ALPHA ,GL_ONE_MINUS_SRC_ALPHA ) ;
	// star = load_and_bind_texture("star.png");
	updateHighScore();  
}

int main(int argc, char* argv[]){
	glutInit(&argc, argv); 
	glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH); 
	screenW = 1000;
	screenH = 1000;
	glutInitWindowSize(screenW, screenH); 
	glutInitWindowPosition(250, 250); 
	glutCreateWindow("Snake Dimensions"); 
	glutKeyboardFunc(keyboard); 
	glutReshapeFunc(reshape); 
	glutSpecialFunc(SpecialInput);
	glutDisplayFunc(display); 
	glutIdleFunc(NULL);
	glutMouseFunc(mouse);
	glutTimerFunc(500,updateInput,1);
	init(); 
	glutMainLoop(); 
	return 0; 
}
