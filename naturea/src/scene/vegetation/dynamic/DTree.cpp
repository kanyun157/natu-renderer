#include "DTree.h"


DTree::DTree(TextureManager *texManager, ShaderManager *shManager):Vegetation(texManager, shManager)
{
	branchShader			 = NULL;
	leafShader				 = NULL;
	bLODShader				 = NULL;
	lLODShader				 = NULL;
							 
	dataTexture				 = NULL;
	lColorTexture			 = NULL;
	frontDecalMap			 = NULL;
	frontNormalMap			 = NULL;
	frontTranslucencyMap	 = NULL;
	frontHalfLife2Map		 = NULL;
	backDecalMap			 = NULL;
	backNormalMap			 = NULL;
	backTranslucencyMap		 = NULL;
	backHalfLife2Map		 = NULL;
							 
	bColorTexture			 = NULL;
	bNormalTexture			 = NULL;
							
	branchNoiseTexture		 = NULL;
	leafNoiseTexture		 = NULL;

	branchesVBO				 = NULL;
	leavesVBO				 = NULL;
	branchesEBO				 = NULL;
}					
DTree::DTree(DTree* copy):
Vegetation(copy->textureManager, copy->shaderManager)
{
	//	textureId	= copy->textureId;
	//	vboId		= copy->vboId;
	//	shader		= copy->shader;
	//	shaderId	= copy->shaderId;
	//	LCmatrixLoc = copy->LCmatrixLoc;
	//	fastModeLoc	= copy->fastModeLoc;
	//	shadowMappingEnabledLoc = copy->shadowMappingEnabledLoc;


}

DTree::~DTree(void)
{
	SAFE_DELETE_PTR(	branchShader			);
	SAFE_DELETE_PTR(	leafShader				);
	SAFE_DELETE_PTR(	bLODShader				);
	SAFE_DELETE_PTR(	lLODShader				);

	SAFE_DELETE_PTR(	dataTexture				);
	SAFE_DELETE_PTR(	lColorTexture			);
	SAFE_DELETE_PTR(	frontDecalMap			);
	SAFE_DELETE_PTR(	frontNormalMap			);
	SAFE_DELETE_PTR(	frontTranslucencyMap	);
	SAFE_DELETE_PTR(	frontHalfLife2Map		);
	SAFE_DELETE_PTR(	backDecalMap			);
	SAFE_DELETE_PTR(	backNormalMap			);
	SAFE_DELETE_PTR(	backTranslucencyMap		);
	SAFE_DELETE_PTR(	backHalfLife2Map		);

	SAFE_DELETE_PTR(	bColorTexture			);
	SAFE_DELETE_PTR(	bNormalTexture			);

	SAFE_DELETE_PTR(	branchNoiseTexture		);
	SAFE_DELETE_PTR(	leafNoiseTexture		);

	SAFE_DELETE_PTR(	branchesVBO				);
	SAFE_DELETE_PTR(	leavesVBO				);
	SAFE_DELETE_PTR(	branchesEBO				);

	int i = 0;
	for (i = 0; i< slices.size(); i++){
		SAFE_DELETE_PTR( slices[i] );
	}
	slices.clear();
	for (i = 0; i< branches.size(); i++){
		SAFE_DELETE_PTR( branches[i] );
	}
	branches.clear();
	for (i = 0; i< leaves.size(); i++){
		SAFE_DELETE_PTR( leaves[i] );
	}
	leaves.clear();

	//SAFE_DELETE_PTR( trunk );
	

}

bool DTree::loadOBJT(string filename)
{
	printf(" DYN_TREE load %s\n", filename.c_str());
	OBJTfile file;
	file.loadFromFile(filename.c_str());

	// setup bbox 
	//TODO: get proper coords of bbox from OBJT / model itself
	bbox = getBBox();

	// process branches
	map <int, DTreeBranch*> m_branches;
	map <int, StEntity>::iterator iter;
	DTreeBranch *parent = NULL;
	CoordSystem cs;
	CoordSystem objectCS;
	float x;
	float length, r1, r2, c2=0.3326, c4=0.398924;
	int divT=2, divR=4;
	v3 motionVector(1.0, 1.0, 1.0);
	StEntity entity;
	// get trunk (id = parentId)
	int id=0;
	branchCount = 0;
	for ( iter=file.branches.begin() ; iter != file.branches.end(); iter++ ){
		parent	= NULL;
		id		= (*iter).first;
		entity	= (*iter).second;
		branchCount++;

		// parent
		if (entity.isSetParent && entity.id!=entity.parentId){
			parent = m_branches[entity.parentId];
		} else {
			parent = NULL;
		}
		// length
		if (entity.isSetLength){
			length = entity.length / file.maxLength;
		} else {
			if (entity.isSetLevel){
				length = 1.f/(entity.level*3.f+1.f);
			} else {
				length = 1.0;
			}
		}
		// x
		if (entity.isSetX){
			x = entity.x;
		} else {
			x = 0.5;
		}
		// base vectors
		if (entity.isSetBase){
			cs.r = entity.base[0];
			cs.s = entity.base[1];	
			cs.t = entity.base[2];
			objectCS.r = entity.base[0];
			objectCS.s = entity.base[1];
			objectCS.t = entity.base[2];
			cs.normalize();
			cs.repair();
		} else {
			cs.r = v3(1.0, 0.0, 0.0);
			cs.s = v3(0.0, 1.0, 0.0);	
			cs.t = v3(0.0, 0.0, 1.0);
			objectCS.r = v3(1.0, 0.0, 0.0);
			objectCS.s = v3(0.0, 1.0, 0.0);
			objectCS.t = v3(0.0, 0.0, 1.0);
		}

		// origin
		if (entity.isSetOrigin){
			cs.origin = entity.origin;
			// recalculate origin
			if (parent!=NULL){
				// TODO!!!! get position of origin{branch space} from origin{obj space} 
			}
		} else {
			if (parent!=NULL){
				cs.origin = v3(0.0, 0.0, 1.0) * x * parent->L;
			} else {
				cs.origin = v3(0.f, 0.f, 0.f);
			}
		}
		// r1
		if (entity.isSetR1){
			r1 = entity.r1;
		} else {		
			r1 = 0.02*length;
		}
		// r2
		if (entity.isSetR2){
			r2 = entity.r2;
		} else {		
			r2 = 0.0001;
		}
		// add branch...	
		m_branches[id] = new DTreeBranch(parent, cs, x, length, r1, r2);  
		m_branches[id]->id = id;
	} // for each entity in map
	// set trunk
	trunk = m_branches[0];
	trunk->level = 0;
	int leafCnt = 0;
	// process leaves
	for ( iter=file.leaves.begin() ; iter != file.leaves.end(); iter++ ){
		id		= (*iter).first;
		entity	= (*iter).second;
		leafCnt++;
		// parent
		if (entity.isSetParent){
			parent = m_branches[entity.parentId];
		} else {
			parent = NULL;
		}

		// leaf X position
		if (entity.isSetX){
			x = entity.x;
		} else {
			x = 0.5f;
		}

		// leaf size
		float size;
		if (entity.isSetLength){
			size = entity.length;
		} else {
			size = DYN_TREE::LEAF_SIZE;
		}

		// base vectors
		if (entity.isSetBase){
			cs.r = entity.base[0];
			cs.s = entity.base[1];	
			cs.t = entity.base[2];
			objectCS.r = entity.base[0];
			objectCS.s = entity.base[1];
			objectCS.t = entity.base[2];
			cs.normalize();
			cs.repair();

		} else {
			cs.r = v3(1.0, 0.0, 0.0);
			cs.s = v3(0.0, 1.0, 0.0);	
			cs.t = v3(0.0, 0.0, 1.0);
			objectCS.r = v3(1.0, 0.0, 0.0);
			objectCS.s = v3(0.0, 1.0, 0.0);
			objectCS.t = v3(0.0, 0.0, 1.0);
		}

		// create leaf
		DTreeLeaf * leaf = new DTreeLeaf(parent, cs, x, size);
		// add to container
		leaves.push_back(leaf);
	}

	printf("DYN_TREE load successfull (branches: %d, leaves: %d)\n", branchCount, leafCnt);
	return true;
}

