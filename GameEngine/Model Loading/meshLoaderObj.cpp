#include "meshLoaderObj.h"
#include "stringTokenizer.h"
#include <map>
#include <sstream>
#include "stb_image.h"

MeshLoaderObj::MeshLoaderObj() {};

static std::map<std::string, std::string>
loadMTLMap(const std::string& mtlFile, const std::string& baseDir)
{
	std::ifstream f(mtlFile);
	std::map<std::string, std::string> result;

	if (!f.good())
	{
		std::cout << "MTL not found: " << mtlFile << std::endl;
		return result;
	}

	std::string line, currentMat;

	while (std::getline(f, line))
	{
		std::istringstream iss(line);
		std::string key;
		iss >> key;

		if (key.empty() || key[0] == '#') continue;

		if (key == "newmtl")
		{
			iss >> currentMat;
		}
		else if (key == "map_Kd" && !currentMat.empty())
		{
			std::string tex;
			iss >> tex;
			result[currentMat] = baseDir + tex;
		}
	}

	return result;
}


static GLuint loadTexture(const char* path)
{
	int w, h, ch;
	stbi_set_flip_vertically_on_load(true);

	unsigned char* data = stbi_load(path, &w, &h, &ch, 0);
	if (!data)
	{
		std::cout << "Failed to load texture: " << path << std::endl;
		return 0;
	}

	GLenum format = GL_RGB;
	if (ch == 1) format = GL_RED;
	if (ch == 3) format = GL_RGB;
	if (ch == 4) format = GL_RGBA;


	GLuint tex;
	glGenTextures(1, &tex);
	glBindTexture(GL_TEXTURE_2D, tex);

	glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	stbi_image_free(data);
	return tex;
}


Mesh MeshLoaderObj::loadObj(const std::string &filename)
{
	std::vector<Vertex> vertices;
	std::vector<int> indices;

	std::map<std::string, std::string> matToTexPath;
	std::string currentMaterial;
	std::string baseDir = filename.substr(0, filename.find_last_of("/\\") + 1);


	//Reading Obj file
	std::ifstream file(filename.c_str(), std::ios::in);
	if (!file.good())
	{
		std::cout << "Obj model not found " << filename << std::endl;
			std::terminate();
	}

	std::string line;
	std::vector<std::string> tokens, facetokens;

	std::vector<glm::vec3> positions;
	positions.reserve(200000);

	std::vector<glm::vec3> normals;
	normals.reserve(200000);

	std::vector<glm::vec2> texcoords;
	texcoords.reserve(200000);

	//Parsing obj file
	while (std::getline(file, line))
	{
		_stringTokenize(line, tokens);

		if (tokens.size() == 0)
			continue;

		//Comments
		if (tokens.size()>0 && tokens[0].at(0) == '#')
			continue;

		// ---------- MATERIAL LIB ----------
		if (tokens[0] == "mtllib" && tokens.size() >= 2)
		{
			std::string mtlPath = baseDir + tokens[1];
			matToTexPath = loadMTLMap(mtlPath, baseDir);
			continue;
		}

		// ---------- USE MATERIAL ----------
		if (tokens[0] == "usemtl" && tokens.size() >= 2)
		{
			currentMaterial = tokens[1];
			continue;
		}


		//Vertices
		if (tokens.size()>3 && tokens[0] == "v")
			positions.push_back(glm::vec3(_stringToFloat(tokens[1]), _stringToFloat(tokens[2]), _stringToFloat(tokens[3])));

		//Normals
		if (tokens.size()>3 && tokens[0] == "vn")
			normals.push_back(glm::vec3(_stringToFloat(tokens[1]), _stringToFloat(tokens[2]), _stringToFloat(tokens[3])));

		//Texture Coords
		if (tokens.size()>2 && tokens[0] == "vt")
			texcoords.push_back(glm::vec2(_stringToFloat(tokens[1]), _stringToFloat(tokens[2])));

		//Faces
		if (tokens.size() >= 4 && tokens[0] == "f")
		{
			unsigned int face_format = 0;
			if (tokens[1].find("//") != std::string::npos) face_format = 3;
			_faceTokenize(tokens[1], facetokens);

			if (facetokens.size() == 3)
				face_format = 4;
			else
			{
				if (facetokens.size() == 2)
				{
					if (face_format != 3) face_format = 2;
				}
				else
				{
					face_format = 1;
				}
			}

			unsigned int index_of_first_vertex_of_face = -1;

			for (unsigned int num_token = 1; num_token<tokens.size(); num_token++)
			{
				if (tokens[num_token].at(0) == '#') break;
				_faceTokenize(tokens[num_token], facetokens);

				if (face_format == 1) //Just pos
				{
					int p_index = _stringToInt(facetokens[0]);
					if (p_index>0) p_index -= 1;
					else p_index = positions.size() + p_index;

					vertices.push_back(Vertex(positions[p_index].x, positions[p_index].y, positions[p_index].z));
				}
				else if (face_format == 2) //Pos and texcoords
				{
					int p_index = _stringToInt(facetokens[0]);
					if (p_index>0) p_index -= 1;
					else p_index = positions.size() + p_index;

					int t_index = _stringToInt(facetokens[1]);
					if (t_index>0) t_index -= 1;
					else t_index = texcoords.size() + t_index;

					vertices.push_back(Vertex(positions[p_index].x, positions[p_index].y, positions[p_index].z, texcoords[t_index].x, texcoords[t_index].y));
				}
				else if (face_format == 3)
				{ 
					//Pos and normal
					int p_index = _stringToInt(facetokens[0]);
					if (p_index>0) p_index -= 1;
					else p_index = positions.size() + p_index;

					int n_index = _stringToInt(facetokens[1]);
					if (n_index>0) n_index -= 1;
					else n_index = normals.size() + n_index;

					vertices.push_back(Vertex(positions[p_index].x, positions[p_index].y, positions[p_index].z, normals[n_index].x, normals[n_index].y, normals[n_index].z));
				}
				else
				{
					//Normal and texcoord
					int p_index = _stringToInt(facetokens[0]);
					if (p_index>0) p_index -= 1;
					else p_index = positions.size() + p_index;

					int t_index = _stringToInt(facetokens[1]);
					if (t_index>0) t_index -= 1;
					else t_index = normals.size() + t_index;

					int n_index = _stringToInt(facetokens[2]);
					if (n_index>0) n_index -= 1;
					else n_index = normals.size() + n_index;

					vertices.push_back(Vertex(positions[p_index].x, positions[p_index].y, positions[p_index].z, normals[n_index].x, normals[n_index].y, normals[n_index].z, texcoords[t_index].x, texcoords[t_index].y));
				}

				if (num_token<4)
				{
					if (num_token == 1)
						index_of_first_vertex_of_face = vertices.size() - 1;

					indices.push_back(vertices.size() - 1);
				}
				else
				{
					indices.push_back(index_of_first_vertex_of_face);
					indices.push_back(vertices.size() - 2);
					indices.push_back(vertices.size() - 1);
				}
			}
		}
	}

	std::cout << "Loading:  " << filename << std::endl;

	std::vector<Texture> textures;

	auto it = matToTexPath.find(currentMaterial);
	if (it != matToTexPath.end())
	{
		Texture t;
		t.id = loadTexture(it->second.c_str()); // stb_image
		t.type = "texture_diffuse";
		textures.push_back(t);
	}

	Mesh mesh(vertices, indices);
	mesh.setTextures(textures);
	return mesh;

}

