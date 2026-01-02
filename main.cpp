#include "Graphics\window.h"
#include "Camera\camera.h"
#include "Shaders\shader.h"
#include "Model Loading\mesh.h"
#include "Model Loading\texture.h"
#include "Model Loading\meshLoaderObj.h"

void processKeyboardInput ();

float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

Window window("Game Engine", 800, 800);
Camera camera;

glm::vec3 lightColor = glm::vec3(1.0f);
glm::vec3 lightPos = glm::vec3(-180.0f, 100.0f, -200.0f);

//s1 enemy
glm::vec3 enemyPos = glm::vec3(-10.0f, 0.0f, 0.0f);// midde irelevent
bool enemyAlive = true;

float distance(glm::vec3 a, glm::vec3 b)
{
	return glm::length(a - b);
}

//s1 vars
int currentTask = 0;
glm::vec3 swordOffset = glm::vec3(5.0f, -5.0f, -10.0f);
float swingTime = 0.0f;


//s1 player and camera
bool firstMouse = true;
float lastX = 400.0f;
float lastY = 400.0f;
float mouseSensitivity = 0.1f;

float playerHeight = 10.0f;   // eye height above ground
//float groundY = -20.0f;

float verticalVelocity = 0.0f;
float gravity = -40.0f;

/*float getGroundHeight(float x, float z)
{
	return 0.0f; // flat ground for now
}*/

//s2 flat ground
float getGroundHeight(float x, float z)
{
	return 0.0f;
}


//s1 mesh hills
Mesh generateTerrain(
	int width,
	int depth,
	float scale,
	std::vector<Texture> textures
)
{
	std::vector<Vertex> vertices;
	std::vector<int> indices;

	// ----- Generate vertices -----
	for (int z = 0; z <= depth; z++)
	{
		for (int x = 0; x <= width; x++)
		{
			Vertex v;

			float worldX = (x - width / 2) * scale;
			float worldZ = (z - depth / 2) * scale;
			float worldY = getGroundHeight(worldX, worldZ);

			v.pos = glm::vec3(worldX, worldY, worldZ);

			v.textureCoords = glm::vec2(
				(float)x / width * 10.0f,
				(float)z / depth * 10.0f
			);

			// Approximate normal using height differences
			float hL = getGroundHeight(worldX - scale, worldZ);
			float hR = getGroundHeight(worldX + scale, worldZ);
			float hD = getGroundHeight(worldX, worldZ - scale);
			float hU = getGroundHeight(worldX, worldZ + scale);

			glm::vec3 normal = glm::vec3(
				hL - hR,
				2.0f,
				hD - hU
			);

			v.normals = glm::normalize(normal);

			vertices.push_back(v);
		}
	}

	// ----- Generate indices -----
	for (int z = 0; z < depth; z++)
	{
		for (int x = 0; x < width; x++)
		{
			int topLeft = z * (width + 1) + x;
			int topRight = topLeft + 1;
			int bottomLeft = (z + 1) * (width + 1) + x;
			int bottomRight = bottomLeft + 1;

			indices.push_back(topLeft);
			indices.push_back(bottomLeft);
			indices.push_back(topRight);

			indices.push_back(topRight);
			indices.push_back(bottomLeft);
			indices.push_back(bottomRight);
		}
	}

	return Mesh(vertices, indices, textures);
}

//s1 skybox
float skyboxVertices[] = {
	-1.0f,  1.0f, -1.0f,
	-1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f, -1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,

	-1.0f, -1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f,
	-1.0f, -1.0f,  1.0f,

	-1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f, -1.0f,
	 1.0f,  1.0f,  1.0f,
	 1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f,  1.0f,
	-1.0f,  1.0f, -1.0f,

	-1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f, -1.0f,
	 1.0f, -1.0f, -1.0f,
	-1.0f, -1.0f,  1.0f,
	 1.0f, -1.0f,  1.0f
};

