#version 450
#extension GL_ARB_separate_shader_objects : enable
#extension GL_GOOGLE_include_directive : enable
#include "./definitions.glsl"
#include "./functions.glsl"

#define CUSTOM_BINDING_0 0 //CUSTOM_SET_1
#define CUSTOM_BINDING_1 1 //CUSTOM_SET_1
#define CUSTOM_BINDING_2 2 //CUSTOM_SET_1
#define CUSTOM_BINDING_3 3 //CUSTOM_SET_1
#define CUSTOM_BINDING_4 2 //CUSTOM_SET_0
#define CUSTOM_SET_0 0
#define CUSTOM_SET_1 2

#include "./runtime_functions.glsl"

layout(location = 0) out vec4 color;
layout(location = 0) in vec3 view_ray;
layout(location = 1) in vec2 uvCoords;

layout(set = 1, binding = 0) uniform LightingData
{
	vec4 cameraPosition;
	vec4 ambientColour;
	vec4 lightPositions[1];
};

float GetSunVisibility(vec3 point, vec3 sun_direction) {
  vec3 p = point - kSphereCenter;
  float p_dot_v = dot(p, sun_direction);
  float p_dot_p = dot(p, p);
  float ray_sphere_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
  float distance_to_intersection = -p_dot_v - sqrt(
      kSphereRadius * kSphereRadius - ray_sphere_center_squared_distance);
  if (distance_to_intersection > 0.0) {
    // Compute the distance between the view ray and the sphere, and the
    // corresponding (tangent of the) subtended angle. Finally, use this to
    // compute an approximate sun visibility.
    float ray_sphere_distance =
        kSphereRadius - sqrt(ray_sphere_center_squared_distance);
    float ray_sphere_angular_distance = -ray_sphere_distance / p_dot_v;
    return smoothstep(1.0, 0.0, ray_sphere_angular_distance / Params.sun_size.x);
  }
  return 1.0;
}

float GetSkyVisibility(vec3 point) {
  vec3 p = point - kSphereCenter;
  float p_dot_p = dot(p, p);
  return
      1.0 + p.z / sqrt(p_dot_p) * kSphereRadius * kSphereRadius / p_dot_p;
}

void GetSphereShadowInOut(vec3 view_direction, vec3 sun_direction,
    out float d_in, out float d_out) {
  vec3 pos = cameraPosition.xyz - kSphereCenter;
  float pos_dot_sun = dot(pos, sun_direction);
  float view_dot_sun = dot(view_direction, sun_direction);
  float k = Params.sun_size.x;
  float l = 1.0 + k * k;
  float a = 1.0 - l * view_dot_sun * view_dot_sun;
  float b = dot(pos, view_direction) - l * pos_dot_sun * view_dot_sun -
      k * kSphereRadius * view_dot_sun;
  float c = dot(pos, pos) - l * pos_dot_sun * pos_dot_sun -
      2.0 * k * kSphereRadius * pos_dot_sun - kSphereRadius * kSphereRadius;
  float discriminant = b * b - a * c;
  if (discriminant > 0.0) {
    d_in = max(0.0, (-b - sqrt(discriminant)) / a);
    d_out = (-b + sqrt(discriminant)) / a;
    // The values of d for which delta is equal to 0 and kSphereRadius / k.
    float d_base = -pos_dot_sun / view_dot_sun;
    float d_apex = -(pos_dot_sun + kSphereRadius / k) / view_dot_sun;
    if (view_dot_sun > 0.0) {
      d_in = max(d_in, d_apex);
      d_out = a > 0.0 ? min(d_out, d_base) : d_base;
    } else {
      d_in = a > 0.0 ? max(d_in, d_base) : d_base;
      d_out = min(d_out, d_apex);
    }
  } else {
    d_in = 0.0;
    d_out = 0.0;
  }
}

