FOR %%I IN (Earth\earth-clouds-hs\textures\hires\earth-clouds-hs-32k\level2\*.dds) DO (
	copy %%I Media\%str:clouds_%%~nI%str:.dds%
)

FOR %%I IN (Earth\earth-nightlights-hs\textures\medres\earth-nightlights-hs-32k\level2\*.dds) DO (
	copy %%I Media\%str:nightlight_%%~nI%str:.dds%
)

FOR %%I IN (Earth\earth-normals-hs\textures\medres\earth-normals-hs-32k\level2\*.dds) DO (
	copy %%I Media\%str:normal_%%~nI%str:.dds%
)

FOR %%I IN (Earth\earth-spec-hs\textures\medres\earth-spec-hs-32k\level2\*.dds) DO (
	copy %%I Media\%str:spec_%%~nI%str:.dds%
)

FOR %%I IN (Earth\earth-unshaded-hs\textures\medres\earth-unshaded-hs-32k\level2\*.dds) DO (
	copy %%I Media\%str:diffuse_%%~nI%str:.dds%
)