//s1 pixel loader
unsigned char* loadBMPPixels(const char* imagepath, unsigned int& width, unsigned int& height)
{
	FILE* file = nullptr;
	fopen_s(&file, imagepath, "rb");
	if (!file)
	{
		std::cout << "Image could not be opened: " << imagepath << std::endl;
		return nullptr;
	}

	unsigned char header[54];
	fread(header, 1, 54, file);

	if (header[0] != 'B' || header[1] != 'M')
	{
		std::cout << "Not a BMP file: " << imagepath << std::endl;
		fclose(file);
		return nullptr;
	}

	width = *(int*)&header[0x12];
	height = *(int*)&header[0x16];
	unsigned int dataPos = *(int*)&header[0x0A];

	// BMP rows are padded to multiples of 4 bytes
	unsigned int rowPadded = (width * 3 + 3) & (~3);
	unsigned char* data = new unsigned char[width * height * 3];

	fseek(file, dataPos, SEEK_SET);

	unsigned char* row = new unsigned char[rowPadded];

	for (unsigned int y = 0; y < height; y++)
	{
		fread(row, 1, rowPadded, file);
		memcpy(
			data + (width * 3 * (height - 1 - y)),
			row,
			width * 3
		);
	}

	delete[] row;
	fclose(file);

	return data; // BGR, bottom-up fixed
}



 
GLuint loadCubemap(const std::vector<std::string>& faces)
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned int width = 0, height = 0;
		unsigned char* data = loadBMPPixels(faces[i].c_str(), width, height);

		if (data)
		{
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0,
				GL_RGB,
				width,
				height,
				0,
				GL_BGR, // BMP is typically BGR
				GL_UNSIGNED_BYTE,
				data
			);

			delete[] data;
		}
		else
		{
			std::cout << "Failed to load cubemap face: " << faces[i] << std::endl;
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	return textureID;
}
//s2
struct SwordPart {
	glm::vec3 offset;
	glm::vec3 scale;
	GLuint textureID;
};
std::vector<SwordPart> getSwordParts(
	GLuint bladeTex,
	GLuint guardTex,
	GLuint handleTex,
	GLuint pommelTex
)
{
	std::vector<SwordPart> parts;

	parts.push_back({
		glm::vec3(0.0f, 6.0f, 0.0f),
		glm::vec3(0.4f, 8.0f, 0.2f),
		bladeTex
		});

	parts.push_back({
		glm::vec3(0.0f, 2.0f, 0.0f),
		glm::vec3(2.0f, 0.3f, 0.5f),
		guardTex
		});

	parts.push_back({
		glm::vec3(0.0f, -1.5f, 0.0f),
		glm::vec3(0.3f, 3.0f, 0.3f),
		handleTex
		});

	parts.push_back({
		glm::vec3(0.0f, -3.2f, 0.0f),
		glm::vec3(0.6f, 0.6f, 0.6f),
		pommelTex
		});

	return parts;
}



struct WizardPart {
	glm::vec3 offset;   // relative to enemyPos
	glm::vec3 scale;    // cube scale
};

std::vector<WizardPart> getWizardParts()
{
	std::vector<WizardPart> parts;

	// Hat (purple)
	parts.push_back({ glm::vec3(0.0f, 2.25f, 0.0f), glm::vec3(1.0f, 0.5f, 1.0f) });

	// Head (skin color)
	parts.push_back({ glm::vec3(0.0f, 1.25f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f) });

	// Body (purple)
	parts.push_back({ glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 1.5f, 0.5f) });

	// Left leg (dark purple)
	parts.push_back({ glm::vec3(-0.25f, -1.25f, 0.0f), glm::vec3(0.5f, 1.0f, 0.5f) });

	// Right leg (dark purple)
	parts.push_back({ glm::vec3(0.25f, -1.25f, 0.0f), glm::vec3(0.5f, 1.0f, 0.5f) });

	return parts;
}







