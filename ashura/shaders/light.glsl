#version 450
#extension GL_GOOGLE_include_directive : require

#include "color.glsl"

// Vector form without project to the plane (dot with the normal)
// Use for proxy sphere clipping
vec3 integrate_edge_vec(vec3 v1, vec3 v2)
{
  // Using built-in acos() function will result flaws
  // Using fitting result for calculating acos()
  float x = dot(v1, v2);
  float y = abs(x);

  float a = 0.8543985 + (0.4965155 + 0.0145206 * y) * y;
  float b = 3.4175940 + (4.1616724 + y) * y;
  float v = a / b;

  float theta_sintheta =
      (x > 0.0) ? v : 0.5 * inversesqrt(max(1.0 - x * x, 1e-7)) - v;

  return cross(v1, v2) * theta_sintheta;
}

// P is fragPos in world space (LTC distribution)
vec3 ltc_evaluate(vec3 N, vec3 V, vec3 P, mat3 Minv, vec3 points[4],
                  bool twoSided)
{
  // construct orthonormal basis around N
  vec3 T1, T2;
  T1 = normalize(V - N * dot(V, N));
  T2 = cross(N, T1);

  // rotate area light in (T1, T2, N) basis
  Minv = Minv * transpose(mat3(T1, T2, N));
  // Minv = Minv * transpose(mat3(N, T2, T1));

  // polygon (allocate 4 vertices for clipping)
  vec3 L[4];
  // transform polygon from LTC back to origin Do (cosine weighted)
  L[0] = Minv * (points[0] - P);
  L[1] = Minv * (points[1] - P);
  L[2] = Minv * (points[2] - P);
  L[3] = Minv * (points[3] - P);

  // use tabulated horizon-clipped sphere
  // check if the shading point is behind the light
  vec3 dir         = points[0] - P;        // LTC space
  vec3 lightNormal = cross(points[1] - points[0], points[3] - points[0]);
  bool behind      = (dot(dir, lightNormal) < 0.0);

  // cos weighted space
  L[0] = normalize(L[0]);
  L[1] = normalize(L[1]);
  L[2] = normalize(L[2]);
  L[3] = normalize(L[3]);

  // integrate
  vec3 vsum = vec3(0.0);
  vsum += integrate_edge_vec(L[0], L[1]);
  vsum += integrate_edge_vec(L[1], L[2]);
  vsum += integrate_edge_vec(L[2], L[3]);
  vsum += integrate_edge_vec(L[3], L[0]);

  // form factor of the polygon in direction vsum
  float len = length(vsum);

  float z = vsum.z / len;
  if (behind)
    z = -z;

  vec2 uv = vec2(z * 0.5f + 0.5f, len);        // range [0, 1]
  uv      = uv * LUT_SCALE + LUT_BIAS;

  // Fetch the form factor for horizon clipping
  float scale = texture(LTC2, uv).w;

  float sum = len * scale;
  if (!behind && !twoSided)
    sum = 0.0;

  // Outgoing radiance (solid angle) for the entire polygon
  vec3 Lo_i = vec3(sum, sum, sum);
  return Lo_i;
}

