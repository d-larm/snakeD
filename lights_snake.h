
#ifndef LIGHTS_MATERIAL_H
#define LIGHTS_MATERIAL_H

// some magic to set up lights and smooth shading
void init_lights(const GLenum shade_model=GL_FLAT)
{
	//Positions of the four lights 
	float light0_position[] = {levelWidth*2, 0.75* planeDistance, levelLength*2, 0.5};
	float light1_position[] = {levelWidth*2, 0.75* planeDistance, -levelLength, 0.5};
	float light2_position[] = {-levelWidth, 0.75* planeDistance, levelLength*2, 0.5};
	float light3_position[] = {-levelWidth, 0.75* planeDistance, -levelLength, 0.5};

	//The diffuse and specular components for all the lights
	float light_diffuse[] = {1.0f, 1.0f, 1.0f, 1.0};
	float light1_specular[] = {0.7f, 0.2f, 0.6f, 1.0};
	float light2_specular[] = {1.0f, 1.0f, 1.0f, 1.0};
	float light3_specular[] = {0.3f, 0.1f, 0.4f, 1.0};
	float light_specular[] = {0.6, 0.6, 0.6, 1.0};

	//Diffuse and specular components of the lights set for all four lights
	glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
	glLightfv(GL_LIGHT0, GL_POSITION, light0_position);

	glLightfv(GL_LIGHT1, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT1, GL_POSITION, light1_position);
	glLightfv(GL_LIGHT1, GL_SPECULAR, light1_specular);

	glLightfv(GL_LIGHT2, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT2, GL_POSITION, light2_position);
	glLightfv(GL_LIGHT2, GL_SPECULAR, light2_specular);

	glLightfv(GL_LIGHT3, GL_DIFFUSE, light_diffuse);
	glLightfv(GL_LIGHT3, GL_POSITION, light3_position);
	glLightfv(GL_LIGHT3, GL_SPECULAR, light3_specular);


    glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 0); //Sets the light model

	glFrontFace(GL_CW);

	 //Enable all the lights
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_LIGHT1);
	glEnable(GL_LIGHT2);
	glEnable(GL_LIGHT3);

    glEnable(GL_AUTO_NORMAL);
    glEnable(GL_NORMALIZE);
	glEnable(GL_DEPTH_TEST);


	glShadeModel(shade_model); // GL_FLAT, GL_SMOOTH

	//glEnable(GL_CULL_FACE);
	//glCullFace(GL_BACK);
}


#endif