////////////////////////////////////////////////////////////////
int main()
{
	glClearColor(0.2f, 0.8f, 1.0f, 1.0f);
	//s1 grav test
	camera.setCameraPosition(glm::vec3(0.0f, 50.0f, 100.0f));

	glfwSetInputMode(window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);


	//building and compiling shader program
	Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
	Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");
	//s1 skybox
	Shader skyboxShader("Shaders/skybox_vertex.glsl", "Shaders/skybox_fragment.glsl");
	//
	skyboxShader.use();
	glUniform1i(glGetUniformLocation(skyboxShader.getId(), "skybox"), 0);
	//

	std::vector<std::string> skyboxFaces =
	{
		"Resources/Skybox/BlueSky/right.bmp",
		"Resources/Skybox/BlueSky/left.bmp",
		"Resources/Skybox/BlueSky/top.bmp",
		"Resources/Skybox/BlueSky/bottom.bmp",
		"Resources/Skybox/BlueSky/front.bmp",
		"Resources/Skybox/BlueSky/back.bmp"
	};
	
	//s1 fix skymap
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	GLuint cubemapTexture = loadCubemap(skyboxFaces);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // RESTORE DEFAULT
	
	//Textures
	GLuint tex = loadBMP("Resources/Textures/wood.bmp");
	GLuint tex2 = loadBMP("Resources/Textures/rock.bmp");
	GLuint tex3 = loadBMP("Resources/Textures/orange.bmp");

	GLuint bladeTex = loadBMP("Resources/Textures/metal.bmp");
	GLuint guardTex = loadBMP("Resources/Textures/metal.bmp");
	GLuint handleTex = loadBMP("Resources/Textures/metal.bmp");
	GLuint pommelTex = loadBMP("Resources/Textures/metal.bmp");


	glEnable(GL_DEPTH_TEST);

	//s1 skybox
	GLuint skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);

	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), skyboxVertices, GL_STATIC_DRAW);

	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	


	//Test custom mesh loading
	std::vector<Vertex> vert;
	vert.push_back(Vertex());
	vert[0].pos = glm::vec3(10.5f, 10.5f, 0.0f);
	vert[0].textureCoords = glm::vec2(1.0f, 1.0f);

	vert.push_back(Vertex());
	vert[1].pos = glm::vec3(10.5f, -10.5f, 0.0f);
	vert[1].textureCoords = glm::vec2(1.0f, 0.0f);

	vert.push_back(Vertex());
	vert[2].pos = glm::vec3(-10.5f, -10.5f, 0.0f);
	vert[2].textureCoords = glm::vec2(0.0f, 0.0f);

	vert.push_back(Vertex());
	vert[3].pos = glm::vec3(-10.5f, 10.5f, 0.0f);
	vert[3].textureCoords = glm::vec2(0.0f, 1.0f);

	vert[0].normals = glm::normalize(glm::cross(vert[1].pos - vert[0].pos, vert[3].pos - vert[0].pos));
	vert[1].normals = glm::normalize(glm::cross(vert[2].pos - vert[1].pos, vert[0].pos - vert[1].pos));
	vert[2].normals = glm::normalize(glm::cross(vert[3].pos - vert[2].pos, vert[1].pos - vert[2].pos));
	vert[3].normals = glm::normalize(glm::cross(vert[0].pos - vert[3].pos, vert[2].pos - vert[3].pos));

	std::vector<int> ind = { 0, 1, 3,   
		1, 2, 3 };

	std::vector<Texture> textures;
	textures.push_back(Texture());
	textures[0].id = tex;
	textures[0].type = "texture_diffuse";

	std::vector<Texture> textures2;
	textures2.push_back(Texture());
	textures2[0].id = tex2;
	textures2[0].type = "texture_diffuse";

	std::vector<Texture> textures3;
	textures3.push_back(Texture());
	textures3[0].id = tex3;
	textures3[0].type = "texture_diffuse";

	//s2 grass texture
	GLuint grassTex = loadBMP("Resources/Textures/grass.bmp");

	std::vector<Texture> grassTextures;
	grassTextures.push_back(Texture());
	grassTextures[0].id = grassTex;
	grassTextures[0].type = "texture_diffuse";





	Mesh mesh(vert, ind, textures3);

	// Create Obj files - easier :)
	// we can add here our textures :)
	MeshLoaderObj loader;
	Mesh sun = loader.loadObj("Resources/Models/sphere.obj");
	Mesh box = loader.loadObj("Resources/Models/cube.obj", textures);
	//Mesh plane = loader.loadObj("Resources/Models/plane.obj", textures3);
	//s1 new plane
	Mesh terrain = generateTerrain(
		120,     // width
		120,     // depth
		5.0f,    // scale
		grassTextures // (we’ll swap this to grass)
	);


	//s1
	Mesh sword = loader.loadObj("Resources/Models/cube.obj", textures);

	//s1 task1 intro
	std::cout << "Task 1: Approach the warlock and press E to attack." << std::endl;

	//check if we close the window or press the escape button
	while (!window.isPressed(GLFW_KEY_ESCAPE) &&
		glfwWindowShouldClose(window.getWindow()) == 0)
	{
		window.clear();
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		processKeyboardInput();

		//s1 read mouise
		double xpos, ypos;
		glfwGetCursorPos(window.getWindow(), &xpos, &ypos);

		if (firstMouse)
		{
			lastX = xpos;
			lastY = ypos;
			firstMouse = false;
		}

		float xoffset = xpos - lastX;
		float yoffset = lastY - ypos;

		lastX = xpos;
		lastY = ypos;

		xoffset *= mouseSensitivity;
		yoffset *= mouseSensitivity;

		camera.rotateOy(-xoffset);
		camera.rotateOx(yoffset);
		/*float mouseYaw = xoffset * mouseSensitivity * 50.0f;
		float mousePitch = yoffset * mouseSensitivity * 50.0f;

		camera.rotateOy(-mouseYaw);
		camera.rotateOx(mousePitch);*/


		//test mouse input
		if (window.isMousePressed(GLFW_MOUSE_BUTTON_LEFT))
		{
			std::cout << "Pressing mouse button" << std::endl;
		}


		//s1 player and gravity
		/*glm::vec3 pos = camera.getCameraPosition();
		pos.y = groundY + playerHeight;
		camera.setCameraPosition(pos);*/

		//float groundY = getGroundHeight(
		//	camera.getCameraPosition().x,
		//	camera.getCameraPosition().z
		//) + playerHeight;

		//// gravity
		//verticalVelocity += gravity * deltaTime;

		//glm::vec3 pos = camera.getCameraPosition();
		//pos.y += verticalVelocity * deltaTime;

		//// ground collision
		//if (pos.y < groundY)
		//{
		//	pos.y = groundY;
		//	verticalVelocity = 0.0f;
		//}


		//camera.setCameraPosition(pos);

		float groundY = getGroundHeight(
			camera.getCameraPosition().x,
			camera.getCameraPosition().z
		) + playerHeight;

		// gravity
		verticalVelocity += gravity * deltaTime;

		glm::vec3 pos = camera.getCameraPosition();
		pos.y += verticalVelocity * deltaTime;

		// ground collision
		if (pos.y < groundY)
		{
			pos.y = groundY;
			verticalVelocity = 0.0f;
		}

		camera.setCameraPosition(pos);



		//
		//// Code for the light ////

		sunShader.use();

		//broken sky: 
		glm::mat4 ProjectionMatrix = glm::perspective(90.0f, window.getWidth() * 1.0f / window.getHeight(), 0.1f, 10000.0f);

		glm::mat4 ViewMatrix = glm::lookAt(camera.getCameraPosition(), camera.getCameraPosition() + camera.getCameraViewDirection(), camera.getCameraUp());

		GLuint MatrixID = glGetUniformLocation(sunShader.getId(), "MVP");

		//Test for one Obj loading = light source

		glm::mat4 ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, lightPos);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

		sun.draw(sunShader);

		//// End code for the light ////

		shader.use();
		glUniform1i(
			glGetUniformLocation(shader.getId(), "texture_diffuse1"),
			0
		);

		//s1 build sword /////

		//s1 reuse vars
		GLuint MatrixID2 = glGetUniformLocation(shader.getId(), "MVP");
		GLuint ModelMatrixID = glGetUniformLocation(shader.getId(), "model");

		// ----- Sword (attached to camera) -----
		glm::mat4 swordModel = glm::mat4(1.0f);

		// Move to camera position
		swordModel = glm::translate(swordModel, camera.getCameraPosition());

		// Rotate with camera
		swordModel *= glm::mat4(glm::mat3(ViewMatrix)); // remove translation

		// Offset relative to player
		swordModel = glm::translate(swordModel, swordOffset);

		// Scale down
		swordModel = glm::scale(swordModel, glm::vec3(0.5f));

		//swing
		/*float swingAngle = sin(swingTime) * 30.0f;
		swordModel = glm::rotate(swordModel, glm::radians(swingAngle), glm::vec3(1, 0, 0));*/
		//float swingAngle = sin(swingTime) * 300.0f; // degrees
		float swingAngle = glm::clamp(sin(swingTime) * 1000.0f, -6000.0f, 10000.0f);

		swordModel = glm::rotate(
			swordModel,
			glm::radians(swingAngle),
			glm::vec3(1, 0, 0)
		);



		glm::mat4 swordMVP = ProjectionMatrix * ViewMatrix * swordModel;

		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &swordMVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &swordModel[0][0]);

		//sword.draw(shader);
		//s2 sword draw
		// ----- Sword base transform (attached to camera) -----
		glm::mat4 swordBase = glm::mat4(1.0f);

		// Move to camera position
		swordBase = glm::translate(swordBase, camera.getCameraPosition());

		// Rotate with camera (remove translation from view)
		swordBase *= glm::mat4(glm::mat3(ViewMatrix));

		// Offset in front of player
		swordBase = glm::translate(swordBase, swordOffset);

		// Overall sword scale
		swordBase = glm::scale(swordBase, glm::vec3(0.5f));

		//// Swing animation unused
		//float swingAngle = glm::clamp(
		//	sin(swingTime) * 80.0f,
		//	-60.0f,
		//	100.0f
		//);

		swordBase = glm::rotate(
			swordBase,
			glm::radians(swingAngle),
			glm::vec3(1, 0, 0)
		);


		//s2 sword draww
		std::vector<SwordPart> swordParts =
			getSwordParts(bladeTex, guardTex, handleTex, pommelTex);

		for (auto& part : swordParts)
		{
			glm::mat4 model = swordBase;
			model = glm::translate(model, part.offset);
			model = glm::scale(model, part.scale);

			glm::mat4 MVP = ProjectionMatrix * ViewMatrix * model;

			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);

			// 🔑 Bind texture for THIS sword part
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, part.textureID);

			box.draw(shader);
		}





		///// Test Obj files for box ////

		/*GLuint MatrixID2 = glGetUniformLocation(shader.getId(), "MVP");
		GLuint ModelMatrixID = glGetUniformLocation(shader.getId(), "model");*/

		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
		glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
		glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"), camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

		box.draw(shader);



		////s1 enemy
		//if (enemyAlive)
		//{
		//	ModelMatrix = glm::mat4(1.0f);
		//	ModelMatrix = glm::translate(ModelMatrix, enemyPos);
		//	MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		//	glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		//	glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		//	box.draw(shader); // reuse cube as enemy

		//}


		//s2 wizad enemy
		// Pre-create wizard parts
		std::vector<WizardPart> wizardParts = getWizardParts();

		if (enemyAlive)
		{
			for (auto& part : wizardParts)
			{
				glm::mat4 ModelMatrix = glm::mat4(1.0f);
				ModelMatrix = glm::translate(ModelMatrix, enemyPos + part.offset);
				ModelMatrix = glm::scale(ModelMatrix, part.scale);

				glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

				box.draw(shader);  // Re-use the cube mesh
			}
		}




		///// Test plane Obj file //////

		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, -20.0f, 0.0f));
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		//s1 replace with new mesh
		//plane.draw(shader);
		ModelMatrix = glm::mat4(1.0f);
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

		terrain.draw(shader);



		//s1 combat logic
		float enemyBaseY = 0.0f;
		float attackRange = 15.0f;
		

		if (window.isPressed(GLFW_KEY_E) && enemyAlive && currentTask == 0)
		{
			swingTime += deltaTime * 10.0f;
			float d = distance(camera.getCameraPosition(), enemyPos);
			if (d < attackRange)
			{
				enemyAlive = false;
				currentTask = 1;
				std::cout << "Task 1 complete!" << std::endl;
				std::cout << "Task 2: Explore the arena." << std::endl;
			}
			else {
				std::cout << "Too far to attack." << std::endl;
			}
		}else {
			swingTime = 0.0f;
		}
		enemyPos.y = enemyBaseY + sin(glfwGetTime()) * 2.0f;

		//s1 skybox load 
		//!!!!!!
		//glDepthFunc(GL_LEQUAL);
		//glDepthMask(GL_FALSE);
		//glDisable(GL_CULL_FACE);
		//skyboxShader.use();

		//glm::mat4 view = glm::mat4(glm::mat3(ViewMatrix)); // remove translation
		//glm::mat4 vp = ProjectionMatrix * view;

		//glUniformMatrix4fv(
		//	glGetUniformLocation(skyboxShader.getId(), "VP"),
		//	1,
		//	GL_FALSE,
		//	&vp[0][0]
		//);

		//glBindVertexArray(skyboxVAO);
		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		//glDrawArrays(GL_TRIANGLES, 0, 36);
		//
		//glEnable(GL_CULL_FACE);
		//glDepthMask(GL_TRUE);
		//glDepthFunc(GL_LESS);

		// ---------- SKYBOX (DRAW FIRST) ----------
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_CULL_FACE);

		skyboxShader.use();

		// REMOVE camera translation
		glm::mat4 view = glm::mat4(glm::mat3(
			glm::lookAt(
				camera.getCameraPosition(),
				camera.getCameraPosition() + camera.getCameraViewDirection(),
				camera.getCameraUp()
			)
		));

		// IMPORTANT: radians
		glm::mat4 proj = glm::perspective(
			glm::radians(70.0f),
			(float)window.getWidth() / window.getHeight(),
			0.1f,
			1000.0f
		);

		glm::mat4 VP = proj * view;
		glUniformMatrix4fv(
			glGetUniformLocation(skyboxShader.getId(), "VP"),
			1, GL_FALSE, &VP[0][0]
		);

		glBindVertexArray(skyboxVAO);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		glEnable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
		// ---------- END SKYBOX ----------



		window.update();
	}
}

