#pragma once

#include <d3d12.h>
#include <directxmath.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include "base.h"

using DirectX::XMFLOAT4X4;
using DirectX::XMFLOAT4;
using DirectX::XMMATRIX;
using Microsoft::WRL::ComPtr;

struct InstancedGeometry {
	size_t num_vertices;
	vertex_t* vertices;

	ComPtr<ID3D12Resource> vertex_buffer;
	D3D12_VERTEX_BUFFER_VIEW vertex_buffer_view;

	size_t num_instancies;
	XMFLOAT4X4* instance_matrices;

	ComPtr<ID3D12Resource> instance_buffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW instance_buffer_view = {};

	// num_instancies * sizeof(XMFLOAT4X4)
	size_t instanceBufferSize() { return num_instancies * sizeof(XMFLOAT4X4); }
	void populateCommandList(ComPtr<ID3D12GraphicsCommandList>& command_list);
};


// @TODO:
void makeCuboidInstancedGeometry();



