CComPtr<ID3D11Buffer> frameConstantBuffer;
D3D11_BUFFER_DESC frameConstantBufferDescription
{
  sizeof(Mat4f),
  D3D11_USAGE_DYNAMIC,
  D3D11_BIND_CONSTANT_BUFFER,
  D3D11_CPU_ACCESS_WRITE
};