void processKeyboardInput()
{
	float cameraSpeed = 30 * deltaTime;

	//translation
	if (window.isPressed(GLFW_KEY_W))
		//std::cout << "wwww" << std::endl;
		camera.keyboardMoveFront(cameraSpeed);
	if (window.isPressed(GLFW_KEY_S))
		camera.keyboardMoveBack(cameraSpeed);
	if (window.isPressed(GLFW_KEY_A))
		//std::cout << "aaaaa" << std::endl;
		camera.keyboardMoveLeft(cameraSpeed);
	if (window.isPressed(GLFW_KEY_D))
		camera.keyboardMoveRight(cameraSpeed);
	if (window.isPressed(GLFW_KEY_R))
		camera.keyboardMoveUp(cameraSpeed);
	if (window.isPressed(GLFW_KEY_F))
		camera.keyboardMoveDown(cameraSpeed);
	 
	//s2 jump
	if (window.isPressed(GLFW_KEY_SPACE)) {
		float groundY = getGroundHeight(camera.getCameraPosition().x, camera.getCameraPosition().z) + playerHeight;
		if (camera.getCameraPosition().y <= groundY + 0.01f) {
			verticalVelocity = 30.0f; // Jump strength
		}
	}

	//s1 disabled
	////rotation
	//if (window.isPressed(GLFW_KEY_LEFT))	
	//	camera.rotateOy(cameraSpeed);
	//if (window.isPressed(GLFW_KEY_RIGHT))
	//	camera.rotateOy(-cameraSpeed);
	//if (window.isPressed(GLFW_KEY_UP))
	//	camera.rotateOx(cameraSpeed);
	//if (window.isPressed(GLFW_KEY_DOWN))
	//	camera.rotateOx(-cameraSpeed);
}
