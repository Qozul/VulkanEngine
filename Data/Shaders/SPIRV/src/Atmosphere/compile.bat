C:\VulkanSDK\1.1.121.2\Bin\glslangValidator.exe -V atmosphere.vert -o ../../AtmosphereVert.spv
::C:\VulkanSDK\1.1.121.2\Bin\glslc.exe alt_scattering.comp -c -o ../../AtmosphereAltScattering.spv
::C:\VulkanSDK\1.1.121.2\Bin\glslc.exe alt_mult_scattering.comp -c -o ../../AtmosphereAltMultipleScattering.spv
::C:\VulkanSDK\1.1.121.2\Bin\glslc.exe alt_transmittance.comp -c -o ../../AtmosphereAltTransmittance.spv
C:\VulkanSDK\1.1.121.2\Bin\glslc.exe alt_gathering.comp -c -o ../../AtmosphereAltGathering.spv
C:\VulkanSDK\1.1.121.2\Bin\glslc.exe atmosphere.frag -c -o ../../AtmosphereFrag.spv
pause