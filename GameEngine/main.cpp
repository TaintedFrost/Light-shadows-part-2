#include "Graphics\window.h"
#include "Camera\camera.h"
#include "Shaders\shader.h"
#include "Model Loading\mesh.h"
#include "Model Loading\texture.h"
#include "Model Loading\meshLoaderObj.h"
#include <time.h>
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

void processKeyboardInput();

float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

Window window("Game Engine", 1000, 900);

Camera camera;

glm::vec3 lightColor = glm::vec3(1.0f);
glm::vec3 lightPos = glm::vec3(-180.0f, 100.0f, -200.0f);


bool task1Completed = false;
//new ui stuff
const char* getTaskText(int task)
{
	switch (task)
	{
	case 0: return "Task 1: Defeat the Warlock (Press E)";
	case 1: return "Task 2: Solve the Memory Puzzle";
	case 2: return "Task 3: Claim the Lunar Blade";
	case 3: return "Task 4: Defeat the Shadow Wizard";
	case 5: return "Final Task: Defeat the Dark Wizard";
	default: return "Task Complete!";
	}
}


//s4 ui
void drawUIRect(
	Shader& shader,
	Mesh& box,
	glm::vec2 pos,      // (-1..1) screen space
	glm::vec2 size,     // width/height
	GLuint texture,
	glm::mat4 ortho
)
{
	glm::mat4 model = glm::mat4(1.0f);
	model = glm::translate(model, glm::vec3(pos.x, pos.y, 0.0f));
	model = glm::scale(model, glm::vec3(size.x, size.y, 1.0f));

	glm::mat4 MVP = ortho * model;

	glUniformMatrix4fv(
		glGetUniformLocation(shader.getId(), "MVP"),
		1, GL_FALSE, &MVP[0][0]
	);
	glUniformMatrix4fv(
		glGetUniformLocation(shader.getId(), "model"),
		1, GL_FALSE, &model[0][0]
	);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);

	box.draw(shader);
}



//task3
bool swordCollected = false;
//bool showSwordPrompt = false;
glm::vec3 pedestalPos = glm::vec3(60.0f, 0.0f, -40.0f);
float swordHoverTime = 0.0f;
GLuint currentBladeTex = 0;


//s1 enemy
glm::vec3 enemyPos = glm::vec3(-10.0f, 0.0f, 0.0f);// midde irelevent
bool enemyAlive = true;

float distance(glm::vec3 a, glm::vec3 b)
{
	return glm::length(a - b);
}
struct TreeCollider
{
	glm::vec3 center;
	float radius;
	float height;
};
// define a collision to block camera from moving through the trees

std::vector<TreeCollider> treeColliders;

bool checkTreeCollision(
	const glm::vec3& newPos,
	const std::vector<TreeCollider>& colliders
)
{
	for (const auto& c : colliders)
	{
		// Horizontal distance only (XZ plane)
		glm::vec2 p(newPos.x, newPos.z);
		glm::vec2 t(c.center.x, c.center.z);

		float dist = glm::length(p - t);
		if (dist < c.radius)
		{
			return true;
		}
	}
	return false;
}

//s1 vars
int currentTask = 0;
glm::vec3 swordOffset = glm::vec3(5.0f, -5.0f, -10.0f);
float swingTime = 0.0f;

// Task 2 Vars
enum Task2Phase {
	T2_IDLE,
	T2_SHINY_CUBE,
	T2_SHOW_SAMPLE,
	T2_SCATTERED,
	T2_COMPLETED,
	T2_FAILED_WAIT // New phase for failure feedback
};

struct PuzzleCube {
	glm::vec3 pos;
	int type; // 0=Wood, 1=Rock, 2=Fire(Orange), 3=Orange(Texture)
	bool collected;
	glm::vec3 targetPos; // For animation or placeholder slot
};

Task2Phase t2Phase = T2_IDLE;
float t2Timer = 0.0f;
std::vector<PuzzleCube> puzzleCubes;
glm::vec3 shinyCubePos;
int collectedCubesCount = 0;
glm::vec3 t2BasePos;

// Store the correct order
std::vector<int> t2TargetOrder;
// Store what the user collected
std::vector<int> t2UserOrder;

// TASK 4 VARS
bool t4WizardAlive = true;
glm::vec3 t4WizardPos; // Set when task starts
bool t4MiniGameActive = false;
float t4BarPos = 0.0f;     // Range -1.0 to 1.0
float t4BarDir = 1.0f;     // Direction
float t4BarSpeed = 0.8f;   // Speed of the bar (Slower)
float t4SafeZoneWidth = 0.25f; // [-0.125, 0.125] - Bigger safe zone

// TASK 5 VARS
struct Projectile {
	glm::vec3 pos;
	glm::vec3 dir;
	bool active;
};
std::vector<Projectile> t5Projectiles;
bool t5BossAlive = true;
glm::vec3 t5BossPos;
int t5PlayerLives = 3;
int t5BossHealth = 10;
float t5CycleTimer = 0.0f; // Track 10s/5s cycle
bool t5IsAttacking = true; // true = 10s attack, false = 5s rest
float t5ShootCooldown = 0.0f;
bool t5GameWon = false;
bool t5GameOver = false;


//s1 player and camera
bool firstMouse = true;
float lastX = 400.0f;
float lastY = 400.0f;
float mouseSensitivity = 0.1f;

float playerHeight = 10.0f;   // eye height above ground
//float groundY = -20.0f;

float verticalVelocity = 0.0f;
float gravity = -40.0f;

