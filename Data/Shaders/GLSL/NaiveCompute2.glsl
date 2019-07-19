#version 440

struct Transform {
	float raX, raY, raZ; 
	float pX, pY, pZ;
	float sX, sY, sZ;
	float rotationAngle;
};

struct ElementData {
	mat4 model;
	mat4 mvp;
};

layout(std430, binding = 0) writeonly buffer OUT0
{
    ElementData elementData;
};

layout(std430, binding = 1) buffer INOUT0
{
    Transform transform;
};

uniform float uRotationAmount = 0.1;
uniform mat4 uViewMatrix;
uniform mat4 uProjMatrix;

layout(local_size_x=1) in;

// Reference: http://www.euclideanspace.com/maths/geometry/rotations/conversions/angleToMatrix/
mat4 makeModelMatrix(Transform transform)
{
	float c = cos(transform.rotationAngle);
	float s = sin(transform.rotationAngle);
	float t = 1.0 - c;
	vec3 axis = normalize(vec3(transform.raX, transform.raY, transform.raZ));
	
	float txx = t*axis.x*axis.x;
	float txy = t*axis.x*axis.y;
	float txz = t*axis.x*axis.z;
	float tyy = t*axis.y*axis.y;
	float tyz = t*axis.y*axis.z;
	float tzz = t*axis.z*axis.z;
	
	float sx = s*axis.x;
	float sy = s*axis.y;
	float sz = s*axis.z;
	
	mat4 scale = mat4(
					vec4(transform.sX, 0.0, 0.0, 0.0),
					vec4(0.0, transform.sY, 0.0, 0.0),
					vec4(0.0, 0.0, transform.sZ, 0.0),
					vec4(0.0, 0.0, 0.0,          1.0));

	mat4 rotation = mat4(
						vec4(txx + c,  txy + sz, txz - sy, 0.0),
						vec4(txy - sz, tyy + c,  tyz + sz, 0.0),
						vec4(txz + sy, tyz - sx, tzz + c,  0.0),
						vec4(0.0,      0.0,      0.0,      1.0));
						
	mat4 modelMat = rotation * scale;

	// Translation directly
	modelMat[3][0] = transform.pX;
	modelMat[3][1] = transform.pY;
	modelMat[3][2] = transform.pZ;
	
	return modelMat;
}

void main(void)
{
	transform.rotationAngle += uRotationAmount;
	mat4 model = makeModelMatrix(transform);
	elementData.model = model;
	elementData.mvp = uProjMatrix * uViewMatrix * model;
}
