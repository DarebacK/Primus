CComPtr<ID3D11Buffer> terrainIndexBuffer;
uint16 terrainIndexBufferIndices[] = {
  0, 1, 2,
  0, 2, 3
};
D3D11_BUFFER_DESC terrainIndexBufferDescription{
  sizeof(terrainIndexBufferIndices),
  D3D11_USAGE_IMMUTABLE,
  D3D11_BIND_INDEX_BUFFER
};
D3D11_SUBRESOURCE_DATA terrainIndexBufferData{ terrainIndexBufferIndices, 0, 0 };