//s2 flat ground
//float getGroundHeight(float x, float z)
//{
//	return 0.0f;
//}
float getGroundHeight(float x, float z)
{
	// Base flat ground
	float height = 0.0f;

	// ---- Lunar Blade Hill ----
	glm::vec2 hillCenter(60.0f, -40.0f); // pedestalPos XZ
	glm::vec2 p(x, z);

	float dist = glm::distance(p, hillCenter);

	float hillRadius = 40.0f;
	float hillHeight = 20.0f;

	if (dist < hillRadius)
	{
		float t = 1.0f - (dist / hillRadius);
		height += t * t * hillHeight; // smooth hill
	}

	return height;
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

	//Generate vertices
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

	//Generate indices
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

//skybox
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

//pixel loader for skybox
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

struct WizardPart
{
	glm::vec3 offset;
	glm::vec3 scale;
	GLuint texture;
};


//Sword parts (procedural model)
struct SwordPart
{
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

	// Blade
	parts.push_back({
		glm::vec3(0.0f, 2.5f, 0.0f),
		glm::vec3(0.4f, 8.0f, 0.2f),
		bladeTex
		});

	// Guard
	parts.push_back({
		glm::vec3(0.0f, 2.0f, 0.0f),
		glm::vec3(2.0f, 0.3f, 0.5f),
		guardTex
		});

	// Handle
	parts.push_back({
		glm::vec3(0.0f, -1.5f, -0.3f),
		glm::vec3(0.3f, 1.0f, 0.3f),
		handleTex
		});

	// Pommel
	parts.push_back({
		glm::vec3(0.0f, -3.5f, -0.9f),
		glm::vec3(0.5f, 0.5f, 0.5f),
		pommelTex
		});

	return parts;
}

//wiz model build
std::vector<WizardPart> getWizardParts(
	GLuint robeTex,
	GLuint skinTex,
	GLuint hatTex,
	GLuint staffTex
)
{
	std::vector<WizardPart> parts;

	// ===== ROBE BODY (tall + tapered look) =====
	parts.push_back({
		glm::vec3(0.0f, 1.5f, 0.0f),
		glm::vec3(1.5f, 2.3f, 0.9f),
		robeTex
		});

	// ===== HEAD =====
	parts.push_back({
		glm::vec3(0.0f,13.0f, 0.0f),
		glm::vec3(0.8f, 0.8f, 0.8f),
		skinTex
		});

	// ===== HAT BRIM =====
	parts.push_back({
		glm::vec3(0.0f, 15.7f, 0.0f),
		glm::vec3(1.4f, 0.15f, 1.4f),
		hatTex
		});

	// ===== HAT TOP =====
	parts.push_back({
		glm::vec3(0.0f, 15.9f, 0.0f),
		glm::vec3(0.7f, 1.4f, 0.7f),
		hatTex
		});

	// ===== LEFT ARM =====
	parts.push_back({
		glm::vec3(-5.1f, 2.2f, 0.0f),
		glm::vec3(0.35f, 1.6f, 0.35f),
		robeTex
		});

	// ===== RIGHT ARM =====
	parts.push_back({
		glm::vec3(5.1f, 2.2f, 0.0f),
		glm::vec3(0.35f, 1.6f, 0.35f),
		robeTex
		});

	// ===== STAFF =====
	parts.push_back({
		glm::vec3(5.8f, 1.8f, 0.0f),
		glm::vec3(0.2f, 3.8f, 0.2f),
		staffTex
		});

	return parts;
}


// TASK 2 HELPERS

// Check if a position collides with any tree (simple circle check)
bool isSafeFromTrees(glm::vec3 pos, float safeRadius, const std::vector<TreeCollider>& colliders)
{
	for (const auto& c : colliders)
	{
		// Distance in XZ plane
		float d = glm::distance(glm::vec2(pos.x, pos.z), glm::vec2(c.center.x, c.center.z));
		if (d < (c.radius + safeRadius))
		{
			return false; // Collision
		}
	}
	return true;
}

glm::vec3 spawnRandomCubeSafe(const std::vector<TreeCollider>& colliders, glm::vec3 center, float range)
{
	int attempts = 0;
	while (attempts < 100)
	{
		float rx = ((rand() % 100) / 50.0f - 1.0f) * range; // -range to range
		float rz = ((rand() % 100) / 50.0f - 1.0f) * range;

		glm::vec3 cand = center + glm::vec3(rx, 0.0f, rz);
		cand.y = getGroundHeight(cand.x, cand.z) + 2.0f; // On ground

		if (isSafeFromTrees(cand, 3.0f, colliders))
		{
			return cand;
		}
		attempts++;
	}
	return center + glm::vec3(0, 10, 0); // Fallback
}


/////////////////////////////////////////
int main()
{
	srand(time(NULL));
	glClearColor(0.2f, 0.8f, 1.0f, 1.0f);
	//s1 grav test
	camera.setCameraPosition(glm::vec3(0.0f, 50.0f, 100.0f));
	glfwSetInputMode(window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	glfwMakeContextCurrent(window.getWindow());

	//building and compiling shader program
	Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
	Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");
	//s1 skybox
	Shader skyboxShader("Shaders/skybox_vertex.glsl", "Shaders/skybox_fragment.glsl");
	skyboxShader.use();
	glUniform1i(glGetUniformLocation(skyboxShader.getId(), "skybox"), 0);

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

	//Textures LOAD !imp
	GLuint redTex = loadBMP("Resources/Textures/red.bmp");
	GLuint blueTex = loadBMP("Resources/Textures/blue.bmp");
	GLuint yellowTex = loadBMP("Resources/Textures/yellow.bmp");
	GLuint tex_tree = loadBMP("Resources/Textures/TreeUVmap.bmp");
	//Sword textures
	GLuint bladeTex = loadBMP("Resources/Textures/swordBase.bmp");
	GLuint guardTex = loadBMP("Resources/Textures/swordBase.bmp");
	GLuint handleTex = loadBMP("Resources/Textures/goldTexture.bmp");
	GLuint pommelTex = loadBMP("Resources/Textures/swordBase.bmp");
	GLuint lunarBladeTex = loadBMP("Resources/Textures/lunarblade.bmp");
	currentBladeTex = bladeTex; // start with normal sword
	//wiz textures
	GLuint wizRobeTex = loadBMP("Resources/Textures/wizard_robe.bmp");
	GLuint wizSkinTex = loadBMP("Resources/Textures/wizard_skin.bmp");
	GLuint wizHatTex = loadBMP("Resources/Textures/wizard_hat.bmp");
	GLuint wizStaffTex = loadBMP("Resources/Textures/wizard_staff.bmp");

	GLuint metalTex = loadBMP("Resources/Textures/metal.bmp");


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
	textures[0].id = redTex;
	textures[0].type = "texture_diffuse";

	std::vector<Texture> textures2;
	textures2.push_back(Texture());
	textures2[0].id = blueTex;
	textures2[0].type = "texture_diffuse";

	std::vector<Texture> textures3;
	textures3.push_back(Texture());
	textures3[0].id = yellowTex;
	textures3[0].type = "texture_diffuse";

	// s2 Orange2 (Requested texture)
	GLuint yellowTex2 = loadBMP("Resources/Textures/yellow.bmp");
	// metal
	GLuint greenTex = loadBMP("Resources/Textures/green.bmp");
	//s2 grass texture
	GLuint grassTex = loadBMP("Resources/Textures/grass.bmp");

	std::vector<Texture> grassTextures;
	grassTextures.push_back(Texture());
	grassTextures[0].id = grassTex;
	grassTextures[0].type = "texture_diffuse";

	// tree texture 
	std::vector<Texture> treeTextures;
	treeTextures.push_back(Texture());
	treeTextures[0].id = tex_tree;
	treeTextures[0].type = "texture_diffuse";

	// ---------- Empty texture list for procedural cube rendering ----------
	std::vector<Texture> emptyTextures;

	Mesh mesh(vert, ind, textures3);

	// Create Obj files - easier :)
	// we can add here our textures :)
	MeshLoaderObj loader;
	Mesh sun = loader.loadObj("Resources/Models/sphere.obj");
	//Mesh box = loader.loadObj("Resources/Models/cube.obj", textures);
	// // Cube mesh WITHOUT textures (textures bound manually per draw)
	Mesh box = loader.loadObj("Resources/Models/cube.obj", emptyTextures);
	//task2 sphapes
	Mesh sphereMesh = loader.loadObj("Resources/Models/sphere.obj", emptyTextures);
	Mesh pyramidMesh = loader.loadObj("Resources/Models/cube.obj", emptyTextures);
	Mesh hexMesh = loader.loadObj("Resources/Models/sphere.obj", emptyTextures);


	//Mesh plane = loader.loadObj("Resources/Models/plane.obj", textures3);
	//s1 new plane
	Mesh terrain = generateTerrain(
		120,     // width
		120,     // depth
		5.0f,    // scale
		grassTextures // (we’ll swap this to grass)
	);

	// tree
	Mesh tree = loader.loadObj("Resources/Models/HorrorTree.obj", treeTextures);
	// tree positions
	std::vector<glm::vec3> treePositions;
	for (int x = -300; x <= 300; x += 40)
	{
		for (int z = -300; z <= 300; z += 40)
		{
			float y = getGroundHeight(x, z);
			treePositions.push_back(glm::vec3(x, y, z));
		}
	}

	for (const auto& pos : treePositions)
	{
		TreeCollider c;
		c.center = pos;
		c.radius = 2.5f * 5.0f;   // scale * trunk radius
		c.height = 20.0f * 5.0f;

		treeColliders.push_back(c);
	}

	//s1 task1 intro
	std::cout << "Task 1: Approach the warlock and press E to attack." << std::endl;


	//s4ui
	// ===== IMGUI INIT =====
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();

	GLFWwindow* glfwWindow = window.getWindow();
	ImGui_ImplGlfw_InitForOpenGL(glfwWindow, true);
	ImGui_ImplOpenGL3_Init("#version 330");



	//check if we close the window or press the escape button
	//game loop?
	while (!window.isPressed(GLFW_KEY_ESCAPE) &&
		glfwWindowShouldClose(window.getWindow()) == 0)
	{
		window.clear();
		//s5
		pedestalPos.y = getGroundHeight(pedestalPos.x, pedestalPos.z);

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

		//test mouse input
		if (window.isMousePressed(GLFW_MOUSE_BUTTON_LEFT))
		{
			std::cout << "Pressing mouse button" << std::endl;
		}

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


		float swingAngle = glm::clamp(sin(swingTime) * 1000.0f, -6000.0f, 10000.0f);
		swordModel = glm::rotate(
			swordModel,
			glm::radians(swingAngle),
			glm::vec3(1, 0, 0)
		);


		// ---------- Procedural sword drawing ----------
		std::vector<SwordPart> swordParts =
			getSwordParts(currentBladeTex, guardTex, handleTex, pommelTex);

		for (auto& part : swordParts)
		{
			glm::mat4 model = swordModel;
			model = glm::translate(model, part.offset);
			model = glm::scale(model, part.scale);

			glm::mat4 MVP = ProjectionMatrix * ViewMatrix * model;

			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);

			// Bind texture for this sword part
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, part.textureID);

			box.draw(shader);
		}



		///// Test Obj files for box ////
		ModelMatrix = glm::mat4(1.0);
		ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, 0.0f, 0.0f));
		MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
		glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"), lightColor.x, lightColor.y, lightColor.z);
		glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
		glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"), camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);
		box.draw(shader);

		//make wiz task1
		auto wizardParts = getWizardParts(
			wizRobeTex,
			wizSkinTex,
			wizHatTex,
			wizStaffTex
		);
		if (enemyAlive)
		{
			for (auto& part : wizardParts)
			{
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, enemyPos + part.offset);
				model = glm::scale(model, part.scale);

				glm::mat4 MVP = ProjectionMatrix * ViewMatrix * model;

				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, part.texture);

				box.draw(shader); // ONLY cube.obj
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


		// trees drawing
		for (const auto& treePos : treePositions)
		{
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, treePos);
			model = glm::scale(model, glm::vec3(5.0f));

			MVP = ProjectionMatrix * ViewMatrix * model;

			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);

			tree.draw(shader);
		}

		//s1 combat logic
		float enemyBaseY = 0.0f;
		float attackRange = 15.0f;


		if (window.isPressed(GLFW_KEY_E))
		{
			swingTime += deltaTime * 10.0f;

			if (!task1Completed && currentTask == 0 && enemyAlive)
			{
				float d = distance(camera.getCameraPosition(), enemyPos);
				if (d < attackRange)
				{
					enemyAlive = false;
					task1Completed = true;

					currentTask = 1;
					t2Phase = T2_SHINY_CUBE;

					shinyCubePos = enemyPos + glm::vec3(-25.0f, 0.0f, 0.0f);
					shinyCubePos.y =
						getGroundHeight(shinyCubePos.x, shinyCubePos.z) + 3.0f;

					std::cout << "Task 1 complete!" << std::endl;
					std::cout << "Task 2 started." << std::endl;
				}
				else {
					std::cout << "Too far to attack." << std::endl;
				}
			}
			/*swingTime += deltaTime * 10.0f;
			float d = distance(camera.getCameraPosition(), enemyPos);
			if (d < attackRange)
			{
				enemyAlive = false;
				currentTask = 1;

				std::cout << "Task 1 complete!" << std::endl;
				std::cout << "Task 2: Explore the arena." << std::endl;

				t2Phase = T2_SHINY_CUBE;
				shinyCubePos = enemyPos + glm::vec3(-25.0f, 0.0f, 0.0f);
				shinyCubePos.y = getGroundHeight(shinyCubePos.x, shinyCubePos.z) + 3.0f;
			}
			else {
				std::cout << "Too far to attack." << std::endl;
			}*/
		}
		else {
			//swingTime = 0.0f;
			// Smoothly reset instead of snapping
			swingTime = glm::max(swingTime - deltaTime * 15.0f, 0.0f);
		}
		enemyPos.y = enemyBaseY + sin(glfwGetTime()) * 2.0f;


		// TASK 2 LOGIC         <<<<<<<<<<<<<<<<<<<<<<<<<<

		static std::vector<glm::vec3> t2Slots; // Store slot positions

		//if (currentTask == 1 && t2Phase == T2_SHINY_CUBE)
		if (t2Phase == T2_SHINY_CUBE)
		{
			if (distance(camera.getCameraPosition(), shinyCubePos) < 15.0f)
			{
				std::cout << "Task 2 Started! Memorize the order!" << std::endl;
				t2Phase = T2_SHOW_SAMPLE;
				t2Timer = 5.0f;
				t2BasePos = enemyPos + glm::vec3(-20.0f, 0.0f, 0.0f);

				t2Slots.clear();
				puzzleCubes.clear();
				t2TargetOrder.clear();
				t2UserOrder.clear();

				std::vector<int> types = { 0, 1, 2, 3 };
				// Simple shuffle
				for (int i = 0; i < 10; i++) {
					int r1 = rand() % 4;
					int r2 = rand() % 4;
					std::swap(types[r1], types[r2]);
				}

				t2TargetOrder = types;

				// Align along Z axis
				glm::vec3 right = glm::vec3(0.0f, 0.0f, 1.0f);

				for (int i = 0; i < 4; i++)
				{
					PuzzleCube pc;
					pc.type = types[i];
					glm::vec3 slotPos = t2BasePos + right * ((i - 1.5f) * 6.5f);
					slotPos.y = getGroundHeight(slotPos.x, slotPos.z) + 2.0f;

					pc.pos = slotPos;
					pc.targetPos = slotPos; // original slot
					pc.collected = false;

					puzzleCubes.push_back(pc);
					t2Slots.push_back(slotPos);
				}
				collectedCubesCount = 0;
			}
		}
		else if (t2Phase == T2_SHOW_SAMPLE)
		{
			t2Timer -= deltaTime;
			if (t2Timer <= 0.0f)
			{
				std::cout << "Scattered! Find them!" << std::endl;
				t2Phase = T2_SCATTERED;
				for (auto& pc : puzzleCubes)
				{
					pc.pos = spawnRandomCubeSafe(treeColliders, t2BasePos, 40.0f);
					pc.collected = false;
				}
				collectedCubesCount = 0;
				t2UserOrder.clear();
			}
		}
		else if (t2Phase == T2_SCATTERED)
		{
			if (collectedCubesCount < 4 && window.isPressed(GLFW_KEY_E))
			{
				bool found = false;
				for (auto& pc : puzzleCubes)
				{
					if (!pc.collected && distance(camera.getCameraPosition(), pc.pos) < 15.0f)
					{
						pc.collected = true;
						pc.pos = t2Slots[collectedCubesCount]; // Move to next available slot

						t2UserOrder.push_back(pc.type);

						collectedCubesCount++;
						found = true;
						std::cout << "Collected cube! " << collectedCubesCount << "/4" << std::endl;
						break; // Collect one at a time
					}
				}

				if (collectedCubesCount >= 4)
				{
					// VERIFY ORDER
					bool correct = true;
					for (size_t i = 0; i < 4; ++i) {
						if (t2UserOrder[i] != t2TargetOrder[i]) {
							correct = false;
							break;
						}
					}

					if (correct)
					{
						t2Phase = T2_COMPLETED;
						std::cout << "Task 2 Completed! Correct Order!" << std::endl;
						currentTask = 2;
						std::cout << "Task 3: Claim the Lunar Blade." << std::endl;
					}
					else
					{
						std::cout << "Wrong Order! Resetting in 5 seconds..." << std::endl;
						t2Phase = T2_FAILED_WAIT;
						t2Timer = 5.0f;
					}
				}
			}
		}
		else if (t2Phase == T2_FAILED_WAIT)
		{
			t2Timer -= deltaTime;
			if (t2Timer <= 0.0f)
			{
				std::cout << "Scattering Again..." << std::endl;
				// Reset Logic
				t2Phase = T2_SCATTERED;
				// Re-scatter cubes
				for (auto& pc : puzzleCubes)
				{
					pc.pos = spawnRandomCubeSafe(treeColliders, t2BasePos, 40.0f);
					pc.collected = false;
				}
				collectedCubesCount = 0;
				t2UserOrder.clear();
			}
		}

		// TASK 2 RENDER

		if (t2Phase != T2_IDLE)
		{
			// Shiny Cube
			if (t2Phase == T2_SHINY_CUBE)
			{
				sunShader.use(); // Draw as bright object
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, shinyCubePos);
				model = glm::scale(model, glm::vec3(1.5f));

				glm::mat4 mvp = ProjectionMatrix * ViewMatrix * model;
				glUniformMatrix4fv(glGetUniformLocation(sunShader.getId(), "MVP"), 1, GL_FALSE, &mvp[0][0]);

				box.draw(sunShader);
				shader.use(); // Restore
			}


			// Placeholders
			if (t2Phase == T2_SCATTERED || t2Phase == T2_COMPLETED || t2Phase == T2_FAILED_WAIT)
			{
				// Draw small markers for slots
				for (const auto& slot : t2Slots)
				{
					glm::mat4 model = glm::mat4(1.0f);
					model = glm::translate(model, slot);
					// Placeholders: Square base (1.0 x 1.0) to match cubes
					model = glm::scale(model, glm::vec3(1.0f, 0.2f, 1.0f));

					glm::mat4 mvp = ProjectionMatrix * ViewMatrix * model;
					glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &mvp[0][0]);
					glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, redTex);
					box.draw(shader);
				}
			}

			//// Cubes
			//if (t2Phase != T2_SHINY_CUBE)
			//{
			//	for (auto& pc : puzzleCubes)
			//	{
			//		GLuint tID = tex;
			//		if (pc.type == 0) tID = tex;  // Wood
			//		if (pc.type == 1) tID = tex2; // Rock
			//		if (pc.type == 2) tID = tex5; // metal
			//		if (pc.type == 3) tID = tex4; // Orange2

			//		glActiveTexture(GL_TEXTURE0);
			//		glBindTexture(GL_TEXTURE_2D, tID);

			//		glm::mat4 model = glm::mat4(1.0f);
			//		model = glm::translate(model, pc.pos);
			//		model = glm::scale(model, glm::vec3(1.0f, 1.0f, 1.0f)); 

			//		glm::mat4 mvp = ProjectionMatrix * ViewMatrix * model; // Fixed identifier case

			//		glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &mvp[0][0]);
			//		glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);

			//		box.draw(shader);
			//	}
			//}

			// Shapes (Task 2)
			if (t2Phase != T2_SHINY_CUBE)
			{
				for (auto& pc : puzzleCubes)
				{
					// ----- Texture -----
					GLuint tID = redTex;
					if (pc.type == 0) tID = redTex;   // Wood
					if (pc.type == 1) tID = blueTex;  // Rock
					if (pc.type == 2) tID = greenTex;  // Metal
					if (pc.type == 3) tID = yellowTex2;  // Orange

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, tID);

					glm::mat4 model = glm::mat4(1.0f);
					/*model = glm::translate(model, pc.pos);*/
					// Lift shapes above ground so they don't clip
					glm::vec3 liftedPos = pc.pos;
					liftedPos.y += 1.2f;   // 👈 adjust if needed
					model = glm::translate(model, liftedPos);

					// Per-shape scaling (Force 1.0f for Cube)
					model = glm::scale(model, glm::vec3(1.0f));

					glm::mat4 mvp = ProjectionMatrix * ViewMatrix * model;

					glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &mvp[0][0]);
					glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);

					// ----- SHAPE SELECT -----
					// Always draw cube as requested
					box.draw(shader);
				}
			}

		}


		//task3 logic
		if (currentTask == 2 && !swordCollected)
		{
			// ----- Stone Pedestal -----
			glm::mat4 pedestalModel = glm::mat4(1.0f);
			pedestalModel = glm::translate(
				pedestalModel,
				pedestalPos + glm::vec3(0.0f, 4.0f, 0.0f)
			);
			pedestalModel = glm::scale(pedestalModel, glm::vec3(6.0f, 4.0f, 6.0f));

			glm::mat4 pedestalMVP = ProjectionMatrix * ViewMatrix * pedestalModel;

			glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &pedestalMVP[0][0]);
			glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &pedestalModel[0][0]);

			glBindTexture(GL_TEXTURE_2D, metalTex); // metal/stone texture
			box.draw(shader);




			swordHoverTime += deltaTime;

			float distToSword = distance(
				camera.getCameraPosition(),
				pedestalPos
			);

			if (distToSword < 20.0f)
			{
				std::cout << "Press E to claim the Lunar Blade" << std::endl;

				if (window.isPressed(GLFW_KEY_E))
				{
					swordCollected = true;
					currentTask = 3;
					currentBladeTex = lunarBladeTex; // SWITCH PLAYER SWORD
					std::cout << "Lunar Blade secured!" << std::endl;
					std::cout << "Task 3 Completed!" << std::endl;
					std::cout << "Task 4: Defeat the Shadow Wizard!" << std::endl;


					// INIT TASK 4
					t4WizardPos = pedestalPos;
					t4WizardPos.z -= 40.0f; // "Further" behind the sword
					t4WizardPos.y = getGroundHeight(t4WizardPos.x, t4WizardPos.z);
					t4WizardAlive = true;
					t4MiniGameActive = false;
					t4BarPos = -0.9f; // Start at left (unsafe)
				}
			}
		}

		// TASK 4 LOGIC
		if (currentTask == 3 && t4WizardAlive)
		{
			float distToWiz = distance(camera.getCameraPosition(), t4WizardPos);
			if (distToWiz < 30.0f)
			{
				t4MiniGameActive = true;
				// Animate Bar
				t4BarPos += t4BarSpeed * t4BarDir * deltaTime;
				if (t4BarPos > 1.0f) {
					t4BarPos = 1.0f; t4BarDir = -1.0f;
				}
				if (t4BarPos < -1.0f) {
					t4BarPos = -1.0f; t4BarDir = 1.0f;
				}

				// Check Input
				if (window.isPressed(GLFW_KEY_E))
				{
					// Check if inside safe zone (centered at 0)
					if (abs(t4BarPos) < (t4SafeZoneWidth * 0.5f)) // Assuming width is total width
					{
						std::cout << "CRITICAL HIT!" << std::endl;

						// Simulate Attack Lunge
						glm::vec3 killPos = t4WizardPos;
						killPos.y += playerHeight;
						camera.setCameraPosition(killPos);

						t4WizardAlive = false;
						t4MiniGameActive = false;
						currentTask = 4;
						std::cout << "Task 4 Completed! YOU WIN!" << std::endl;

						// INIT TASK 5
						std::cout << "FINAL TASK: THE DARK WIZARD APPEARS!" << std::endl;
						std::cout << "Hide behind trees! Attack when he rests!" << std::endl;
						currentTask = 5;
						t5BossPos = glm::vec3(0.0f, 0.0f, 150.0f); // Far away
						t5BossPos.y = getGroundHeight(t5BossPos.x, t5BossPos.z);
						t5BossAlive = true;
						t5PlayerLives = 3;
						t5BossHealth = 10;
						t5CycleTimer = 10.0f; // Start with 10s attack
						t5IsAttacking = true;
						t5Projectiles.clear();
					}
					else
					{
						t4BarPos = -1.0f;
					}
				}
			}
			else
			{
				t4MiniGameActive = false;
			}
		}

		// TASK 5 LOGIC
		if (currentTask == 5 && t5BossAlive && !t5GameOver && !t5GameWon)
		{
			// Cycle Logic
			t5CycleTimer -= deltaTime;
			if (t5CycleTimer <= 0.0f)
			{
				t5IsAttacking = !t5IsAttacking;
				t5CycleTimer = t5IsAttacking ? 10.0f : 5.0f;
				if (t5IsAttacking) std::cout << "Boss is ATTACKING! (10s)" << std::endl;
				else               std::cout << "Boss is RESTING! (5s) ATTACK NOW!" << std::endl;
			}

			// Shooting Logic
			if (t5IsAttacking)
			{
				t5ShootCooldown -= deltaTime;
				if (t5ShootCooldown <= 0.0f)
				{
					t5ShootCooldown = 0.5f; // Fire every 0.5s
					Projectile p;
					p.pos = t5BossPos + glm::vec3(0, 8, 0); // Shoot from staff/hand height
					p.dir = glm::normalize(camera.getCameraPosition() - p.pos);
					p.active = true;
					t5Projectiles.push_back(p);
				}
			}

			// Update Projectiles
			for (auto& p : t5Projectiles)
			{
				if (!p.active) continue;

				float speed = 40.0f;
				p.pos += p.dir * speed * deltaTime;

				// Collision with Player
				if (distance(p.pos, camera.getCameraPosition()) < 3.0f)
				{
					p.active = false;
					t5PlayerLives--;
					std::cout << "HIT! Lives left: " << t5PlayerLives << std::endl;
					if (t5PlayerLives <= 0)
					{
						t5GameOver = true;
						std::cout << "GAME OVER! You died." << std::endl;
					}
				}

				// Collision with Trees
				if (checkTreeCollision(p.pos, treeColliders))
				{
					p.active = false;
				}

				// Cleanup distance
				if (distance(p.pos, t5BossPos) > 300.0f) p.active = false;
			}

			// Player Attack Logic
			if (!t5IsAttacking && window.isPressed(GLFW_KEY_E) && !t5GameWon)
			{
				// Must be close enough?
				if (distance(camera.getCameraPosition(), t5BossPos) < 30.0f)
				{
					// Simple "debounce" or rapid click handling? "Press E 10 times"
					// We'll use a small timer to prevent 1-frame insta-kill 
					static float hitTimer = 0.0f;
					hitTimer -= deltaTime;
					if (hitTimer <= 0.0f)
					{
						t5BossHealth--;
						hitTimer = 0.3f; // Max 3 hits per second
						std::cout << "Boss Hit! HP: " << t5BossHealth << std::endl;
						if (t5BossHealth <= 0)
						{
							t5BossAlive = false;
							t5GameWon = true;
							std::cout << "VICTORY! The Wizard is defeated!" << std::endl;
						}
					}
				}
			}
		}

		// ================= TASK 3 RENDER (QUEST SWORD) =================
		if (currentTask == 2 && !swordCollected)
		{
			//s5 Stone Pedestal
			//glm::mat4 pedestalModel = glm::mat4(1.0f);
			//pedestalModel = glm::translate(
			//	pedestalModel,
			//	pedestalPos + glm::vec3(0.0f, 4.0f, 0.0f)
			//);
			//pedestalModel = glm::scale(pedestalModel, glm::vec3(6.0f, 4.0f, 6.0f));

			//glm::mat4 pedestalMVP = ProjectionMatrix * ViewMatrix * pedestalModel;

			//glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &pedestalMVP[0][0]);
			//glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &pedestalModel[0][0]);

			//glBindTexture(GL_TEXTURE_2D, tex5); // metal/stone texture
			//box.draw(shader);

			

			//float hoverY = sin(swordHoverTime * 2.0f) * 2.0f;
			float hover = sin(swordHoverTime * 2.5f) * 1.2f;
			float spin = glfwGetTime() * 45.0f;


			glm::mat4 baseModel = glm::mat4(1.0f);
			baseModel = glm::translate(
				baseModel,
				pedestalPos + glm::vec3(0.0f, 10.0f + hover, 0.0f)
			);

			baseModel = glm::rotate(
				baseModel,
				glm::radians(spin),
				glm::vec3(0.0f, 1.0f, 0.0f)
			);

			baseModel = glm::rotate(
				baseModel,
				glm::radians(90.0f),
				glm::vec3(1.0f, 0.0f, 0.0f)
			);
			float tiltSpin = sin(glfwGetTime() * 1.5f) * 150.0f;

			baseModel = glm::rotate(
				baseModel,
				glm::radians(tiltSpin),
				glm::vec3(0.0f, 0.0f, 1.0f)
			);


			baseModel = glm::scale(baseModel, glm::vec3(0.6f));



			/*glm::mat4 baseModel = glm::mat4(1.0f);
			baseModel = glm::translate(
				baseModel,
				pedestalPos + glm::vec3(0.0f, 8.0f + hoverY, 0.0f)
			);

			baseModel *= glm::rotate(
				glm::radians((float)glfwGetTime() * 50.0f),
				glm::vec3(0.0f, 1.0f, 0.0f)
			);


			baseModel = glm::scale(baseModel, glm::vec3(0.6f));*/

			auto swordParts = getSwordParts(
				bladeTex, guardTex, handleTex, pommelTex
			);

			for (auto& part : swordParts)
			{
				glm::mat4 model = baseModel;
				model = glm::translate(model, part.offset);
				model = glm::scale(model, part.scale);

				glm::mat4 MVP = ProjectionMatrix * ViewMatrix * model;

				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);

				glBindTexture(GL_TEXTURE_2D, part.textureID);
				box.draw(shader);
			}
		}



		// ================= TASK 4 RENDER (WIZARD + UI) =================
		if (currentTask >= 3 && t4WizardAlive)
		{
			// Render Wizard (Reusing logic)
			auto wizardParts4 = getWizardParts(wizRobeTex, wizSkinTex, wizHatTex, wizStaffTex);
			for (auto& part : wizardParts4)
			{
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, t4WizardPos + part.offset);
				model = glm::scale(model, part.scale);

				glm::mat4 MVP = ProjectionMatrix * ViewMatrix * model;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, part.texture);
				box.draw(shader);
			}

			// Render Mini-Game UI
			if (t4MiniGameActive)
			{
				// Disable Depth Test & Culling for UI overlay
				glDisable(GL_DEPTH_TEST);
				glDisable(GL_CULL_FACE); // Fix visibility

				// Orthographic Projection for 2D UI
				// Screen is -1 to 1 in both axes
				glm::mat4 orthoProj = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);
				glm::mat4 identityView = glm::mat4(1.0f);

				// --- 1. Background Bar (Metal) ---
				shader.use();
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(0.0f, -0.6f, 0.0f));
				model = glm::scale(model, glm::vec3(0.8f, 0.1f, 1.0f)); // Wide bar

				glm::mat4 MVP = orthoProj * identityView * model;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);

				// Keep lighting "neutral" for UI by placing light in front
				glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), 0, 0, 10);
				// Also update ViewPos to be in front so specular works
				glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"), 0, 0, 10);

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, redTex); // Metal
				box.draw(shader);

				// --- 2. Safe Zone (Green) ---
				model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(0.0f, -0.6f, 0.01f)); // Slightly in front
				model = glm::scale(model, glm::vec3(t4SafeZoneWidth * 0.8f, 0.11f, 1.0f)); // Green Zone

				MVP = orthoProj * identityView * model;
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]); // Update model for lighting

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, greenTex); // Green
				box.draw(shader);

				// --- 3. Moving Line (White) ---
				sunShader.use(); // Unlit white
				model = glm::mat4(1.0f);
				// Map t4BarPos (-1..1) to visual range (-0.8..0.8)
				float visualPos = t4BarPos * 0.8f;
				model = glm::translate(model, glm::vec3(visualPos, -0.6f, 0.02f)); // More in front
				model = glm::scale(model, glm::vec3(0.02f, 0.15f, 1.0f)); // Thin vertical line

				MVP = orthoProj * identityView * model;
				glUniformMatrix4fv(glGetUniformLocation(sunShader.getId(), "MVP"), 1, GL_FALSE, &MVP[0][0]);
				box.draw(sunShader);

				// Restore State
				glEnable(GL_DEPTH_TEST);
				glEnable(GL_CULL_FACE); // Restore Culling

				shader.use();
				// Restore Light/View Pos
				glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
				glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"), camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);

			}
		}



		// ================= TASK 5 RENDER =================
		if (currentTask == 5 && !t5GameOver)
		{
			// 1. Render Boss (Big Dark Wizard)
			// Re-use wizard parts but darker texture? We don't have one, so we'll use same texture but maybe red light?
			// Just Scale it UP
			if (t5BossAlive)
			{
				auto bossParts = getWizardParts(wizRobeTex, wizSkinTex, wizHatTex, wizStaffTex);
				for (auto& part : bossParts)
				{
					glm::mat4 model = glm::mat4(1.0f);
					model = glm::translate(model, t5BossPos + part.offset * 2.0f); // x2 Offset
					model = glm::scale(model, part.scale * 2.0f); // x2 Scale

					glm::mat4 MVP = ProjectionMatrix * ViewMatrix * model;
					glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
					glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);

					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, part.texture);
					box.draw(shader);
				}
			}

			// 2. Render Projectiles
			sunShader.use(); // Unlit
			// Purple Color
			// glUniform4f(glGetUniformLocation(sunShader.getId(), "OverrideColor"), 0.6f, 0.0f, 1.0f, 1.0f); 

			for (const auto& p : t5Projectiles)
			{
				if (!p.active) continue;
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, p.pos);
				model = glm::scale(model, glm::vec3(0.5f, 0.5f, 2.0f)); // Long projectile
				model = glm::rotate(model, atan2(p.dir.x, p.dir.z), glm::vec3(0, 1, 0)); // Rotate to face dir

				glm::mat4 MVP = ProjectionMatrix * ViewMatrix * model;
				glUniformMatrix4fv(glGetUniformLocation(sunShader.getId(), "MVP"), 1, GL_FALSE, &MVP[0][0]);
				box.draw(sunShader);
			}

			// 3. UI (Health Bars)
			// Disable Depth
			glDisable(GL_DEPTH_TEST);
			glDisable(GL_CULL_FACE);
			shader.use();
			glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), 0, 0, 10);
			glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"), 0, 0, 10);

			glm::mat4 ortho = glm::ortho(-1.0f, 1.0f, -1.0f, 1.0f, -1.0f, 1.0f);

			// Player Health (Bottom Left) - 3 Lives
			for (int i = 0; i < t5PlayerLives; i++) {
				glm::mat4 model = glm::mat4(1.0f);
				model = glm::translate(model, glm::vec3(-0.9f + i * 0.1f, -0.9f, 0.0f));
				model = glm::scale(model, glm::vec3(0.04f, 0.04f, 1.0f));
				glm::mat4 MVP = ortho * model; // No view
				glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
				glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);
				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, redTex); // Red Heart
				box.draw(shader);
			}

			// Boss Health (Top Center) - 10 units
			if (t5BossAlive) {
				for (int i = 0; i < t5BossHealth; i++) {
					glm::mat4 model = glm::mat4(1.0f);
					model = glm::translate(model, glm::vec3(-0.5f + i * 0.1f, 0.9f, 0.0f));
					model = glm::scale(model, glm::vec3(0.04f, 0.08f, 1.0f));
					glm::mat4 MVP = ortho * model;
					glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
					glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &model[0][0]);
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, redTex); // Metal/Grey Bar
					box.draw(shader);
				}
			}

			glEnable(GL_DEPTH_TEST);
			glEnable(GL_CULL_FACE);

			// RESTORE STANDARD SHADER STATE (Camera/Light)
			shader.use();
			glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"), lightPos.x, lightPos.y, lightPos.z);
			glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"), camera.getCameraPosition().x, camera.getCameraPosition().y, camera.getCameraPosition().z);
		}

		//s4 ui
		glm::mat4 orthoUI = glm::ortho(
			-1.0f, 1.0f,
			-1.0f, 1.0f,
			-1.0f, 1.0f
		);

		// ===== TASK UI =====
		float taskWidth = 0.15f + currentTask * 0.05f;

		// Background
		drawUIRect(
			shader, box,
			glm::vec2(-0.8f, 0.85f),
			glm::vec2(0.35f, 0.08f),
			greenTex, // metal background
			orthoUI
		);

		// Progress bar
		drawUIRect(
			shader, box,
			glm::vec2(-0.8f + taskWidth * 0.5f, 0.85f),
			glm::vec2(taskWidth, 0.05f),
			grassTex, // green = progress
			orthoUI
		);
		// ===== PLAYER HP =====
		for (int i = 0; i < t5PlayerLives; i++)
		{
			drawUIRect(
				shader, box,
				glm::vec2(-0.9f + i * 0.1f, -0.9f),
				glm::vec2(0.06f, 0.06f),
				redTex, // red/orange
				orthoUI
			);
		}
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);

		shader.use();
		glUniform3f(
			glGetUniformLocation(shader.getId(), "lightPos"),
			lightPos.x, lightPos.y, lightPos.z
		);
		glUniform3f(
			glGetUniformLocation(shader.getId(), "viewPos"),
			camera.getCameraPosition().x,
			camera.getCameraPosition().y,
			camera.getCameraPosition().z
		);



		// ---------- SKYBOX (DRAW FIRST) ----------
		glDepthMask(GL_FALSE);
		glDepthFunc(GL_LEQUAL);
		glDisable(GL_CULL_FACE);

		skyboxShader.use();

		// Use SAME camera matrices (no translation)
		glm::mat4 view = glm::mat4(glm::mat3(
			glm::lookAt(
				camera.getCameraPosition(),
				camera.getCameraPosition() + camera.getCameraViewDirection(),
				camera.getCameraUp()
			)
		));

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
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
		glDrawArrays(GL_TRIANGLES, 0, 36);
		glBindVertexArray(0);

		// Restore state
		glEnable(GL_CULL_FACE);
		glDepthMask(GL_TRUE);
		glDepthFunc(GL_LESS);
		// ---------- END SKYBOX ----------
		



		//uic real
		// ===== IMGUI FRAME START =====
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		// ===== IMGUI UI =====
		/*ImGui::Begin("TEST", nullptr,
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_AlwaysAutoResize);

		ImGui::Text("IMGUI IS WORKING");
		ImGui::Text("If you see this, rendering is fine.");*/
		ImGui::SetNextWindowPos(ImVec2(20, 20));
		ImGui::SetNextWindowBgAlpha(0.6f);

		ImGui::Begin(
			"HUD",
			nullptr,
			ImGuiWindowFlags_NoTitleBar |
			ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_AlwaysAutoResize |
			ImGuiWindowFlags_NoMove
		);

		// ---- TASK TEXT ----
		ImGui::Text("OBJECTIVE:");
		ImGui::Separator();
		ImGui::TextWrapped("%s", getTaskText(currentTask));

		// ---- PLAYER HP (only when relevant) ----
		if (currentTask == 5 && !t5GameOver)
		{
			ImGui::Separator();
			ImGui::Text("HP: %d", t5PlayerLives);
		}

		// ---- GAME STATES ----
		if (t5GameOver)
		{
			ImGui::Separator();
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "GAME OVER");
		}
		if (t5GameWon)
		{
			ImGui::Separator();
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "YOU WIN!");
		}
		ImGui::End();
		// ===== IMGUI RENDER =====
		ImGui::Render();

		glDisable(GL_DEPTH_TEST);   // <<< THIS IS CRITICAL
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		glEnable(GL_DEPTH_TEST);


		window.update();
	}
}