Mesh MeshLoaderObj::loadObj(const std::string &filename, std::vector<Texture> textures)
{
	Mesh mesh = loadObj(filename);
	mesh.setTextures(textures);



	return mesh;
}

std::vector<Mesh> MeshLoaderObj::loadObjMulti(const std::string &filename)
{
	struct SubmeshData
	{
		std::vector<Vertex> vertices;
		std::vector<int> indices;
		std::string material;
	};

	std::map<std::string, SubmeshData> submeshes;
	std::map<std::string, std::string> matToTexPath;
	std::string currentMaterial = "default";
	std::string baseDir = filename.substr(0, filename.find_last_of("/\\") + 1);

	std::ifstream file(filename.c_str(), std::ios::in);
	if (!file.good())
	{
		std::cout << "Obj model not found " << filename << std::endl;
		std::terminate();
	}

	std::string line;
	std::vector<std::string> tokens, facetokens;

	std::vector<glm::vec3> positions;
	positions.reserve(200000);

	std::vector<glm::vec3> normals;
	normals.reserve(200000);

	std::vector<glm::vec2> texcoords;
	texcoords.reserve(200000);

	auto& getSubmesh = [&](const std::string& mat) -> SubmeshData&
	{
		auto& sm = submeshes[mat];
		sm.material = mat;
		return sm;
	};

	while (std::getline(file, line))
	{
		_stringTokenize(line, tokens);

		if (tokens.size() == 0)
			continue;

		if (tokens.size() > 0 && tokens[0].at(0) == '#')
			continue;

		if (tokens[0] == "mtllib" && tokens.size() >= 2)
		{
			std::string mtlPath = baseDir + tokens[1];
			matToTexPath = loadMTLMap(mtlPath, baseDir);
			continue;
		}

		if (tokens[0] == "usemtl" && tokens.size() >= 2)
		{
			currentMaterial = tokens[1];
			continue;
		}

		if (tokens.size() > 3 && tokens[0] == "v")
			positions.push_back(glm::vec3(_stringToFloat(tokens[1]), _stringToFloat(tokens[2]), _stringToFloat(tokens[3])));

		if (tokens.size() > 3 && tokens[0] == "vn")
			normals.push_back(glm::vec3(_stringToFloat(tokens[1]), _stringToFloat(tokens[2]), _stringToFloat(tokens[3])));

		if (tokens.size() > 2 && tokens[0] == "vt")
			texcoords.push_back(glm::vec2(_stringToFloat(tokens[1]), _stringToFloat(tokens[2])));

		if (tokens.size() >= 4 && tokens[0] == "f")
		{
			unsigned int face_format = 0;
			if (tokens[1].find("//") != std::string::npos) face_format = 3;
			_faceTokenize(tokens[1], facetokens);

			if (facetokens.size() == 3)
				face_format = 4;
			else
			{
				if (facetokens.size() == 2)
				{
					if (face_format != 3) face_format = 2;
				}
				else
				{
					face_format = 1;
				}
			}

			SubmeshData& sm = getSubmesh(currentMaterial);
			unsigned int index_of_first_vertex_of_face = -1;

			for (unsigned int num_token = 1; num_token < tokens.size(); num_token++)
			{
				if (tokens[num_token].at(0) == '#') break;
				_faceTokenize(tokens[num_token], facetokens);

				if (face_format == 1)
				{
					int p_index = _stringToInt(facetokens[0]);
					if (p_index > 0) p_index -= 1;
					else p_index = positions.size() + p_index;

					sm.vertices.push_back(Vertex(positions[p_index].x, positions[p_index].y, positions[p_index].z));
				}
				else if (face_format == 2)
				{
					int p_index = _stringToInt(facetokens[0]);
					if (p_index > 0) p_index -= 1;
					else p_index = positions.size() + p_index;

					int t_index = _stringToInt(facetokens[1]);
					if (t_index > 0) t_index -= 1;
					else t_index = texcoords.size() + t_index;

					sm.vertices.push_back(Vertex(positions[p_index].x, positions[p_index].y, positions[p_index].z, texcoords[t_index].x, texcoords[t_index].y));
				}
				else if (face_format == 3)
				{
					int p_index = _stringToInt(facetokens[0]);
					if (p_index > 0) p_index -= 1;
					else p_index = positions.size() + p_index;

					int n_index = _stringToInt(facetokens[1]);
					if (n_index > 0) n_index -= 1;
					else n_index = normals.size() + n_index;

					sm.vertices.push_back(Vertex(positions[p_index].x, positions[p_index].y, positions[p_index].z, normals[n_index].x, normals[n_index].y, normals[n_index].z));
				}
				else
				{
					int p_index = _stringToInt(facetokens[0]);
					if (p_index > 0) p_index -= 1;
					else p_index = positions.size() + p_index;

					int t_index = _stringToInt(facetokens[1]);
					if (t_index > 0) t_index -= 1;
					else t_index = normals.size() + t_index;

					int n_index = _stringToInt(facetokens[2]);
					if (n_index > 0) n_index -= 1;
					else n_index = normals.size() + n_index;

					sm.vertices.push_back(Vertex(positions[p_index].x, positions[p_index].y, positions[p_index].z, normals[n_index].x, normals[n_index].y, normals[n_index].z, texcoords[t_index].x, texcoords[t_index].y));
				}

				if (num_token < 4)
				{
					if (num_token == 1)
						index_of_first_vertex_of_face = sm.vertices.size() - 1;

					sm.indices.push_back(sm.vertices.size() - 1);
				}
				else
				{
					sm.indices.push_back(index_of_first_vertex_of_face);
					sm.indices.push_back(sm.vertices.size() - 2);
					sm.indices.push_back(sm.vertices.size() - 1);
				}
			}
		}
	}

	std::cout << "Loading (multi):  " << filename << std::endl;

	std::vector<Mesh> meshes;
	meshes.reserve(submeshes.size());

	for (auto& kv : submeshes)
	{
		SubmeshData& sm = kv.second;
		if (sm.indices.empty())
			continue;

		std::vector<Texture> textures;
		auto it = matToTexPath.find(sm.material);
		if (it != matToTexPath.end())
		{
			GLuint texId = loadTexture(it->second.c_str());
			if (texId != 0)
			{
				Texture t;
				t.id = texId;
				t.type = "texture_diffuse";
				textures.push_back(t);
			}
		}

		meshes.emplace_back(sm.vertices, sm.indices, textures);
	}

	return meshes;
}