void DTree::createVBOs()
{
	// TODO:
	// create VBO for branch vertices
	createBranchesVBO();

	// create VBO for leaf vertices
	createLeavesVBO();
}

void DTree::createBranchesVBO()
{
	int i, sizeB = branches.size();
	int vCnt = 0, iCnt = 0;
	// count total vertices & indices
	for (i=0; i<sizeB; i++){
		vCnt += branches[i]->getVertexCount();
		iCnt += branches[i]->getIndicesCount();
	}
	int * sizes = new int[DYN_TREE::VERTEX_ATTRIBUTES::COUNT];
	// get sizes
	for (i = 0; i < DYN_TREE::VERTEX_ATTRIBUTES::COUNT; i++){
		sizes[i]=0;
	}
	sizes[DYN_TREE::VERTEX_ATTRIBUTES::POSITION]	= 3;
	sizes[DYN_TREE::VERTEX_ATTRIBUTES::NORMAL]		= 3;
	sizes[DYN_TREE::VERTEX_ATTRIBUTES::TANGENT]		= 3;
	sizes[DYN_TREE::VERTEX_ATTRIBUTES::TEXCOORD0]	= 2;
	//sizes[DYN_TREE::VERTEX_ATTRIBUTES::TEXCOORD1]	= 2;
	sizes[DYN_TREE::VERTEX_ATTRIBUTES::BRANCH_INDEX]= 1;
	sizes[DYN_TREE::VERTEX_ATTRIBUTES::WEIGHT]		= 4;
	int totalVertexSize = 0;
	for (i = 0; i < DYN_TREE::VERTEX_ATTRIBUTES::COUNT; i++){
		vbo_data[i]= new GLfloat[vCnt*sizes[i]];
		totalVertexSize+=sizes[i];
	}
	// ebo
	ebo_data = new GLuint	[iCnt];

	// fill arrays
	int k, offset;
	int indexPtr = 0, dataPtr=0, v=0, id=0;
	int indexOffset = 0;
	Vertex *vertex;
	int index = 0;
	for (i=0; i<sizeB; i++)// each branch 
	{
		id = branches[i]->id;
		offset = dataPtr;
		for (k=0; k<branches[i]->vertices.size(); k++){
			vertex = branches[i]->vertices[k];
			// position
			index = DYN_TREE::VERTEX_ATTRIBUTES::POSITION;
			vbo_data[index][dataPtr*sizes[index] + 0] = vertex->bPos.data[ 0 ];
			vbo_data[index][dataPtr*sizes[index] + 1] = vertex->bPos.data[ 1 ];
			vbo_data[index][dataPtr*sizes[index] + 2] = vertex->bPos.data[ 2 ];

			// normal
			index = DYN_TREE::VERTEX_ATTRIBUTES::NORMAL;
			vbo_data[index][dataPtr*sizes[index] + 0] = vertex->normal.data[ 0 ];
			vbo_data[index][dataPtr*sizes[index] + 1] = vertex->normal.data[ 1 ];
			vbo_data[index][dataPtr*sizes[index] + 2] = vertex->normal.data[ 2 ];

			// tangent
			index = DYN_TREE::VERTEX_ATTRIBUTES::TANGENT;
			vbo_data[index][dataPtr*sizes[index] + 0] = vertex->tangent.data[ 0 ];
			vbo_data[index][dataPtr*sizes[index] + 1] = vertex->tangent.data[ 1 ];
			vbo_data[index][dataPtr*sizes[index] + 2] = vertex->tangent.data[ 2 ];

			// texcoord0
			index = DYN_TREE::VERTEX_ATTRIBUTES::TEXCOORD0;
			vbo_data[index][dataPtr*sizes[index] + 0] = vertex->textureCoords.data[ 0 ];
			vbo_data[index][dataPtr*sizes[index] + 1] = vertex->textureCoords.data[ 1 ];

			// xvals
			index = DYN_TREE::VERTEX_ATTRIBUTES::WEIGHT;
			vbo_data[index][dataPtr*sizes[index] + 0] = vertex->x[ 0 ];
			vbo_data[index][dataPtr*sizes[index] + 1] = vertex->x[ 1 ];
			vbo_data[index][dataPtr*sizes[index] + 2] = vertex->x[ 2 ];
			vbo_data[index][dataPtr*sizes[index] + 3] = vertex->x[ 3 ];

			// branch index
			index = DYN_TREE::VERTEX_ATTRIBUTES::BRANCH_INDEX;
			vbo_data[index][dataPtr + 0] = id+0.5f;
			dataPtr++;
		}
		// copy branch indices...
		int * branchIndices = branches[i]->indexPtr;
		int branchIndexCnt = branches[i]->indicesCount;

		for (k=0; k<branchIndexCnt; k++){
			ebo_data[indexPtr] = branchIndices[k]+offset; 
			indexPtr++;
		}
	}
	// create Buffers...

	// VBO
	branchesVBO = new VBO();
	branchesVBO->setVertexCount(vCnt);
	// position
	VBODataSet * dataSet = new VBODataSet(
		vbo_data[DYN_TREE::VERTEX_ATTRIBUTES::POSITION],
		sizes[DYN_TREE::VERTEX_ATTRIBUTES::POSITION]*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::POSITION,
		true
		);
	branchesVBO->addVertexAttribute( dataSet );
	// normal
	dataSet = new VBODataSet(
		vbo_data[DYN_TREE::VERTEX_ATTRIBUTES::NORMAL],
		sizes[DYN_TREE::VERTEX_ATTRIBUTES::NORMAL]*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::NORMAL,
		false
		);
	branchesVBO->addVertexAttribute( dataSet );

	// tangent
	dataSet = new VBODataSet(
		vbo_data[DYN_TREE::VERTEX_ATTRIBUTES::TANGENT],
		sizes[DYN_TREE::VERTEX_ATTRIBUTES::TANGENT]*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::TANGENT,
		false
		);
	branchesVBO->addVertexAttribute( dataSet );

	// texCoord0
	dataSet = new VBODataSet(
		vbo_data[DYN_TREE::VERTEX_ATTRIBUTES::TEXCOORD0],
		sizes[DYN_TREE::VERTEX_ATTRIBUTES::TEXCOORD0]*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::TEXCOORD0,
		false
		);
	branchesVBO->addVertexAttribute( dataSet );

	// weights
	dataSet = new VBODataSet(
		vbo_data[DYN_TREE::VERTEX_ATTRIBUTES::WEIGHT],
		sizes[DYN_TREE::VERTEX_ATTRIBUTES::WEIGHT]*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::WEIGHT,
		false
		);	
	branchesVBO->addVertexAttribute( dataSet );

	// branch index
	dataSet = new VBODataSet(
		vbo_data[DYN_TREE::VERTEX_ATTRIBUTES::BRANCH_INDEX],
		sizes[DYN_TREE::VERTEX_ATTRIBUTES::BRANCH_INDEX]*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::BRANCH_ID,
		false
		);	
	branchesVBO->addVertexAttribute( dataSet );

	branchesVBO->compileData(GL_STATIC_DRAW);

	// element buffer object
	branchesEBO = new EBO();
	branchesEBO->create(GL_UNSIGNED_INT, GL_TRIANGLES, iCnt, ebo_data, GL_STATIC_DRAW);
	branchesEBO->linkVBO(branchesVBO);

}