void area_light_main()
{
  // gamma correction
  vec3 mDiffuse = texture(material.diffuse, texcoord)
                      .xyz;        // * vec3(0.7f, 0.8f, 0.96f);

  // PBR-maps for roughness (and metallic) are usually stored in non-linear
  // color space (sRGB), so we use these functions to convert into linear RGB.
  vec3 mSpecular = ToLinear(vec3(0.23f, 0.23f, 0.23f));        // mDiffuse

  vec3 result = vec3(0.0f);

  vec3  N     = normalize(worldNormal);
  vec3  V     = normalize(viewPosition - worldPosition);
  vec3  P     = worldPosition;
  float dotNV = clamp(dot(N, V), 0.0f, 1.0f);

  // use roughness and sqrt(1-cos_theta) to sample M_texture
  vec2 uv = vec2(material.albedoRoughness.w, sqrt(1.0f - dotNV));
  uv      = uv * LUT_SCALE + LUT_BIAS;

  // get 4 parameters for inverse_M
  vec4 t1 = texture(LTC1, uv);

  // Get 2 parameters for Fresnel calculation
  vec4 t2 = texture(LTC2, uv);

  mat3 Minv = mat3(vec3(t1.x, 0, t1.y), vec3(0, 1, 0), vec3(t1.z, 0, t1.w));

  // iterate through all area lights
  for (int i = 0; i < numAreaLights; i++)
  {
    // Evaluate LTC shading
    vec3 diffuse  = ltc_evaluate(N, V, P, mat3(1), areaLights[i].points,
                                 areaLights[i].twoSided);
    vec3 specular = ltc_evaluate(N, V, P, Minv, areaLights[i].points,
                                 areaLights[i].twoSided);

    // GGX BRDF shadowing and Fresnel
    // t2.x: shadowedF90 (F90 normally it should be 1.0)
    // t2.y: Smith function for Geometric Attenuation Term, it is dot(V or L,
    // H).
    specular *= mSpecular * t2.x + (1.0f - mSpecular) * t2.y;

    // Add contribution
    result += areaLights[i].color * areaLights[i].intensity *
              (specular + mDiffuse * diffuse);
    // result += vec3(0.5, 0.5, 0.5);
  }

  fragColor = vec4(ToSRGB(result), 1.0f);
}

void dir_light()
{
  vec3  l           = normalize(-lightDirection);
  float NoL         = clamp(dot(n, l), 0.0, 1.0);
  float illuminance = lightIntensity * NoL;
  vec3  luminance   = BSDF(v, l) * illuminance;
}

float getSquareFalloffAttenuation(vec3 posToLight, float lightInvRadius)
{
  float distanceSquare = dot(posToLight, posToLight);
  float factor         = distanceSquare * lightInvRadius * lightInvRadius;
  float smoothFactor   = max(1.0 - factor * factor, 0.0);
  return (smoothFactor * smoothFactor) / max(distanceSquare, 1e-4);
}

float getSpotAngleAttenuation(vec3 l, vec3 lightDir, float innerAngle,
                              float outerAngle)
{
  // the scale and offset computations can be done CPU-side
  float cosOuter    = cos(outerAngle);
  float spotScale   = 1.0 / max(cos(innerAngle) - cosOuter, 1e-4);
  float spotOffset  = -cosOuter * spotScale;
  float cd          = dot(normalize(-lightDir), l);
  float attenuation = clamp(cd * spotScale + spotOffset, 0.0, 1.0);
  return attenuation * attenuation;
}

vec3 evaluatePunctualLight()
{
  vec3  l           = normalize(posToLight);
  float NoL         = clamp(dot(n, l), 0.0, 1.0);
  vec3  posToLight  = lightPosition - worldPosition;
  float attenuation = getSquareFalloffAttenuation(posToLight, lightInvRadius);
  attenuation *= getSpotAngleAttenuation(l, lightDir, innerAngle, outerAngle);
  vec3 luminance =
      (BSDF(v, l) * lightIntensity * attenuation * NoL) * lightColor;
  return luminance;
}

vec3 ibl(vec3 n, vec3 v, vec3 diffuseColor, vec3 f0, vec3 f90,
         float perceptualRoughness)
{
  vec3  r    = reflect(n);
  vec3  Ld   = textureCube(irradianceEnvMap, r) * diffuseColor;
  float lod  = computeLODFromRoughness(perceptualRoughness);
  vec3  Lld  = textureCube(prefilteredEnvMap, r, lod);
  vec2  Ldfg = textureLod(dfgLut, vec2(dot(n, v), perceptualRoughness), 0.0).xy;
  vec3  Lr   = (f0 * Ldfg.x + f90 * Ldfg.y) * Lld;
  return Ld + Lr;
}