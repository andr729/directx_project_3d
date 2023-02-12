#include "drawing.hpp"

void InstancedGeometry::populateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list) {
	command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	command_list->IASetVertexBuffers(
  		0, 1, &vertex_buffer_view
	);

	command_list->IASetVertexBuffers(
  		1, 1, &instance_buffer_view
	);

	command_list->DrawInstanced(
  		num_vertices, num_instancies, 0, 0
	);
}


