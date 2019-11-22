/*
	Implemented using http://blog.simonrodriguez.fr/articles/30-07-2016_implementing_fxaa.html#ref4 as reference
	for FXAA with a medium quality, with use of https://developer.download.nvidia.com/assets/gamedev/files/sdk/11/FXAA_WhitePaper.pdf for
	example value tuning.
*/
#version 450
#extension GL_EXT_nonuniform_qualifier : require
#extension GL_GOOGLE_include_directive : enable
#include "../common.glsl"

layout (constant_id = 0) const float SC_INV_SCREEN_X = 0.0;
layout (constant_id = 1) const float SC_INV_SCREEN_Y = 0.0;
layout (constant_id = 2) const uint SC_GEOMETRY_COLOUR_IDX = 0;
layout (constant_id = 3) const uint SC_GEOMETRY_DEPTH_IDX = 0;

const float EDGE_THRESHOLD_MAX = 0.0625;
const float EDGE_THRESHOLD_MIN = 0.03125;
const float SUBPIX_QUALITY = 0.875;
const float INV_TWELVE = 1.0 / 12.0;
const int ITERATIONS = 12;
const float QUALITY[12] = float[](1.0, 1.0, 1.0, 1.0, 1.0, 1.5, 2.0, 2.0, 2.0, 2.0, 4.0, 8.0);

layout(location = 0) in vec2 uv;
layout(location = 0) out vec4 colour;

float rgbToLuminance(vec3 rgb){
    return dot(rgb, vec3(0.299, 0.587, 0.114));
}

void main()
{
	vec3 col = texture(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv).rgb;
	
	float lum = rgbToLuminance(col);
	float lumD = rgbToLuminance(textureOffset(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv, ivec2( 0, -1)).rgb);
	float lumU = rgbToLuminance(textureOffset(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv, ivec2( 0,  1)).rgb);
	float lumL = rgbToLuminance(textureOffset(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv, ivec2(-1,  0)).rgb);
	float lumR = rgbToLuminance(textureOffset(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv, ivec2( 1,  0)).rgb);
	
	float lumMin = min(lum, min(min(lumD, lumU), min(lumL, lumR)));
	float lumMax = max(lum, max(max(lumD, lumU), max(lumL, lumR)));
	
	float range = lumMax - lumMin;
	
	if (range < max(EDGE_THRESHOLD_MIN, lumMax * EDGE_THRESHOLD_MAX)) {
		colour = vec4(col, 1.0);
	}
	else {
		float lumDL = rgbToLuminance(textureOffset(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv, ivec2(-1,-1)).rgb);
		float lumUR = rgbToLuminance(textureOffset(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv, ivec2( 1, 1)).rgb);
		float lumUL = rgbToLuminance(textureOffset(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv, ivec2(-1, 1)).rgb);
		float lumDR = rgbToLuminance(textureOffset(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv, ivec2( 1,-1)).rgb);
		
		float lumDU = lumD + lumU;
		float lumLR = lumL + lumR;
		
		float lumLC = lumDL + lumUL;
		float lumDC = lumDL + lumDR;
		float lumRC = lumDR + lumUR;
		float lumUC = lumUR + lumUL;
		
		float edgeH = abs(-2.0 * lumL + lumLC) + abs(-2.0 * lum + lumDU) * 2.0 + abs(-2.0 * lumR + lumRC);
		float edgeV = abs(-2.0 * lumU + lumUC) + abs(-2.0 * lum + lumLR) * 2.0 + abs(-2.0 * lumD + lumDC);
		
		bool horizontal = edgeH >= edgeV;
		
		float lum1 = horizontal ? lumD : lumL;
		float lum2 = horizontal ? lumU : lumR;
		
		float grad1 = lum1 - lum;
		float grad2 = lum2 - lum;
		
		float grad = 0.25 * max(abs(grad1), abs(grad2));
		float stepLength = horizontal ? SC_INV_SCREEN_Y : SC_INV_SCREEN_X;
		stepLength *= abs(grad1) >= abs(grad2) ? -1.0 : 1.0;

		float lumAvg = abs(grad1) >= abs(grad2) ? 0.5 * (lum1 + lum) : 0.5 * (lum2 + lum);

		vec2 uvCurrent = horizontal ? vec2(uv.x, uv.y + stepLength * 0.5) : vec2(uv.x + stepLength * 0.5, uv.y);

		vec2 uvOffset = horizontal ? vec2(SC_INV_SCREEN_X, 0.0) : vec2(0.0, SC_INV_SCREEN_Y);
		vec2 uv1 = uvCurrent - uvOffset;
		vec2 uv2 = uvCurrent + uvOffset;
		
		float lumEnd1 = rgbToLuminance(texture(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv1).rgb) - lumAvg;
		float lumEnd2 = rgbToLuminance(texture(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv2).rgb) - lumAvg;
		
		bool reached1 = abs(lumEnd1) >= grad;
		bool reached2 = abs(lumEnd2) >= grad;
		bool reachedBoth = reached1 && reached2;
		
		if (!reached1) {
			uv1 -= uvOffset;
		}
		if(!reached2){
			uv2 += uvOffset;
		}
		
		if (!reachedBoth) {
			for (int i = 2; i < ITERATIONS; ++i) {
				if (!reached1) {
					lumEnd1 = rgbToLuminance(texture(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv1).rgb) - lumAvg;
				}
				if (!reached2) {
					lumEnd2 = rgbToLuminance(texture(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uv2).rgb) - lumAvg;
				}
				reached1 = abs(lumEnd1) >= grad;
				reached2 = abs(lumEnd2) >= grad;
				reachedBoth = reached1 && reached2;
				
				if (reachedBoth) {
					break;
				}
				
				if (!reached1) {
					uv1 -= uvOffset * QUALITY[i];
				}
				if (!reached2) {
					uv2 += uvOffset * QUALITY[i];
				}
			}
		}
		
		float dist1 = horizontal ? (uv.x - uv1.x) : (uv.y - uv1.y);
		float dist2 = horizontal ? (uv2.x - uv.x) : (uv2.y - uv.y);
		
		float dist = min(dist1, dist2);
		float thickness = dist1 + dist2;
		
		float pixelOffset = -dist/thickness + 0.5;
		
		float finalOffset = (((dist1 < dist2) ? lumEnd1 : lumEnd2) < 0.0) != lum < lumAvg ? pixelOffset : 0.0;
		
		lumAvg = INV_TWELVE * (2.0 * (lumDU + lumLR) + lumLC + lumRC);
		float subpixOffset1 = clamp(abs(lumAvg - lum) / range, 0.0, 1.0);
		float subpixOffset2 = (-2.0 * subpixOffset1 + 3.0) * subpixOffset1 * subpixOffset1;
		
		float subpixOffset = subpixOffset2 * subpixOffset2 * SUBPIX_QUALITY;
		finalOffset = max(finalOffset, subpixOffset);
		vec2 uvFinal = horizontal ? vec2(uv.x, uv.y + finalOffset * stepLength) : vec2(uv.x + finalOffset * stepLength, uv.y);
		colour = vec4(texture(texSamplers[nonuniformEXT(SC_GEOMETRY_COLOUR_IDX)], uvFinal).rgb, 1.0);
	}
}