void DTree::createLeavesVBO()
{
	// recalc coord systems...
	int i, sizeL = leaves.size();

	DTreeLeaf * leaf;
	for (i=0; i<sizeL; i++){
		leaf = leaves[i];
		leaf->init();
		if (leaf->parent!=NULL){
			leaf->cs = leaf->parent->originalCS.getSystemInThisSystem(leaf->originalCS);
		}
	}
	int vCnt = 0;
	// count total vertices & indices
	for (i=0; i<sizeL; i++){
		vCnt += leaves[i]->getVertexCount();
	}
	//leafVerticesCount = vCnt;
	// get sizes
	int * sizes = new int [DYN_TREE::COUNT];
	for (i = 0; i < DYN_TREE::COUNT; i++){
		sizes[i]=0;
	}
	sizes[DYN_TREE::POSITION	]	= 3;
	sizes[DYN_TREE::NORMAL		]	= 3;
	sizes[DYN_TREE::TANGENT		]	= 3;
	sizes[DYN_TREE::TEXCOORD0	]	= 2;
	sizes[DYN_TREE::BRANCH_INDEX]	= 1;
	sizes[DYN_TREE::WEIGHT		]	= 4;

	int totalVertexSize = 0;

	for (i = 0; i < DYN_TREE::COUNT; i++){
		vbo_data[i]= new GLfloat[vCnt*sizes[i]];
		totalVertexSize+=sizes[i];
	}

	// fill arrays
	int k;
	int indexPtr = 0, dataPtr=0, v=0, id=0;
	int indexOffset = 0;
	int index = 0;
	Vertex *vertex;
	for (i=0; i<sizeL; i++) // each leaf 
	{
		leaf = leaves[i];
		id = leaf->parentID;
		for (k=0; k<leaf->vertices.size(); k++){
			vertex = leaf->vertices[k];
			// position
			index = DYN_TREE::VERTEX_ATTRIBUTES::POSITION;
			vbo_data[index][dataPtr*sizes[index] + 0] = vertex->bPos.data[ 0 ];
			vbo_data[index][dataPtr*sizes[index] + 1] = vertex->bPos.data[ 1 ];
			vbo_data[index][dataPtr*sizes[index] + 2] = vertex->bPos.data[ 2 ];

			// normal
			index = DYN_TREE::VERTEX_ATTRIBUTES::NORMAL;
			vbo_data[index][dataPtr*sizes[index] + 0] = vertex->normal.data[ 0 ];
			vbo_data[index][dataPtr*sizes[index] + 1] = vertex->normal.data[ 1 ];
			vbo_data[index][dataPtr*sizes[index] + 2] = vertex->normal.data[ 2 ];

			// tangent
			index = DYN_TREE::VERTEX_ATTRIBUTES::TANGENT;
			vbo_data[index][dataPtr*sizes[index] + 0] = vertex->tangent.data[ 0 ];
			vbo_data[index][dataPtr*sizes[index] + 1] = vertex->tangent.data[ 1 ];
			vbo_data[index][dataPtr*sizes[index] + 2] = vertex->tangent.data[ 2 ];

			// texcoord0
			index = DYN_TREE::VERTEX_ATTRIBUTES::TEXCOORD0;
			vbo_data[index][dataPtr*sizes[index] + 0] = vertex->textureCoords.data[ 0 ];
			vbo_data[index][dataPtr*sizes[index] + 1] = vertex->textureCoords.data[ 1 ];

			// xvals
			index = DYN_TREE::VERTEX_ATTRIBUTES::WEIGHT;
			vbo_data[index][dataPtr*sizes[index] + 0] = vertex->x[ 0 ];
			vbo_data[index][dataPtr*sizes[index] + 1] = vertex->x[ 1 ];
			vbo_data[index][dataPtr*sizes[index] + 2] = vertex->x[ 2 ];
			vbo_data[index][dataPtr*sizes[index] + 3] = vertex->x[ 3 ];

			// branch index
			index = DYN_TREE::VERTEX_ATTRIBUTES::BRANCH_INDEX;
			vbo_data[index][dataPtr + 0] = id+0.5f;

			dataPtr++;
		}
	}

	// create VBO
	leavesVBO = new VBO();
	leavesVBO->setVertexCount(vCnt);
	// position
	VBODataSet * dataSet = new VBODataSet(
		vbo_data[DYN_TREE::VERTEX_ATTRIBUTES::POSITION],
		sizes[DYN_TREE::VERTEX_ATTRIBUTES::POSITION]*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::POSITION,
		true
		);
	leavesVBO->addVertexAttribute( dataSet );
	// normal
	dataSet = new VBODataSet(
		vbo_data[DYN_TREE::VERTEX_ATTRIBUTES::NORMAL],
		sizes[DYN_TREE::VERTEX_ATTRIBUTES::NORMAL]*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::NORMAL,
		false
		);
	leavesVBO->addVertexAttribute( dataSet );

	// tangent
	dataSet = new VBODataSet(
		vbo_data[DYN_TREE::VERTEX_ATTRIBUTES::TANGENT],
		sizes[DYN_TREE::VERTEX_ATTRIBUTES::TANGENT]*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::TANGENT,
		false
		);
	leavesVBO->addVertexAttribute( dataSet );

	// texCoord0
	dataSet = new VBODataSet(
		vbo_data[DYN_TREE::VERTEX_ATTRIBUTES::TEXCOORD0],
		sizes[DYN_TREE::VERTEX_ATTRIBUTES::TEXCOORD0]*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::TEXCOORD0,
		false
		);
	leavesVBO->addVertexAttribute( dataSet );

	// weights
	dataSet = new VBODataSet(
		vbo_data[DYN_TREE::VERTEX_ATTRIBUTES::WEIGHT],
		sizes[DYN_TREE::VERTEX_ATTRIBUTES::WEIGHT]*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::WEIGHT,
		false
		);	
	leavesVBO->addVertexAttribute( dataSet );

	// branch index
	dataSet = new VBODataSet(
		vbo_data[DYN_TREE::VERTEX_ATTRIBUTES::BRANCH_INDEX],
		sizes[DYN_TREE::VERTEX_ATTRIBUTES::BRANCH_INDEX]*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::BRANCH_ID,
		false
		);	
	leavesVBO->addVertexAttribute( dataSet );

	leavesVBO->compileData(GL_STATIC_DRAW);

}

void DTree::createDataTexture()
{
	// linearize structure
	texDimX = linearizeHierarchy();
	int branch_count = texDimX;
	this->branchCountF = branch_count;
	texDimY = 18; // num of data rows  [4 floats]

	recalcCoordSystems();

	fillParentDataForEachBranch();

	DTreeBranch * b;
	int channels = 4, ch, bh;
	float *data = new float[texDimX*texDimY*channels];
	int k;
	for (int i=0; i<texDimX*texDimY*channels; i=i+texDimY*channels){
		k=0;
		b = branches[i/(texDimY*channels)];
		for (ch = 0; ch<channels; ch++){
			data[i + k*channels + ch] = 0.0f;
		}
		// motion vectors
		k = 0;
		data[i + k*channels + 0] = b->motionVectors[0].x;
		data[i + k*channels + 1] = b->motionVectors[0].y;
		data[i + k*channels + 2] = b->motionVectors[1].x;
		data[i + k*channels + 3] = b->motionVectors[1].y;
		k = 1;
		data[i + k*channels + 0] = b->motionVectors[2].x;
		data[i + k*channels + 1] = b->motionVectors[2].y;
		data[i + k*channels + 2] = b->motionVectors[3].x;
		data[i + k*channels + 3] = b->motionVectors[3].y;

		// up0_l0 - up2_l2
		k = 2;
		for (bh = 0;bh < DYN_TREE::MAX_HIERARCHY_DEPTH; bh++){
			data[i + k*channels + 0] = b->upVectors[bh].x;
			data[i + k*channels + 1] = b->upVectors[bh].y;
			data[i + k*channels + 2] = b->upVectors[bh].z;
			data[i + k*channels + 3] = b->lengths[bh];
			k++;
		}
		// right0_x0 - right2_x2
		for (bh = 0;bh < DYN_TREE::MAX_HIERARCHY_DEPTH; bh++){
			data[i + k*channels + 0] = b->rightVectors[bh].x;
			data[i + k*channels + 1] = b->rightVectors[bh].y;
			data[i + k*channels + 2] = b->rightVectors[bh].z;
			data[i + k*channels + 3] = b->xvals[bh];
			k++;
		}
		// t0_x0 - t2_x2
		for (bh = 0;bh < DYN_TREE::MAX_HIERARCHY_DEPTH; bh++){
			data[i + k*channels + 0] = b->tVectors[bh].x;
			data[i + k*channels + 1] = b->tVectors[bh].y;
			data[i + k*channels + 2] = b->tVectors[bh].z;
			data[i + k*channels + 3] = 1.0f;
			k++;
		}
		// center1-center3
		for (bh = 0;bh < DYN_TREE::MAX_HIERARCHY_DEPTH; bh++){
			data[i + k*channels + 0] = b->origins[bh].x;
			data[i + k*channels + 1] = b->origins[bh].y;
			data[i + k*channels + 2] = b->origins[bh].z;
			data[i + k*channels + 3] = 1.0f;
			k++;
		}
	}
	dataTexture = new Texture(  
		GL_TEXTURE_2D,
		GL_RGBA32F,
		GL_RGBA,
		GL_FLOAT,
		data,
		texDimY, 
		texDimX,
		DYN_TREE::DATA_TEXTURE_NAME
		);
	delete [] data;
	data = NULL;
}

