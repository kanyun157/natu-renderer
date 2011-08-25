#include "EBO.h"


EBO::EBO(void)
{
}


EBO::~EBO(void)
{
	
}

bool EBO::save(string filename)
{
	return false;
}
bool EBO::load(string filename)
{
	return false;
}


void EBO::linkVBO(VBO * _vbo)
{
	vbo = _vbo;
}

void EBO::create(GLenum _dataType, GLenum _primitiveType, int _indicesCount, const void * _data, GLenum _usage)
{
	data			= _data;
	dataType		= _dataType;
	primitiveType	= _primitiveType;
	indicesCount	= _indicesCount;
	usage			= _usage;
	glGenBuffers(1, &id);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indicesCount*sizeof(GLuint), data, usage);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void EBO::draw(Shader * shader)
{
	if (shader!=NULL)
		shader->use(true);
	// enable 
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, id);
		vbo->bind(shader);
		GLuint t = GL_UNSIGNED_INT;
		GLuint p = GL_TRIANGLES;
		glDrawElements(primitiveType, indicesCount, dataType, BUFFER_OFFSET(0));		   
		vbo->unbind(shader);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	if (shader!=NULL)
		shader->use(false);
}