void processKeyboardInput()
{
	float cameraSpeed = 30 * deltaTime;

	glm::vec3 oldPos = camera.getCameraPosition();

	if (window.isPressed(GLFW_KEY_W))
	{
		camera.keyboardMoveFront(cameraSpeed);
		if (checkTreeCollision(camera.getCameraPosition(), treeColliders))
			camera.setCameraPosition(oldPos);
	}

	if (window.isPressed(GLFW_KEY_S))
	{
		camera.keyboardMoveBack(cameraSpeed);
		if (checkTreeCollision(camera.getCameraPosition(), treeColliders))
			camera.setCameraPosition(oldPos);
	}

	if (window.isPressed(GLFW_KEY_A))
	{
		camera.keyboardMoveLeft(cameraSpeed);
		if (checkTreeCollision(camera.getCameraPosition(), treeColliders))
			camera.setCameraPosition(oldPos);
	}

	if (window.isPressed(GLFW_KEY_D))
	{
		camera.keyboardMoveRight(cameraSpeed);
		if (checkTreeCollision(camera.getCameraPosition(), treeColliders))
			camera.setCameraPosition(oldPos);
	}

	if (window.isPressed(GLFW_KEY_R))
		camera.keyboardMoveUp(cameraSpeed);
	if (window.isPressed(GLFW_KEY_F))
		camera.keyboardMoveDown(cameraSpeed);

	// Jump
	if (window.isPressed(GLFW_KEY_SPACE))
	{
		float groundY =
			getGroundHeight(
				camera.getCameraPosition().x,
				camera.getCameraPosition().z
			) + playerHeight;

		if (camera.getCameraPosition().y <= groundY + 0.01f)
			verticalVelocity = 30.0f;
	}
}


//s1 disabled
// !!!!!!!!!!!!!!keep just in case we need this!!!!!!!!!!!!!!!!!!!!!!!!
////rotation
//if (window.isPressed(GLFW_KEY_LEFT))	
//	camera.rotateOy(cameraSpeed);
//if (window.isPressed(GLFW_KEY_RIGHT))
//	camera.rotateOy(-cameraSpeed);
//if (window.isPressed(GLFW_KEY_UP))
//	camera.rotateOx(cameraSpeed);
//if (window.isPressed(GLFW_KEY_DOWN))
//	camera.rotateOx(-cameraSpeed);