bool DTree::loadDataTexture(string filename)
{
	return false;
}

bool DTree::saveDataTexture(string filename)
{
	return false;
}

bool DTree::loadVBO(string filename)
{
	return false;
}

bool DTree::saveVBO(string filename)
{
	return false;
}


int DTree::linearizeHierarchy(){

	stack<DTreeBranch*> bStack;	
	bStack.push(trunk);
	DTreeBranch * branch, *actBranch;
	int cnt = 0, i;
	while( !bStack.empty() ){
		branch = bStack.top();
		branch->id = cnt;
		cnt++;
		bStack.pop();
		branches.push_back(branch);
		for (i=0; i<branch->children.size(); i++){
			if (! branch->children[i]->isLeaf()){
				actBranch = (DTreeBranch*)(branch->children[i]);
				bStack.push(actBranch);
			} else {
				//leaves.push_back((TreeLeaf*) branch->children[i] );
			}
		}		
	}
	return cnt;
};

void DTree::recalcCoordSystems()

{
	/*
	*	Express original coord system in terms of parent coord system...
	*/

	int i, size=branches.size();
	DTreeBranch * branch;
	for (i=0; i<size; i++){
		branch = branches[i];
		if (branch->parent!=NULL){
			branch->cs = branch->parent->originalCS.getSystemInThisSystem(branch->originalCS);
		}
	}

}

void DTree::fillParentDataForEachBranch()
{
	/*
	*	Propagate data from parents to all children, grandchildren, etc.
	*/
	// and copy from parent...
	DTreeBranch* actBranch, *branch;
	CoordSystem *cSys;
	int i, size=branches.size();
	for (i=0; i<size; i++){
		branch = branches[i];
		actBranch = branch;
		while (actBranch!=NULL){
			cSys = &(actBranch->cs);
			branch->xvals.data		[actBranch->level] = actBranch->x;
			branch->lengths.data	[actBranch->level] = actBranch->L;
			branch->motionVectors[actBranch->level]= actBranch->motionVector;
			if (actBranch->parent!=NULL){
				branch->origins		[actBranch->level] = cSys->origin;
				branch->upVectors	[actBranch->level] = cSys->r;
				branch->rightVectors[actBranch->level] = cSys->s;
				branch->tVectors    [actBranch->level] = cSys->t;
				actBranch = (DTreeBranch*)(actBranch->parent);

			} else {
				branch->upVectors	[actBranch->level] = cSys->r;
				branch->rightVectors[actBranch->level] = cSys->s;
				branch->tVectors    [actBranch->level] = cSys->t;
				actBranch = NULL;
			}
		}
	}
}


Vegetation* DTree::getCopy(){
	DTree * copy = new DTree(this);
	return copy;
}

void DTree::drawLOD0()
{
	g_tree_lod0_count++;
	glDisable(GL_CULL_FACE);
	glPushMatrix();
	glScalef( 10.f , -10.f, 10.f);
	
	// draw bbox
	//bbox->draw();


	// bind textures
	dataTexture->bind(GL_TEXTURE1);
	branchNoiseTexture->bind(GL_TEXTURE2);
	bColorTexture->bind(GL_TEXTURE4);

	// TODO: use positions

	// draw branches
	branchesEBO->draw(branchShader);

	bColorTexture->unbind();

	// bind textures
	leafNoiseTexture->bind(GL_TEXTURE3);
	frontDecalMap		->bind(GL_TEXTURE4);
	frontNormalMap		->bind(GL_TEXTURE5);
	frontTranslucencyMap->bind(GL_TEXTURE6);
	frontHalfLife2Map	->bind(GL_TEXTURE7);
	backDecalMap		->bind(GL_TEXTURE8);
	backNormalMap		->bind(GL_TEXTURE9);
	backTranslucencyMap	->bind(GL_TEXTURE10);
	backHalfLife2Map	->bind(GL_TEXTURE11);

	// TODO: use positions

	// draw leaves
	leavesVBO->draw(leafShader, GL_QUADS, 0);


	leafNoiseTexture->unbind();
	frontDecalMap		->unbind();
	frontNormalMap		->unbind();
	frontTranslucencyMap->unbind();
	frontHalfLife2Map	->unbind();
	backDecalMap		->unbind();
	backNormalMap		->unbind();
	backTranslucencyMap	->unbind();
	backHalfLife2Map	->unbind();
	branchNoiseTexture->unbind();
	dataTexture->unbind();
	glPopMatrix();
	glEnable(GL_CULL_FACE);
}

void DTree::drawLOD1()
{
	if (g_draw_dtree_lod){
	g_tree_lod1_count++;
	int i, j, sliceCount, setCount=sliceSets.size();
	Texture * colorTexture, * dataTexture, *displacementTexture, *displacement2Texture, *normalTexture;
		displacementTexture		= leafNoiseTexture;
		displacement2Texture	= branchNoiseTexture;
		DTreeSlice * slice;
	for (j = 0; j< setCount; j++){
		sliceCount = sliceSets[j]->size();
		
		for (i=0; i<sliceCount; i++){
			slice					= sliceSets[j]->getSlice(i);
			colorTexture			= slice->colormap;
			normalTexture			= slice->normalmap;
			dataTexture				= slice->datamap;

			glPushMatrix();	
		
				//l3dBillboardCheatCylindricalBegin();
			
				glRotatef(j*90, 0.0, 1.0, 0.0);
				glTranslatef((i-(float(sliceCount)/2.f) + 0.5), 0.0, 0.0);
				glScalef(10.0,10.0,10.0);
				//glRotatef(90, 0.0, 0.0, 1.0);
				glRotatef(90, 0.0, 1.0, 0.0);
				glDisable(GL_CULL_FACE);
				colorTexture		->bind(GL_TEXTURE0);
				displacementTexture	->bind(GL_TEXTURE1);
				displacement2Texture	->bind(GL_TEXTURE4);
				dataTexture			->bind(GL_TEXTURE2);
				normalTexture		->bind(GL_TEXTURE3);

				u_time_offset->data = &	g_tree_time_offset_1;
				// turn on shader
				lod1shader->use(true);
				lod1shader->setTexture(l_color	, colorTexture			->textureUnitNumber	);
				lod1shader->setTexture(l_displ	, displacementTexture	->textureUnitNumber	);
				lod1shader->setTexture(l_displ2	, displacement2Texture	->textureUnitNumber	);
				lod1shader->setTexture(l_data	, dataTexture			->textureUnitNumber	);
				lod1shader->setTexture(l_normal	, normalTexture			->textureUnitNumber	);
			
				// TODO: draw at all positions
				lod1vbo->draw(lod1shader, GL_QUADS, 0);

				colorTexture		->unbind();
				displacementTexture	->unbind();
				displacement2Texture	->unbind();
				dataTexture			->unbind();
				normalTexture		->unbind();
				// turn off shader
				lod1shader->use(false);
				glEnable(GL_CULL_FACE);

			glPopMatrix();
		} // for i - each slice
	} // for j - sliceSets
	}
}

