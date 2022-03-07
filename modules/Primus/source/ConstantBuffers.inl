CComPtr<ID3D11Buffer> terrainConstantBuffer;
D3D11_BUFFER_DESC terrainConstantBufferDescription
{
  sizeof(Mat4f),
  D3D11_USAGE_DYNAMIC,
  D3D11_BIND_CONSTANT_BUFFER,
  D3D11_CPU_ACCESS_WRITE
};
