C:\VulkanSDK\1.1.101.0\Bin\glslangValidator.exe -V atmosphere.vert -o ../../AtmosphereVert.spv
C:\VulkanSDK\1.1.101.0\Bin\glslangValidator.exe -V atmosphere.tesc -o ../../AtmosphereTESC.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe alt_atmosphere.frag -c -o ../../AtmosphereAltFrag.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe alt_atmosphere.tese -c -o ../../AtmosphereAltTESE.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe alt_scattering.comp -c -o ../../AtmosphereAltScattering.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe alt_mult_scattering.comp -c -o ../../AtmosphereAltMultipleScattering.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe alt_transmittance.comp -c -o ../../AtmosphereAltTransmittance.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe alt_gathering.comp -c -o ../../AtmosphereAltGathering.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe ap_scattering.comp -c -o ../../AtmosphereAerialPerspectiveS.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe ap_transmittance.comp -c -o ../../AtmosphereAerialPerspectiveT.spv
pause