void DTree::drawLOD2()
{
	g_tree_lod2_count++;

	if (g_draw_dtree_lod){
	g_tree_lod1_count++;
	int i, j, sliceCount, setCount=sliceSets.size();
	Texture * colorTexture, * dataTexture, *displacementTexture, *displacement2Texture, *normalTexture;
		displacementTexture		= leafNoiseTexture;
		displacement2Texture	= branchNoiseTexture;
		DTreeSlice * slice;
	for (j = 0; j< setCount; j++){
		sliceCount = sliceSets[j]->size();
		i = sliceCount/2;
		
			slice					= sliceSets[j]->getSlice(i);
			colorTexture			= slice->colormap;
			normalTexture			= slice->normalmap;
			dataTexture				= slice->datamap;

			glPushMatrix();	
		
				//l3dBillboardCheatCylindricalBegin();
			
				glRotatef(j*90, 0.0, 1.0, 0.0);
				glTranslatef((i-(float(sliceCount)/2.f) + 0.5), 0.0, 0.0);
				glScalef(10.0,10.0,10.0);
				//glRotatef(90, 0.0, 0.0, 1.0);
				glRotatef(90, 0.0, 1.0, 0.0);
				glDisable(GL_CULL_FACE);
				colorTexture		->bind(GL_TEXTURE0);
				displacementTexture	->bind(GL_TEXTURE1);
				displacement2Texture	->bind(GL_TEXTURE4);
				dataTexture			->bind(GL_TEXTURE2);
				normalTexture		->bind(GL_TEXTURE3);

				u_time_offset->data = &	g_tree_time_offset_1;
				// turn on shader
				lod1shader->use(true);
				lod1shader->setTexture(l_color	, colorTexture			->textureUnitNumber	);
				lod1shader->setTexture(l_displ	, displacementTexture	->textureUnitNumber	);
				lod1shader->setTexture(l_displ2	, displacement2Texture	->textureUnitNumber	);
				lod1shader->setTexture(l_data	, dataTexture			->textureUnitNumber	);
				lod1shader->setTexture(l_normal	, normalTexture			->textureUnitNumber	);
			
				// TODO: draw at all positions
				lod1vbo->draw(lod1shader, GL_QUADS, 0);

				colorTexture		->unbind();
				displacementTexture	->unbind();
				displacement2Texture	->unbind();
				dataTexture			->unbind();
				normalTexture		->unbind();
				// turn off shader
				lod1shader->use(false);
				glEnable(GL_CULL_FACE);

			glPopMatrix();
	} // for j - sliceSets
	}

}

void DTree::draw(){
	// get distance to viewer
	v3		toViewDir = *viewer_position - position;
	float distance = toViewDir.length();
	toViewDir.normalize();
	float discrepacy = toViewDir.dot(*viewer_direction);
	v4 lodTresholds = v4 (15.0, 20.0, 50.0, 55.0);
	if (discrepacy<0.0){ // if is infront of the camera
		float alpha=0.0;
		if (distance >= lodTresholds.w){
			glColor4f(0.0, 0.0, 0.0, 1.0);
			drawLOD2();
		} else if (distance >= lodTresholds.z){
			alpha = (distance - lodTresholds.z) / (lodTresholds.w - lodTresholds.z);
			/*
			// show LOD 1
				glColor4f(0.0, 0.0, 0.0, 1.0-alpha);
				drawLOD1();
				glDepthMask(GL_FALSE);				
				// show LOD 2 
				glColor4f(0.0, 0.0, 0.0, alpha);
				drawLOD2();
				glDepthMask(GL_TRUE);
			*/
			
			if (alpha<0.5){
				// show LOD 1
				glColor4f(0.0, 0.0, 0.0, 1.0);
				drawLOD1();
				glDepthMask(GL_FALSE);
				// show LOD 2 
				glColor4f(0.0, 0.0, 0.0, 2*alpha);
				drawLOD2();
				glDepthMask(GL_TRUE);
			} else {
				// show LOD 1 
				glDepthMask(GL_FALSE);
				glColor4f(0.0, 0.0, 0.0, 2*(1-alpha));
				drawLOD1();
				glDepthMask(GL_TRUE);
				// show LOD 2 
				glColor4f(0.0, 0.0, 0.0, 1.0);
				drawLOD2();
			}
			
		} else if (distance >=lodTresholds.y){
			glColor4f(0.0, 0.0, 0.0, 1.0);
			drawLOD1();
		} else if (distance >=lodTresholds.x){
			alpha = (distance - lodTresholds.x) / (lodTresholds.y - lodTresholds.x);
			/*
			glColor4f(0.0, 0.0, 0.0, 1.0-alpha);
				drawLOD0();
				glDepthMask(GL_FALSE);
				// show LOD 1 
				glColor4f(0.0, 0.0, 0.0, alpha);
				drawLOD1();
				glDepthMask(GL_TRUE);
			/*/
			if (alpha<0.5){
				// show LOD 0
				glColor4f(0.0, 0.0, 0.0, 1.0);
				drawLOD0();
				glDepthMask(GL_FALSE);
				// show LOD 1 
				glColor4f(0.0, 0.0, 0.0, 2*alpha);
				drawLOD1();
				glDepthMask(GL_TRUE);
			} else {
				// show LOD 0
				glDepthMask(GL_FALSE);
				glColor4f(0.0, 0.0, 0.0, 2*(1-alpha));
				drawLOD0();
				glDepthMask(GL_TRUE);
				// show LOD 1 
				glColor4f(0.0, 0.0, 0.0, 1.0);
				drawLOD1();
				
			}
			
		} else {
			glColor4f(0.0, 0.0, 0.0, 1.0);
			drawLOD0();
		}

	} // if in front of camera plane
}

void DTree::drawForLOD(){
	// TODO: optimize rendering for LOD slice generation...
	glDisable(GL_CULL_FACE);
	glPushMatrix();
	glScalef( 1.f , -1.f, 1.f);
	
	// draw bbox
	//bbox->draw();


	// bind textures
	dataTexture->bind(GL_TEXTURE1);
	branchNoiseTexture->bind(GL_TEXTURE2);
	bColorTexture->bind(GL_TEXTURE4);

	// draw branches
	branchesEBO->draw(bLODShader);

	bColorTexture->unbind();

	// bind textures
	leafNoiseTexture->bind(GL_TEXTURE3);
	frontDecalMap		->bind(GL_TEXTURE4);
	frontNormalMap		->bind(GL_TEXTURE5);
	frontTranslucencyMap->bind(GL_TEXTURE6);
	frontHalfLife2Map	->bind(GL_TEXTURE7);
	backDecalMap		->bind(GL_TEXTURE8);
	backNormalMap		->bind(GL_TEXTURE9);
	backTranslucencyMap	->bind(GL_TEXTURE10);
	backHalfLife2Map	->bind(GL_TEXTURE11);



	// draw leaves
	leavesVBO->draw(lLODShader, GL_QUADS, 0);


	leafNoiseTexture->unbind();
	frontDecalMap		->unbind();
	frontNormalMap		->unbind();
	frontTranslucencyMap->unbind();
	frontHalfLife2Map	->unbind();
	backDecalMap		->unbind();
	backNormalMap		->unbind();
	backTranslucencyMap	->unbind();
	backHalfLife2Map	->unbind();
	branchNoiseTexture->unbind();
	dataTexture->unbind();
	glPopMatrix();
	glEnable(GL_CULL_FACE);
}