void main() 
{  
  // Normalized view direction vector.
 /* vec3 view_direction = normalize(view_ray);
  
  // Tangent of the angle subtended by this fragment.
  float fragment_angular_size =
      length(dFdx(view_ray) + dFdy(view_ray)) / length(view_ray);

  float shadow_in;
  float shadow_out;
  GetSphereShadowInOut(view_direction, sun_direction, shadow_in, shadow_out);

  // Hack to fade out light shafts when the Sun is very close to the horizon.
  float lightshaft_fadein_hack = smoothstep(
      0.02, 0.04, dot(normalize(cameraPosition.xyz - earth_center), sun_direction));
	  
  // Compute the distance between the view ray line and the sphere center,
  // and the distance between the camera and the intersection of the view
  // ray with the sphere (or NaN if there is no intersection).
  vec3 p = cameraPosition.xyz - kSphereCenter;
  float p_dot_v = dot(p, view_direction);
  float p_dot_p = dot(p, p);
  float ray_sphere_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
  float distance_to_intersection = -p_dot_v - sqrt(
      kSphereRadius * kSphereRadius - ray_sphere_center_squared_distance);

  // Compute the radiance reflected by the sphere, if the ray intersects it.
  float sphere_alpha = 0.0;
  vec3 sphere_radiance = vec3(0.0);
  if (distance_to_intersection > 0.0) {
	// Compute the distance between the view ray and the sphere, and the
	// corresponding (tangent of the) subtended angle. Finally, use this to
	// compute the approximate analytic antialiasing factor sphere_alpha.
	float ray_sphere_distance =
		kSphereRadius - sqrt(ray_sphere_center_squared_distance);
	float ray_sphere_angular_distance = -ray_sphere_distance / p_dot_v;
	sphere_alpha =
		min(ray_sphere_angular_distance / fragment_angular_size, 1.0);
		
	vec3 point = cameraPosition.xyz + view_direction * distance_to_intersection;
	vec3 normal = normalize(point - kSphereCenter);

	// Compute the radiance reflected by the sphere.
	vec3 sky_irradiance;
	vec3 sun_irradiance = GetSunAndSkyIrradiance(
		point - earth_center, normal, sun_direction, sky_irradiance);
	sphere_radiance =
		kSphereAlbedo * (1.0 / PI) * (sun_irradiance + sky_irradiance);
		
	float shadow_length =
		max(0.0, min(shadow_out, distance_to_intersection) - shadow_in) *
		lightshaft_fadein_hack;
	vec3 transmittance;
	vec3 in_scatter = GetSkyRadianceToPoint(cameraPosition.xyz - earth_center,
		point - earth_center, shadow_length, sun_direction, transmittance);
	sphere_radiance = sphere_radiance * transmittance + in_scatter;
  }
  // Compute the distance between the view ray line and the Earth center,
  // and the distance between the camera and the intersection of the view
  // ray with the ground (or NaN if there is no intersection).
  p = cameraPosition.xyz - earth_center;
  p_dot_v = dot(p, view_direction);
  p_dot_p = dot(p, p);
  float ray_earth_center_squared_distance = p_dot_p - p_dot_v * p_dot_v;
  distance_to_intersection = -p_dot_v - sqrt(
      earth_center.z * earth_center.z - ray_earth_center_squared_distance);

  // Compute the radiance reflected by the ground, if the ray intersects it.
  float ground_alpha = 0.0;
  vec3 ground_radiance = vec3(0.0);
  if (distance_to_intersection > 0.0) {
    vec3 point = cameraPosition.xyz + view_direction * distance_to_intersection;
    vec3 normal = normalize(point - earth_center);

    // Compute the radiance reflected by the ground.
    vec3 sky_irradiance;
    vec3 sun_irradiance = GetSunAndSkyIrradiance(
        point - earth_center, normal, sun_direction, sky_irradiance);
    ground_radiance = kGroundAlbedo * (1.0 / PI) * (
        sun_irradiance * GetSunVisibility(point, sun_direction) +
        sky_irradiance * GetSkyVisibility(point));

    float shadow_length =
        max(0.0, min(shadow_out, distance_to_intersection) - shadow_in) *
        lightshaft_fadein_hack;
    vec3 transmittance;
    vec3 in_scatter = GetSkyRadianceToPoint(cameraPosition.xyz - earth_center,
        point - earth_center, shadow_length, sun_direction, transmittance);
    ground_radiance = ground_radiance * transmittance + in_scatter;
    ground_alpha = 1.0;
  }
   // Compute the radiance of the sky.
  float shadow_length = max(0.0, shadow_out - shadow_in) *
      lightshaft_fadein_hack;
  vec3 transmittance;
  vec3 radiance = GetSkyRadiance(
      cameraPosition.xyz - earth_center, view_direction, shadow_length, sun_direction,
      transmittance);

  // If the view ray intersects the Sun, add the Sun radiance.
  if (dot(view_direction, sun_direction) > Params.sun_size.y) {
    radiance = radiance + transmittance * GetSolarRadiance();
  }
  radiance = mix(radiance, ground_radiance, ground_alpha);
  radiance = mix(radiance, sphere_radiance, sphere_alpha);
  color.rgb = 
      pow(vec3(1.0) - exp(-radiance / Params.white_point * exposure), vec3(1.0 / 2.2));
  color.a = 1.0;*/
  
  color = texture(transmittance_texture, uvCoords);
  //color = vec4(1.0);
}
