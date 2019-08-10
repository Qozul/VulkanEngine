C:\VulkanSDK\1.1.101.0\Bin\glslangValidator.exe -V atmosphere.vert -o ../../AtmosphereVert.spv
C:\VulkanSDK\1.1.101.0\Bin\glslangValidator.exe -V atmosphere.tesc -o ../../AtmosphereTESC.spv
C:\VulkanSDK\1.1.101.0\Bin\glslangValidator.exe -V atmosphere.tese -o ../../AtmosphereTESE.spv
C:\VulkanSDK\1.1.101.0\Bin\glslangValidator.exe -V atmosphere.frag -o ../../AtmosphereFrag.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe direct_irradiance.comp -c -o ../../AtmosphereDirectIrradiance.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe indirect_irradiance.comp -c -o ../../AtmosphereIndirectIrradiance.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe transmittance.comp -c -o ../../AtmosphereTransmittance.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe single_scattering.comp -c -o ../../AtmosphereSingleScattering.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe multiple_scattering.comp -c -o ../../AtmosphereMultipleScattering.spv
C:\VulkanSDK\1.1.101.0\Bin\glslc.exe scattering_density.comp -c -o ../../AtmosphereScatteringDensity.spv
pause