void DTree::initLOD0()
{
	printf("DYN_TREE init\n");
	// load textures

	// NOISE
	branchNoiseTexture = new Texture("branch_noise_tex"); 
	branchNoiseTexture->load(DYN_TREE::BRANCH_NOISE_TEXTURE, false, true, GL_REPEAT, GL_LINEAR, GL_LINEAR);
	leafNoiseTexture = new Texture("leaf_noise_tex");
	leafNoiseTexture->load(DYN_TREE::LEAF_NOISE_TEXTURE, false, true, GL_REPEAT, GL_LINEAR, GL_LINEAR);

	//  COLOR
	bColorTexture = new Texture("color_texture");
	bColorTexture->load(DYN_TREE::TEX_WOOD1, true, false , GL_REPEAT, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
	//lColorTexture = new Texture("color_texture");
	//lColorTexture->load(DYN_TREE::TEX_LEAF1, true, false , GL_REPEAT, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);

	//frontDecalMap		=new Texture("color_texture");
	frontDecalMap		=new Texture("frontDecalMap");
	frontNormalMap		=new Texture("frontNormalMap");
	frontTranslucencyMap=new Texture("frontTranslucencyMap");
	frontHalfLife2Map	=new Texture("frontHalfLife2Map");
	backDecalMap		=new Texture("backDecalMap");
	backNormalMap		=new Texture("backNormalMap");
	backTranslucencyMap	=new Texture("backTranslucencyMap");
	backHalfLife2Map	=new Texture("backHalfLife2Map");
	frontDecalMap		->load(DYN_TREE::TEX_FDM, true, false, GL_CLAMP, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
	frontNormalMap		->load(DYN_TREE::TEX_FNM, true, false, GL_CLAMP, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
	frontTranslucencyMap->load(DYN_TREE::TEX_FTM, true, false, GL_CLAMP, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
	frontHalfLife2Map	->load(DYN_TREE::TEX_FHM, true, false, GL_CLAMP, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
	backDecalMap		->load(DYN_TREE::TEX_BDM, true, false, GL_CLAMP, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
	backNormalMap		->load(DYN_TREE::TEX_BNM, true, false, GL_CLAMP, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
	backTranslucencyMap	->load(DYN_TREE::TEX_BTM, true, false, GL_CLAMP, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);
	backHalfLife2Map	->load(DYN_TREE::TEX_BHM, true, false, GL_CLAMP, GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR);



	// init shaders
	branchShader = new Shader("branch");
	branchShader->loadShader(DYN_TREE::SHADER_BRANCH_V,DYN_TREE::SHADER_BRANCH_F); 
	leafShader = new Shader("leaf");
	leafShader->loadShader(DYN_TREE::SHADER_LEAF_V,DYN_TREE::SHADER_LEAF_F); 
	// LOD shaders
	bLODShader = new Shader("LODbranch");
	bLODShader->loadShader(DYN_TREE::SHADER_BRANCH_LOD_V,DYN_TREE::SHADER_BRANCH_LOD_F); 
	lLODShader = new Shader("LODleaf");
	lLODShader->loadShader(DYN_TREE::SHADER_LEAF_LOD_V,DYN_TREE::SHADER_LEAF_LOD_F); 
	 

	// connect textures with shader
	branchShader->linkTexture(branchNoiseTexture);
	branchShader->linkTexture(bColorTexture);
	bLODShader->linkTexture(branchNoiseTexture);
	bLODShader->linkTexture(bColorTexture);

	leafShader->linkTexture(branchNoiseTexture);
	leafShader->linkTexture(leafNoiseTexture);
	//leafShader->linkTexture(lColorTexture);
	leafShader->linkTexture(frontDecalMap		);
	leafShader->linkTexture(frontNormalMap		);
	leafShader->linkTexture(frontTranslucencyMap);
	leafShader->linkTexture(frontHalfLife2Map	);
	leafShader->linkTexture(backDecalMap		);
	leafShader->linkTexture(backNormalMap		);
	leafShader->linkTexture(backTranslucencyMap	);
	leafShader->linkTexture(backHalfLife2Map	);

	lLODShader->linkTexture(branchNoiseTexture	);
	lLODShader->linkTexture(leafNoiseTexture	);
	//lLODShader->linkTexture(lColorTexture);
	lLODShader->linkTexture(frontDecalMap		);
	lLODShader->linkTexture(frontNormalMap		);
	lLODShader->linkTexture(frontTranslucencyMap);
	lLODShader->linkTexture(frontHalfLife2Map	);
	lLODShader->linkTexture(backDecalMap		);
	lLODShader->linkTexture(backNormalMap		);
	lLODShader->linkTexture(backTranslucencyMap	);
	lLODShader->linkTexture(backHalfLife2Map	);



	// register uniforms
	this->branchCountF = 0;
	branchShader->registerUniform("branch_count",			UniformType::F1,	& this->branchCountF);
	branchShader->registerUniform("time",					UniformType::F1,	& g_float_time);
	branchShader->registerUniform("wind_direction",			UniformType::F3,	& g_tree_wind_direction.data);
	branchShader->registerUniform("wind_strength",			UniformType::F1,	& g_tree_wind_strength);
	branchShader->registerUniform("wood_amplitudes",		UniformType::F4,	& g_tree_wood_amplitudes.data);
	branchShader->registerUniform("wood_frequencies",		UniformType::F4,	& g_tree_wood_frequencies.data);
	branchShader->registerUniform("window_size",			UniformType::F2,	& g_window_sizes.data);		

	bLODShader->registerUniform("branch_count",				UniformType::F1,	& this->branchCountF);
	bLODShader->registerUniform("time",						UniformType::F1,	& g_float_time);
	bLODShader->registerUniform("wind_direction",			UniformType::F3,	& g_tree_wind_direction.data);
	bLODShader->registerUniform("wind_strength",			UniformType::F1,	& g_tree_wind_strength);
	bLODShader->registerUniform("wood_amplitudes",			UniformType::F4,	& g_tree_wood_amplitudes.data);
	bLODShader->registerUniform("wood_frequencies",			UniformType::F4,	& g_tree_wood_frequencies.data);
	bLODShader->registerUniform("window_size",				UniformType::F2,	& g_window_sizes.data);		

	//branchShader->registerUniform("leaf_amplitude", UniformType::F1, & g_tree_wood_amplitudes.data);
	//branchShader->registerUniform("leaf_frequency", UniformType::F1, & g_tree_wood_amplitudes.data);

	leafShader->registerUniform("branch_count",				UniformType::F1,	& this->branchCountF);
	leafShader->registerUniform("time",						UniformType::F1,	& g_float_time);
	leafShader->registerUniform("wind_direction",			UniformType::F3,	& g_tree_wind_direction.data);
	leafShader->registerUniform("wind_strength",			UniformType::F1,	& g_tree_wind_strength);
	leafShader->registerUniform("wood_amplitudes",			UniformType::F4,	& g_tree_wood_amplitudes.data);
	leafShader->registerUniform("wood_frequencies",			UniformType::F4,	& g_tree_wood_frequencies.data);
	leafShader->registerUniform("leaf_amplitude",			UniformType::F1,	& g_tree_leaf_amplitude);
	leafShader->registerUniform("leaf_frequency",			UniformType::F1,	& g_tree_leaf_frequency);
	leafShader->registerUniform("MultiplyAmbient",			UniformType::F1,	& g_leaves_MultiplyAmbient);
	leafShader->registerUniform("MultiplyDiffuse",			UniformType::F1,	& g_leaves_MultiplyDiffuse);
	leafShader->registerUniform("MultiplySpecular",			UniformType::F1,	& g_leaves_MultiplySpecular);
	leafShader->registerUniform("MultiplyTranslucency",		UniformType::F1,	& g_leaves_MultiplyTranslucency);
	leafShader->registerUniform("ReduceTranslucencyInShadow", UniformType::F1,	& g_leaves_ReduceTranslucencyInShadow);
	leafShader->registerUniform("shadow_intensity",			UniformType::F1,	& g_leaves_shadow_intensity);
	leafShader->registerUniform("LightDiffuseColor",		UniformType::F3,	& g_leaves_LightDiffuseColor.data);
	leafShader->registerUniform("window_size",				UniformType::F2,	& g_window_sizes.data);		

	lLODShader->registerUniform("branch_count",				UniformType::F1,	& this->branchCountF);
	lLODShader->registerUniform("time",						UniformType::F1,	& g_float_time);
	lLODShader->registerUniform("wind_direction",			UniformType::F3,	& g_tree_wind_direction.data);
	lLODShader->registerUniform("wind_strength",			UniformType::F1,	& g_tree_wind_strength);
	lLODShader->registerUniform("wood_amplitudes",			UniformType::F4,	& g_tree_wood_amplitudes.data);
	lLODShader->registerUniform("wood_frequencies",			UniformType::F4,	& g_tree_wood_frequencies.data);
	lLODShader->registerUniform("leaf_amplitude",			UniformType::F1,	& g_tree_leaf_amplitude);
	lLODShader->registerUniform("leaf_frequency",			UniformType::F1,	& g_tree_leaf_frequency);
	lLODShader->registerUniform("MultiplyAmbient",			UniformType::F1,	& g_leaves_MultiplyAmbient);
	lLODShader->registerUniform("MultiplyDiffuse",			UniformType::F1,	& g_leaves_MultiplyDiffuse);
	lLODShader->registerUniform("MultiplySpecular",			UniformType::F1,	& g_leaves_MultiplySpecular);
	lLODShader->registerUniform("MultiplyTranslucency",		UniformType::F1,	& g_leaves_MultiplyTranslucency);
	lLODShader->registerUniform("ReduceTranslucencyInShadow", UniformType::F1,	& g_leaves_ReduceTranslucencyInShadow);
	lLODShader->registerUniform("shadow_intensity",			UniformType::F1,	& g_leaves_shadow_intensity);
	lLODShader->registerUniform("LightDiffuseColor",		UniformType::F3,	& g_leaves_LightDiffuseColor.data);
	lLODShader->registerUniform("window_size",				UniformType::F2,	& g_window_sizes.data);		


	// create branch data texture
	createDataTexture();
	// link data texture
	//dataTexture->save("dataTexture.png");
	branchShader->linkTexture(dataTexture);
	leafShader	->linkTexture(dataTexture);
	bLODShader	->linkTexture(dataTexture);
	lLODShader	->linkTexture(dataTexture);


	// create VBO & EBO with geometry...
	createVBOs();

	// link vbos & shaders
	branchesVBO	->compileWithShader(branchShader);
	leavesVBO	->compileWithShader(leafShader);

	branchesVBO	->compileWithShader(bLODShader);
	leavesVBO	->compileWithShader(lLODShader);

	//printf("branch count = %i\n", branchCount);
	//printf("DYN_TREE done (branch VBOid:%i, branchEBOid:%i, leafVBOid:%i)\n", branchesVBO->getID(), branchesEBO->getID(), leavesVBO->getID());


}

void DTree::initLOD1()
{
	
	// create slices

	// create 2 sliceSets (cross, double sided)
	v3 dir = v3(-1.0, 0.0, 0.0);
	DTreeSliceSet * set = new DTreeSliceSet();
	float res = 256;
	win_resolution = v2 (res, res);
	this->createSlices(dir, 3, win_resolution.x, win_resolution.y, false);
	set->setSlices(this->slices);
	//set->createFromDir(this, dir);
	sliceSets.push_back(set);

	
	dir = v3 (0.0, 0.0, -1.0);
	set = new DTreeSliceSet();
	this->createSlices(dir, 3, win_resolution.x, win_resolution.y, false);
	set->setSlices(this->slices);
	//set->createFromDir(this, dir);
	sliceSets.push_back(set);

	// init shaders

	lod1shader = new Shader("test");
	lod1shader->loadShader("shaders/test2_vs.glsl", "shaders/test2_fs.glsl");
	// link textures to shader
	//shader->linkTexture(colorMap			);
	//shader->linkTexture(displacementMap		);
	//shader->linkTexture(dataMap				);

	l_color		 = lod1shader->getGLLocation("colorMap"			);
	l_displ		 = lod1shader->getGLLocation("leaf_noise_tex"	);
	l_displ2	 = lod1shader->getGLLocation("branch_noise_tex"	);
	l_data		 = lod1shader->getGLLocation("dataMap"			);
	l_normal	 = lod1shader->getGLLocation("normalMap"		);

	lod1shader->registerUniform("time", UniformType::F1, & g_float_time);

	lod1shader->registerUniform("wave_amplitude"		, UniformType::F1, & g_tree_wave_amplitude		);
	lod1shader->registerUniform("wave_frequency"		, UniformType::F1, & g_tree_wave_frequency		);
	lod1shader->registerUniform("movementVectorA"		, UniformType::F2, & g_tree_movementVectorA		);
	lod1shader->registerUniform("movementVectorB"		, UniformType::F2, & g_tree_movementVectorB		);
	lod1shader->registerUniform("wave_y_offset"			, UniformType::F1, & g_tree_wave_y_offset			);
	lod1shader->registerUniform("wave_increase_factor"	, UniformType::F1, & g_tree_wave_increase_factor	);
	lod1shader->registerUniform("window_size"			, UniformType::F2, & win_resolution				);


	lod1shader->registerUniform("wood_amplitudes"		, UniformType::F4, & g_tree_wood_amplitudes.data	);
	lod1shader->registerUniform("wood_frequencies"		, UniformType::F4, & g_tree_wood_frequencies.data	);
	lod1shader->registerUniform("leaf_amplitude"		, UniformType::F1, & g_tree_leaf_amplitude	);
	lod1shader->registerUniform("leaf_frequency"		, UniformType::F1, & g_tree_leaf_frequency	);

	int i = lod1shader->registerUniform("time_offset"	, UniformType::F1, & tree_time_offset);
	u_time_offset = lod1shader->getUniform(i);
	
	/*
	shader = new Shader("test");
	shader->loadShader("shaders/test_vs.glsl", "shaders/test_fs.glsl");
	// link textures to shader
	shader->linkTexture(frontDecalMap		);
	shader->linkTexture(frontNormalMap		);
	shader->linkTexture(frontTranslucencyMap);
	shader->linkTexture(frontHalfLife2Map	);
	shader->linkTexture(backDecalMap		);
	shader->linkTexture(backNormalMap		);
	shader->linkTexture(backTranslucencyMap	);
	shader->linkTexture(backHalfLife2Map	);
	*/
	
	// init VBO
	int count = 4;
	lod1vbo = new VBO();
	lod1vbo->setVertexCount(count);
	// position
	VBODataSet * dataSet = new VBODataSet(
		PLANE_VERTEX_ARRAY2,
		3*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::POSITION,
		true
	);
	lod1vbo->addVertexAttribute( dataSet );
	// normal
	dataSet = new VBODataSet(
		PLANE_NORMAL_ARRAY2,
		3*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::NORMAL,
		false
	);
	lod1vbo->addVertexAttribute( dataSet );
	// tangent
	dataSet = new VBODataSet(
		PLANE_TANGENT_ARRAY,
		3*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::TANGENT,
		false
	);
	lod1vbo->addVertexAttribute( dataSet );
	// texture coordinates
	dataSet = new VBODataSet(
		PLANE_TEX_COORD_ARRAY2,
		2*sizeof(GLfloat),
		GL_FLOAT, 
		ATTRIB_NAMES::TEXCOORD0,
		false
	);
	lod1vbo->addVertexAttribute( dataSet );

	// link vbo and shaders
	lod1vbo->compileData(GL_STATIC_DRAW);
	lod1vbo->compileWithShader(lod1shader);
	
}

void DTree::initLOD2()
{


}

void DTree::init(){
	initLOD0();
	initLOD1();
	//initLOD2();
}

void DTree::update(double time){

}

void DTree::bakeToVBO(){

}

void DTree::fixTexType(){

}

v3	 DTree::transformTexCoords(v3 &origTexCoords){
	return origTexCoords;
}

BBox * DTree::getBBox()
{
	if (bbox==NULL){
		bbox = new BBox(v3(-0.5, -1.0, -0.5),v3(0.5, 0.0, 0.5), v3(0.0, 1.0, 0.0));
	}
	return bbox;
}

void DTree::createSlices(v3 & direction, int num, int resolution_x, int resolution_y, bool half){
	// init data pre-processing shader
	Shader * dataProcessShader = new Shader("data_pre-processor");
	dataProcessShader->loadShader(DYN_TREE::SHADER_PREPROCESS_V, DYN_TREE::SHADER_PREPROCESS_F);
	GLint	gl_location = dataProcessShader->getGLLocation("branchMap");
	int		loc_win_size = dataProcessShader->getLocation("window_size");
	// dummy depth map
	Texture * depthmap = new Texture(GL_TEXTURE_2D, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, NULL, resolution_x, resolution_y, "dummy_depthMap");
	depthmap->textureUnit = GL_TEXTURE7;
	depthmap->textureUnitNumber = 7;
	// dummy color map

	//Texture * colormap = new Texture(GL_TEXTURE_2D, GL_RGB, GL_RGB, GL_UNSIGNED_BYTE, NULL, resolution_x, resolution_y, "dummy_colorMap");
	//colormap->textureUnit = GL_TEXTURE8;
	//colormap->textureUnitNumber = 8;

// get "slice thickness"
	BBox * box = getBBox();
	float distance = -10.f;
	v3 position = direction * distance;
	float diameter = 0.7*box->getMinSize();
	float radius = diameter*0.5f;
	float thickness = diameter/(float(num));
	if (half) thickness*=0.5;
	float left = -0.5, right= 0.5, bottom= 0.0, top= 1.0, near, far;
	float positionDist = position.length();
	DTreeSlice * slice;
	int i;

	// clear previous slices
	/*
	for ( i = 0; i < slices.size(); i++){
			delete slices[i];
		}
	*/
		slices.clear();
		GLuint fbo = 0;
		

	for ( i = 0; i< num; i++){
		// create FBO
			glGenFramebuffersEXT(1, &fbo);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
		// each slice
			slice = new DTreeSlice();
		// add slice
			slices.push_back(slice);
		// create & setup textures
			slice->colormap = new Texture(GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, NULL, resolution_x, resolution_y, "colorMap");
			slice->colormap->textureUnit = GL_TEXTURE0;
			slice->colormap->textureUnitNumber = 0;
			slice->colormap->setParameterI(GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			slice->colormap->setParameterI(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			

			slice->depthmap = new Texture(GL_TEXTURE_2D, GL_DEPTH_COMPONENT32F, GL_DEPTH_COMPONENT, GL_FLOAT, NULL, resolution_x, resolution_y, "depthMap");
			slice->depthmap->textureUnit = GL_TEXTURE2;
			slice->depthmap->textureUnitNumber = 2;


			slice->normalmap = new Texture(GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, NULL, resolution_x, resolution_y, "normalMap");
			slice->normalmap->textureUnit = GL_TEXTURE3;
			slice->normalmap->textureUnitNumber = 3;
			
			slice->branchmap = new Texture(GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, NULL, resolution_x, resolution_y, "branchMap");
			slice->branchmap->textureUnit = GL_TEXTURE4;
			slice->branchmap->textureUnitNumber = 4;

		// attach textures to FBO attachments

			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, slice->colormap->id  , 0);
			//glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_TEXTURE_2D, colormap->id , 0);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT2_EXT, GL_TEXTURE_2D, slice->normalmap->id , 0);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT3_EXT, GL_TEXTURE_2D, slice->branchmap->id , 0);
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT , GL_TEXTURE_2D, slice->depthmap->id  , 0);
			//glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			

			glClearColor(0.f, 0.f, 0.f, 0.f);
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			//GLenum buffers[4] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT1_EXT, GL_COLOR_ATTACHMENT2_EXT, GL_COLOR_ATTACHMENT3_EXT};
			//glDrawBuffersARB(4, buffers);
			GLenum buffers[3] = {GL_COLOR_ATTACHMENT0_EXT, GL_COLOR_ATTACHMENT2_EXT, GL_COLOR_ATTACHMENT3_EXT};
			glDrawBuffersARB(3, buffers);
			assert(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT);
			assert( glGetError() == GL_NO_ERROR );
			printf("TREE_SLICE %i framebuffer initialized successfully\n", i);
			/*
			GLenum buffers[3] = {GL_COLOR_ATTACHMENT0_EXT,GL_COLOR_ATTACHMENT1_EXT, GL_COLOR_ATTACHMENT2_EXT};
			glDrawBuffersARB(3, buffers);
			*/

		// setup near & far plane
			near = positionDist-radius + i * thickness;
			far = near + thickness;
			// last interval must be to infinity if half slicing
			if (half && i==num-1)
			{
				far = positionDist-radius + 2*(i+1) * thickness;
			}
		// setup camera to orthoprojection with respect to slice interval
			glMatrixMode(GL_PROJECTION);
			glPushMatrix();
				glLoadIdentity();
				g_window_sizes.x = resolution_x;
				g_window_sizes.y = resolution_y;
				glViewport(0, 0, g_window_sizes.x, g_window_sizes.y);
				//system("PAUSE");
				glOrtho(left, right, bottom, top, near, far);
				
			glMatrixMode(GL_MODELVIEW);
				glPushMatrix();
				glLoadIdentity();
				gluLookAt( position.x, position.y, position.z, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0);
		// render offscreen
		// draw tree now...
			drawForLOD();
			glFinish();
		glMatrixMode(GL_PROJECTION);
			glPopMatrix();
		glMatrixMode(GL_MODELVIEW);
			glPopMatrix();
		//
	

		// generate mipmaps where needed
			//slice->colormap->generateMipmaps();
			//slice->colormap->setParameterI(GL_TEXTURE_MIN_LOD, 0);
			//slice->colormap->setParameterI(GL_TEXTURE_BASE_LEVEL, 0);
			
			//slice->normalmap->generateMipmaps();
			//slice->depthmap->generateMipmaps();


		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

		// prepare data texture from branch texture
		glDeleteFramebuffersEXT(1, &fbo);
		
		glGenFramebuffersEXT(1, &fbo);
		glDisable(GL_DEPTH_TEST);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
			slice->datamap = new Texture(GL_TEXTURE_2D, GL_RGBA, GL_RGBA, GL_UNSIGNED_BYTE, NULL, resolution_x, resolution_y, "dataMap");
			slice->datamap->textureUnit = GL_TEXTURE5;
			slice->datamap->textureUnitNumber = 5;
			glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, slice->datamap->id , 0);
			//glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT , GL_TEXTURE_2D, depthmap->id , 0);
			assert(glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT)==GL_FRAMEBUFFER_COMPLETE_EXT);
			assert( glGetError() == GL_NO_ERROR );
			printf("TREE_SLICE %i data framebuffer initialized successfully\n", i);

			glClearColor(1.f, 0.f, 0.f, 1.f);
			glClear(GL_COLOR_BUFFER_BIT);
			//glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);
			GLenum buffers2[1] = {GL_COLOR_ATTACHMENT0_EXT};
			glDrawBuffersARB(1, buffers2);
			
			// activate shader
			//slice->branchmap->bind(GL_TEXTURE0);
			dataProcessShader->use(true);
			slice->branchmap->bind(slice->branchmap->textureUnit);
			dataProcessShader->setTexture(gl_location, slice->branchmap->textureUnitNumber);			
			dataProcessShader->setUniform2f(loc_win_size, g_window_sizes.x, g_window_sizes.y);
			slice->branchmap->show(0,0, g_window_sizes.x, g_window_sizes.y);
			dataProcessShader->use(false);
			//slice->branchmap->unbind();
			glFinish();
			glEnable(GL_DEPTH_TEST);
		// return to normal screen rendering	
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		// branchmap not useful anymore, delete it
		SAFE_DELETE_PTR(slice->branchmap);
		
		glDeleteFramebuffersEXT(1, &fbo);
		//*/
		glDrawBuffer(GL_BACK);

		g_window_sizes.x = g_WinWidth;
		g_window_sizes.y = g_WinHeight;
		glViewport(0, 0, g_window_sizes.x, g_window_sizes.y);
		
	} // for each slice
	
	SAFE_DELETE_PTR ( depthmap );
	SAFE_DELETE_PTR ( dataProcessShader );
}
