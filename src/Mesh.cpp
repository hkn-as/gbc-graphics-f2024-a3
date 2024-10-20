#define PAR_SHAPES_IMPLEMENTATION
#define FAST_OBJ_IMPLEMENTATION
#include <par_shapes.h>
#include <fast_obj.h>
#include "Mesh.h"
#include <cassert>
#include <cstdio>

void Upload(Mesh* mesh);
std::vector<Vector3> vertices = {
	{ 0.5f, -0.5f, 0.0f },
	{ -0.5f, -0.5f, 0.0f },
	{ 0.0f,  0.5f, 0.0f }
};

void CreateMesh(Mesh* mesh, const char* path)
{
	fastObjMesh* obj = fast_obj_read(path);
	int count = obj->index_count;
	mesh->positions.resize(count);
	mesh->normals.resize(count);
	
	assert(obj->position_count > 1);
	for (int i = 0; i < count; i++)
	{
		// Using the obj file's indices, populate the mesh->positions with the object's vertex positions
		fastObjIndex idx = obj->indices[i];
	}
	
	assert(obj->normal_count > 1);
	for (int i = 0; i < count; i++)
	{
		// Using the obj file's indices, populate the mesh->normals with the object's vertex normals
		fastObjIndex idx = obj->indices[i];
	}
	
	if (obj->texcoord_count > 1)
	{
		mesh->tcoords.resize(count);
		for (int i = 0; i < count; i++)
		{
			// Using the obj file's indices, populate the mesh->tcoords with the object's vertex texture coordinates
			fastObjIndex idx = obj->indices[i];
		}
	}
	else
	{
		printf("**Warning: mesh %s loaded without texture coordinates**\n", path);
	}
	fast_obj_destroy(obj);
	mesh->count = count;

	Upload(mesh);
}

void CreateMesh(Mesh* mesh, ShapeType shape)
{
	// 1. Generate par_shapes_mesh
	par_shapes_mesh* par = nullptr;
	switch (shape)
	{
	case PLANE:
		par = par_shapes_create_plane(1, 1);
		break;

	case CUBE:
		par = par_shapes_create_cube();
		break;

	case SPHERE:
		par = par_shapes_create_subdivided_sphere(1);
		break;

	default:
		assert(false, "Invalid shape type");
		break;
	}
	
	par_shapes_compute_normals(par);

	// 2. Convert par_shapes_mesh to our Mesh representation
	int count = par->ntriangles * 3;	// 3 points per triangle
	mesh->count = count;
	mesh->indices.resize(count);
	memcpy(mesh->indices.data(), par->triangles, count * sizeof(uint16_t));
	mesh->positions.resize(par->npoints);
	memcpy(mesh->positions.data(), par->points, par->npoints * sizeof(Vector3));
	mesh->normals.resize(par->npoints);
	memcpy(mesh->normals.data(), par->normals, par->npoints * sizeof(Vector3));
	if (par->tcoords != nullptr)
	{
		mesh->tcoords.resize(par->npoints);
		memcpy(mesh->tcoords.data(), par->tcoords, par->npoints * sizeof(Vector2));
	}
	else
	{
		printf("Warning: Mesh generated without tcoords!\n");
	}
	par_shapes_free_mesh(par);

	// 3. Upload Mesh to GPU
	Upload(mesh);
}

void DestroyMesh(Mesh* mesh)
{

}

void DrawMesh(const Mesh& mesh) {
	glBindVertexArray(mesh.vao);

	if (mesh.ebo != GL_NONE) {
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
		glDrawElements(GL_TRIANGLES, mesh.count, GL_UNSIGNED_SHORT, nullptr);
	}
	else {
		glDrawArrays(GL_TRIANGLES, 0, mesh.count);
	}

	glBindVertexArray(0);  // Unbind the VAO
}
void SetupMesh(Mesh* mesh)
{
	glBindBuffer(GL_ARRAY_BUFFER, mesh->yourBufferID);  // Bind the buffer before updating
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(&mesh->data), &mesh->data);  // Then update it

	// Generate VAO
	glGenVertexArrays(1, &mesh->vao);
	glBindVertexArray(mesh->vao);

	// Generate VBO
	glGenBuffers(1, &mesh->vbo);
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);

	//
	glGenBuffers(1, &mesh->yourBufferID);


	// Upload vertex data to VBO (replace `vertices` with actual data)
	glBufferData(GL_ARRAY_BUFFER, mesh->positions.size() * sizeof(Vector3), mesh->positions.data(), GL_STATIC_DRAW);

	// Specify the vertex attributes (position in this case)
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), (void*)0);
	glEnableVertexAttribArray(0);

	// Unbind the VAO
	glBindVertexArray(0);
}


void Upload(Mesh* mesh) {
	GLuint vao, nbo, tbo;
	GLuint pbo = 0;  // Initialize pbo (or set to GL_NONE)
	glGenBuffers(1, &pbo);  // Generate pbo
	glBindBuffer(GL_ARRAY_BUFFER, pbo);
	glBufferData(GL_ARRAY_BUFFER, mesh->positions.size() * sizeof(Vector3), mesh->positions.data(), GL_STATIC_DRAW);
	GLuint ebo = 0;  // Ensure ebo is initialized
	glGenBuffers(1, &ebo);  // Generate the buffer
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);  // Bind it before usage
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size() * sizeof(uint16_t), mesh->indices.data(), GL_STATIC_DRAW);  // Upload the data


	// Generate and bind VAO
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Generate and bind VBO for positions
	glGenBuffers(1, &mesh->vbo);  // Use the mesh's vbo
	glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
	glBufferData(GL_ARRAY_BUFFER, mesh->positions.size() * sizeof(Vector3), mesh->positions.data(), GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), nullptr);
	glEnableVertexAttribArray(0);

	// Generate and bind VBO for normals (if available)
	if (!mesh->normals.empty()) {
		glGenBuffers(1, &nbo);
		glBindBuffer(GL_ARRAY_BUFFER, nbo);
		glBufferData(GL_ARRAY_BUFFER, mesh->normals.size() * sizeof(Vector3), mesh->normals.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vector3), nullptr);
		glEnableVertexAttribArray(1);
	}

	// Generate and bind VBO for texture coordinates (if available)
	if (!mesh->tcoords.empty()) {
		glGenBuffers(1, &tbo);
		glBindBuffer(GL_ARRAY_BUFFER, tbo);
		glBufferData(GL_ARRAY_BUFFER, mesh->tcoords.size() * sizeof(Vector2), mesh->tcoords.data(), GL_STATIC_DRAW);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vector2), nullptr);
		glEnableVertexAttribArray(2);
	}

	// Generate and bind EBO for indices (if available)
	if (!mesh->indices.empty()) {
		glGenBuffers(1, &ebo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->indices.size() * sizeof(uint16_t), mesh->indices.data(), GL_STATIC_DRAW);
	}

	// Unbind VAO and buffers
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// Store the buffer objects in the mesh structure
	mesh->vao = vao;
	mesh->pbo = pbo;
	mesh->nbo = nbo;
	mesh->tbo = tbo;
	mesh->ebo = ebo;
}

