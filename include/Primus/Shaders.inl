#ifndef VERTEX_SHADER
  #define VERTEX_SHADER(name, ...)
#endif // !VERTEX_SHADER

VERTEX_SHADER(FullScreen, 
  { "SV_VertexID", 0, DXGI_FORMAT_R32_UINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
)
VERTEX_SHADER(Terrain, 
  { "SV_VertexID ", 0, DXGI_FORMAT_R32_UINT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
)

#undef VERTEX_SHADER


#ifndef PIXEL_SHADER
#define PIXEL_SHADER(name)
#endif // !PIXEL_SHADER

PIXEL_SHADER(ToneMapping)
PIXEL_SHADER(Terrain)

#undef PIXEL_SHADER