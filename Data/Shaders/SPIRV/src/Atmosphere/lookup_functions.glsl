// Transmittance lookups ----------------

/*
  assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
*/
DimensionlessSpectrum GetTransmittanceToTopAtmosphereBoundary(
    in AtmosphereParameters atmosphere,
    in TransmittanceTexture transmittance_texture,
    Length r, Number mu) {
  vec2 uv = GetTransmittanceTextureUvFromRMu(atmosphere, r, mu);
  return DimensionlessSpectrum(texture(transmittance_texture, uv));
}
/*
  assert(r >= atmosphere.bottom_radius && r <= atmosphere.top_radius);
  assert(mu >= -1.0 && mu <= 1.0);
  assert(d >= 0.0 * m);
*/
DimensionlessSpectrum GetTransmittance(
    in AtmosphereParameters atmosphere,
    in TransmittanceTexture transmittance_texture,
    Length r, Number mu, Length d, bool ray_r_mu_intersects_ground) {
  Length r_d = ClampRadius(atmosphere, sqrt(d * d + 2.0 * r * mu * d + r * r));
  Number mu_d = ClampCosine((r * mu + d) / r_d);

  if (ray_r_mu_intersects_ground) {
    return min(
        GetTransmittanceToTopAtmosphereBoundary(
            atmosphere, transmittance_texture, r_d, -mu_d) /
        GetTransmittanceToTopAtmosphereBoundary(
            atmosphere, transmittance_texture, r, -mu),
        DimensionlessSpectrum(1.0));
  } else {
    return min(
        GetTransmittanceToTopAtmosphereBoundary(
            atmosphere, transmittance_texture, r, mu) /
        GetTransmittanceToTopAtmosphereBoundary(
            atmosphere, transmittance_texture, r_d, mu_d),
        DimensionlessSpectrum(1.0));
  }
}

DimensionlessSpectrum GetTransmittanceToSun(
    in AtmosphereParameters atmosphere,
    in TransmittanceTexture transmittance_texture,
    Length r, Number mu_s) {
  Number sin_theta_h = atmosphere.bottom_radius / r;
  Number cos_theta_h = -sqrt(max(1.0 - sin_theta_h * sin_theta_h, 0.0));
  return GetTransmittanceToTopAtmosphereBoundary(
          atmosphere, transmittance_texture, r, mu_s) *
      smoothstep(-sin_theta_h * atmosphere.sun_angular_radius / rad,
                 sin_theta_h * atmosphere.sun_angular_radius / rad,
                 mu_s - cos_theta_h);
}

// Single-scattering lookups ----------------

AbstractSpectrum GetScattering(in AtmosphereParameters atmosphere, in AbstractScatteringTexture scattering_texture,
    Length r, Number mu, Number mu_s, Number nu, bool ray_r_mu_intersects_ground) {
  vec4 uvwz = GetScatteringTextureUvwzFromRMuMuSNu(
      atmosphere, r, mu, mu_s, nu, ray_r_mu_intersects_ground);
  Number tex_coord_x = uvwz.x * Number(SCATTERING_TEXTURE_NU_SIZE - 1);
  Number tex_x = floor(tex_coord_x);
  Number lerp = tex_coord_x - tex_x;
  vec3 uvw0 = vec3((tex_x + uvwz.y) / Number(SCATTERING_TEXTURE_NU_SIZE),
      uvwz.z, uvwz.w);
  vec3 uvw1 = vec3((tex_x + 1.0 + uvwz.y) / Number(SCATTERING_TEXTURE_NU_SIZE),
      uvwz.z, uvwz.w);
  return AbstractSpectrum(texture(scattering_texture, uvw0) * (1.0 - lerp) +
      texture(scattering_texture, uvw1) * lerp);
}

RadianceSpectrum GetScattering(in AtmosphereParameters atmosphere,
    in ReducedScatteringTexture single_rayleigh_scattering_texture,
    in ReducedScatteringTexture single_mie_scattering_texture,
    in ScatteringTexture multiple_scattering_texture,
    Length r, Number mu, Number mu_s, Number nu, bool ray_r_mu_intersects_ground, int scattering_order) {
  if (scattering_order == 1) {
    IrradianceSpectrum rayleigh = GetScattering(
        atmosphere, single_rayleigh_scattering_texture, r, mu, mu_s, nu,
        ray_r_mu_intersects_ground);
    IrradianceSpectrum mie = GetScattering(
        atmosphere, single_mie_scattering_texture, r, mu, mu_s, nu,
        ray_r_mu_intersects_ground);
    return rayleigh * RayleighPhaseFunction(nu) +
        mie * MiePhaseFunction(atmosphere.mie_phase_function_g, nu);
  } else {
    return GetScattering(
        atmosphere, multiple_scattering_texture, r, mu, mu_s, nu,
        ray_r_mu_intersects_ground);
  }
}

// Irradiance lookups ---------------------

IrradianceSpectrum GetIrradiance( in AtmosphereParameters atmosphere, in IrradianceTexture irradiance_texture,
    Length r, Number mu_s) {
  vec2 uv = GetIrradianceTextureUvFromRMuS(atmosphere, r, mu_s);
  return IrradianceSpectrum(texture(irradiance_texture, uv));
}
