#include <d3d12.h>
#include <directxmath.h>
#include <dxgi1_6.h>
#include <wincodec.h>
#include <wrl.h>
#include <utility>
#include <cmath>
#include <exception>
#include <stdexcept>
#include "D3DApp.h"
#include "vertex_shader.h",
#include "pixel_shader.h".
#include "base.h"
#include "maze.h"
#include "mazephysics.hpp"
#include "global_state.hpp"
#include "bitmap.hpp"

using DirectX::XMFLOAT4X4;
using DirectX::XMFLOAT4;
using DirectX::XMMATRIX;
using namespace DirectX;

struct vs_const_buffer_t {
	XMFLOAT4X4 matWorldViewProj;
	XMFLOAT4X4 matWorldView;
	XMFLOAT4X4 matView;

	XMFLOAT4 colMaterial;
	XMFLOAT4 colLight;
	XMFLOAT4 dirLight;
	XMFLOAT4 padding;
};
static_assert(sizeof(vs_const_buffer_t) == 256);

inline void ThrowIfFailed(HRESULT hr) {
	if (FAILED(hr)) {
		throw std::logic_error("Bad HR");
	}
}

namespace {
	const INT FrameCount = 2;
	using Microsoft::WRL::ComPtr;
	ComPtr<IDXGISwapChain3> swapChain;
	ComPtr<IDXGIFactory7> factory;
	ComPtr<ID3D12Device> device;
	ComPtr<ID3D12Resource> renderTargets[FrameCount];
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ComPtr<ID3D12CommandQueue> commandQueue;
	
	ComPtr<ID3D12RootSignature> rootSignature;

	typedef ComPtr<ID3D12DescriptorHeap> HeapType;
	HeapType rtvHeap;
	HeapType cbvHeap;

	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle;
	ComPtr<ID3D12PipelineState> pipelineState;
	ComPtr<ID3D12GraphicsCommandList> commandList;
	UINT rtvDescriptorSize;

	ComPtr<ID3D12Fence> fence;
	UINT frameIndex;
	UINT64 fenceValue;
	HANDLE fenceEvent;

	D3D12_VIEWPORT viewport;

	ComPtr<ID3D12Resource> vertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView;

	ComPtr<ID3D12Resource> vsConstBuffer;
	ComPtr<ID3D12Resource> texture_resource;
	
	// Matrices are assigned dynamicly 
	vs_const_buffer_t vsConstBufferData = {
		.colMaterial = {0.4, 1, 0.4, 1},
		.colLight = {1, 1, 1, 1},
		.dirLight = {0.0, 0.0, 1.0, 1},
	};
	UINT8* vsConstBufferPointer;

	RECT rc;

	ComPtr<ID3D12Resource> depthBuffer;
	HeapType depthBufferHeap;

	
	constexpr size_t VERTEX_COUNT = CUBOID_VERTEX_COUNT + HEXPRISM_VERTEX_COUNT;
	constexpr size_t CUBOID_START_POSITION = 0;
	constexpr size_t HEXPRISM_START_POSITION = CUBOID_VERTEX_COUNT;
	static_assert(VERTEX_COUNT % 3 == 0);

	constexpr size_t NUM_TRIANGLES = VERTEX_COUNT / 3;

	vertex_t triangle_data[VERTEX_COUNT];

	constexpr size_t VERTEX_BUFFER_SIZE = sizeof(triangle_data);


	size_t NUM_HEXPRISM_INSTANCES;
	size_t NUM_CUBOID_INSTANCES;

	size_t CUBOID_INSTANCE_DATA_START;
	size_t HEXPRISM_INSTANCE_DATA_START;

	constinit size_t const MAX_NUM_INSTANCES = 2048;
	XMFLOAT4X4 instance_matrices[MAX_NUM_INSTANCES];
	
	constinit const size_t INSTANCE_BUFFER_SIZE = sizeof(instance_matrices);

	ComPtr<ID3D12Resource> instance_buffer = nullptr;
	D3D12_VERTEX_BUFFER_VIEW instance_buffer_view = {};


	ObjectHandler obj_handler;

}

namespace player_state {
	Vector2 position = {0, 0};
	float height = 0;
	float rotY = 0;
	float rotUpDown = 0;

	void rotateUpDown(float value) {
		rotUpDown += value;
		
		// @TODO:
		if (rotUpDown < -1) rotUpDown = -1;
		if (rotUpDown > 1) rotUpDown = 1;
	}

	void rotateY(float value) {
		rotY += value;
		rotY = fmodf(rotY, 3.14 * 2);
	}

	void move(float dx, float dy) {
		auto new_position = position + 
			Vector2{
				cos(rotY) * dx - sin(rotY) * dy, 
				sin(rotY) * dx + cos(rotY) * dy
			};
		
		RectangleObj player = {{new_position, 0}, 0.2, 0.2};
		if (obj_handler.collidesWith(player)) {
			return;
		}

		position = new_position;
	}

	void moveUp(float dz) {
		height += dz;
		height = min(height, 10);
		height = max(height, 0); 
	}
};

void initTriangleAndInstanceData() {
	const float length = 1;
	const float width = .1;
	const float height = .2;
	const int side_edges = 1;

	auto maze = getMaze(length, width, height, side_edges, 1);

	for(int i = 0; i < CUBOID_VERTEX_COUNT; i++)
		triangle_data[i + CUBOID_START_POSITION] = maze.cuboid[i];

	for(int i = 0; i < HEXPRISM_VERTEX_COUNT; i++)
		triangle_data[i + HEXPRISM_START_POSITION] = maze.hexprism[i];

	NUM_CUBOID_INSTANCES = maze.transformations_cuboid.size();
	NUM_HEXPRISM_INSTANCES = maze.transformations_hexprism.size();

	CUBOID_INSTANCE_DATA_START = 0;
	HEXPRISM_INSTANCE_DATA_START = NUM_CUBOID_INSTANCES;

	assert(NUM_CUBOID_INSTANCES + NUM_HEXPRISM_INSTANCES < MAX_NUM_INSTANCES);

	for (size_t k = 0;
			k < NUM_CUBOID_INSTANCES;
			++k) {
		auto& trn = maze.transformations_cuboid[k];

		XMStoreFloat4x4(
			&instance_matrices[k + CUBOID_INSTANCE_DATA_START],
			XMMatrixMultiply( 
				XMMatrixRotationY(trn.rotation),
				XMMatrixTranslation(trn.translation.x, 0, trn.translation.y)
			)
		);
	}

	for (size_t k = 0;
			k < NUM_HEXPRISM_INSTANCES;
			++k) {
		auto& trn = maze.transformations_hexprism[k];

		XMStoreFloat4x4(
			&instance_matrices[k + HEXPRISM_INSTANCE_DATA_START],
			XMMatrixTranslation(trn.translation.x, 0, trn.translation.y)
		);
	}


	// Making objects:
	for(int i = 0; i < maze.transformations_cuboid.size(); i++) {
		auto obj = new RectangleObj{maze.transformations_cuboid[i], length, width};
		obj_handler.addObject(obj);
	}

	for(int i = 0; i < maze.transformations_hexprism.size(); i++) {
		auto obj = new HexObj(maze.transformations_hexprism[i], width);
		obj_handler.addObject(obj);
	}
}

void copyConstBufferToGpu() {
	memcpy(
		vsConstBufferPointer,
		&vsConstBufferData,
		sizeof(vsConstBufferData)
	);
}

void copyTriangleDataToVertexBuffer() {
	UINT8* pVertexDataBegin;
	D3D12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
	ThrowIfFailed(vertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
	memcpy(pVertexDataBegin, triangle_data, sizeof(triangle_data));
	vertexBuffer->Unmap(0, nullptr);
}

void calcNewMatrix() {

	XMStoreFloat4x4(
		&vsConstBufferData.matWorldView,
		XMMatrixIdentity()
	);

	XMStoreFloat4x4(
		&vsConstBufferData.matView,
		XMMatrixIdentity()
	);

	XMMATRIX wvp_matrix = XMMatrixIdentity();

	// player stuff:
	wvp_matrix = XMMatrixMultiply(
		wvp_matrix,
		XMMatrixTranslation(-player_state::position.x, -player_state::height, -player_state::position.y)
	);

	wvp_matrix = XMMatrixMultiply(
		wvp_matrix,
		XMMatrixRotationY(player_state::rotY)
	);

	// x or z:
	wvp_matrix = XMMatrixMultiply(
		wvp_matrix,
		XMMatrixRotationX(player_state::rotUpDown)
	);

	wvp_matrix = XMMatrixMultiply(
		wvp_matrix, 
		XMMatrixPerspectiveFovLH(
			45.0f, viewport.Width / viewport.Height, 0.1f, 100.0f
		)
	);
	wvp_matrix = XMMatrixTranspose(wvp_matrix);
	XMStoreFloat4x4(
		&vsConstBufferData.matWorldViewProj, 	// zmienna typu vs_const_buffer_t z pkt. 2d
		wvp_matrix
	);

	copyConstBufferToGpu();
}

void WaitForPreviousFrame(HWND hwnd);

namespace DXInitAux {
	void inline createHeap(const D3D12_DESCRIPTOR_HEAP_DESC& desc, HeapType& heap) {
		ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&heap)));
		if (heap == nullptr) {
			throw std::logic_error("Heap is a nullptr");
		}
	}

	void inline createBasicCommittedResource(
		const D3D12_HEAP_PROPERTIES *pHeapProperties,
		const D3D12_RESOURCE_DESC *pDesc,
		ComPtr<ID3D12Resource>& resource) {

		ThrowIfFailed(device->CreateCommittedResource(
			pHeapProperties,
			D3D12_HEAP_FLAG_NONE,
			pDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&resource)
		));
	}

	void initWicFactory() {
		// @TODO: this does not work for some reason
		ThrowIfFailed(CoCreateInstance(
			CLSID_WICImagingFactory,
			nullptr,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&img_factory)
		));
	}

	void initDepthBuffer() {
		D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV,
			.NumDescriptors = 1,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
			.NodeMask = 0,
		};
		D3D12_HEAP_PROPERTIES heapProp = {
			.Type = D3D12_HEAP_TYPE_DEFAULT,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,      
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1,
		};
		D3D12_RESOURCE_DESC resDesc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0,
			.Width = UINT64(rc.right - rc.left),
			.Height = UINT64(rc.bottom - rc.top),
			.DepthOrArraySize = 1,
			.MipLevels = 0,
			.Format = DXGI_FORMAT_D32_FLOAT,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL,
		};
		// D3D12_CLEAR_VALUE clearValue = {
		// 	.Format = DXGI_FORMAT_D32_FLOAT,
		// 	.DepthStencil = { .Depth = 1.0f, .Stencil = 0 }
		// };
		D3D12_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc = {
			.Format = DXGI_FORMAT_D32_FLOAT,
			.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D,
			.Flags = D3D12_DSV_FLAG_NONE,
			.Texture2D = {}
		};

		createHeap(heapDesc, depthBufferHeap);

		DXInitAux::createBasicCommittedResource(&heapProp, &resDesc, depthBuffer);
		
		device->CreateDepthStencilView(
			depthBuffer.Get(),
			&depthStencilViewDesc,
			depthBufferHeap->GetCPUDescriptorHandleForHeapStart()
		);
	}

	void initDeviceAndFactory() {
		ThrowIfFailed(CreateDXGIFactory2(0, IID_PPV_ARGS(&factory)));

		ThrowIfFailed(D3D12CreateDevice(
			nullptr,
			D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&device)
		));
	}

	void initViewPort() {
		viewport = {
			.TopLeftX = 0.f,
			.TopLeftY = 0.f,
			.Width = FLOAT(rc.right - rc.left),
			.Height = FLOAT(rc.bottom - rc.top),
			.MinDepth = 0.0f,
			.MaxDepth = 1.0f
		};
	}

	void initSwapChain(HWND hwnd) {
			DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {
			.Width = 0,
			.Height = 0,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc = { .Count = 1 },
			.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT,
			.BufferCount = FrameCount,
			.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD,
		};

		ComPtr<IDXGISwapChain1> tempSwapChain;
		ThrowIfFailed(factory->CreateSwapChainForHwnd(
			commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
			hwnd,
			&swapChainDesc,
			nullptr,
			nullptr,
			&tempSwapChain
		));
		ThrowIfFailed(factory->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));
		ThrowIfFailed(tempSwapChain.As(&swapChain));

		frameIndex = swapChain->GetCurrentBackBufferIndex();
	}

	void initVertexBuffer() {
		// Note: using upload heaps to transfer static data like vert buffers is not 
		// recommended. Every time the GPU needs it, the upload heap will be marshalled 
		// over. Please read up on Default Heap usage. An upload heap is used here for 
		// code simplicity and because there are very few verts to actually transfer.
		D3D12_HEAP_PROPERTIES heapProps = {
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1,
		};

		D3D12_RESOURCE_DESC desc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
			.Width = VERTEX_BUFFER_SIZE,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE,
		};
   
		DXInitAux::createBasicCommittedResource(&heapProps, &desc, vertexBuffer);
		copyTriangleDataToVertexBuffer();

		// Initialize the vertex buffer view.
		vertexBufferView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();
		vertexBufferView.StrideInBytes = sizeof(vertex_t);
		vertexBufferView.SizeInBytes = VERTEX_BUFFER_SIZE;
	}

	void initCommandQueue() {
		D3D12_COMMAND_QUEUE_DESC queueDesc = {
			.Type = D3D12_COMMAND_LIST_TYPE_DIRECT,
			.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE,
		};
		ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));
	}

	void initCBVRTVHeaps() {
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV,
			.NumDescriptors = FrameCount,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE,
		};


		D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc = {
			.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV,
			.NumDescriptors = 2,
			.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
			.NodeMask = 0  
		};

		DXInitAux::createHeap(rtvHeapDesc, rtvHeap);
		DXInitAux::createHeap(cbvHeapDesc, cbvHeap);
	}

	void initPipelineState() {
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] = {
			{
				.SemanticName = "POSITION",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			{
				.SemanticName = "NORMAL",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32B32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			{
				.SemanticName = "COLOR",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			{
				.SemanticName = "TEXCOORD",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32_FLOAT,
				.InputSlot = 0,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
				.InstanceDataStepRate = 0
			},
			{ // Pierwszy wiersz macierzy instancji
				.SemanticName = "WORLD",
				.SemanticIndex = 0,
				.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
				.InputSlot = 1,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass =
				D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
				.InstanceDataStepRate = 1
			},
			{  // Drugi wiersz macierzy instancji
				.SemanticName = "WORLD",
				.SemanticIndex = 1,
				.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
				.InputSlot = 1,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass =
				D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
				.InstanceDataStepRate = 1
			},
			{  // Trzeci wiersz macierzy instancji
				.SemanticName = "WORLD",
				.SemanticIndex = 2,
				.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
				.InputSlot = 1,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass =
				D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
				.InstanceDataStepRate = 1
			},
			{  // Czwarty wiersz macierzy instancji
				.SemanticName = "WORLD",
				.SemanticIndex = 3,
				.Format = DXGI_FORMAT_R32G32B32A32_FLOAT,
				.InputSlot = 1,
				.AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT,
				.InputSlotClass =
				D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA,
				.InstanceDataStepRate = 1
			}

		};

		D3D12_BLEND_DESC blendDesc = {
			.AlphaToCoverageEnable = FALSE,
			.IndependentBlendEnable = FALSE,
			.RenderTarget = {
				{
				.BlendEnable = FALSE,
				.LogicOpEnable = FALSE,
				.SrcBlend = D3D12_BLEND_ONE,
				.DestBlend = D3D12_BLEND_ZERO,
				.BlendOp = D3D12_BLEND_OP_ADD,
				.SrcBlendAlpha = D3D12_BLEND_ONE,
				.DestBlendAlpha = D3D12_BLEND_ZERO,
				.BlendOpAlpha = D3D12_BLEND_OP_ADD,
				.LogicOp = D3D12_LOGIC_OP_NOOP,
				.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL
				}
			}
		};

		D3D12_RASTERIZER_DESC rasterizerDesc = {
			.FillMode = D3D12_FILL_MODE_SOLID,
			.CullMode = D3D12_CULL_MODE_BACK,
			.FrontCounterClockwise = FALSE,
			.DepthBias = D3D12_DEFAULT_DEPTH_BIAS,
			.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP,
			.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS,
			.DepthClipEnable = TRUE,
			.MultisampleEnable = FALSE,
			.AntialiasedLineEnable = FALSE,
			.ForcedSampleCount = 0,
			.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF
		};

		D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {
			.DepthEnable = TRUE,
			.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL,
			.DepthFunc = D3D12_COMPARISON_FUNC_LESS,
			.StencilEnable = FALSE,
			.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK,
			.StencilWriteMask = D3D12_DEFAULT_STENCIL_READ_MASK,
			.FrontFace = {
				.StencilFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilPassOp = D3D12_STENCIL_OP_KEEP,
				.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
			},
			.BackFace = {
				.StencilFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP,
				.StencilPassOp = D3D12_STENCIL_OP_KEEP,
				.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS
			}
		};

		// D3D12_GRAPHICS_PIPELINE_STATE_DESC pipelineStateDesc = {
		// 	.DepthStencilState = depthStencilDesc,
		// 	.DSVFormat = DXGI_FORMAT_D32_FLOAT,
		// };

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {
			.pRootSignature = rootSignature.Get(),
			.VS = { vs_main, sizeof(vs_main) },
			.PS = { ps_main, sizeof(ps_main) },
			.BlendState = blendDesc,
			.SampleMask = UINT_MAX,
			.RasterizerState = rasterizerDesc,
			.DepthStencilState = depthStencilDesc,
			.InputLayout = { inputElementDescs, _countof(inputElementDescs) },
			.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE,
			.NumRenderTargets = 1,
			.DSVFormat = DXGI_FORMAT_D32_FLOAT,
			.SampleDesc = {.Count = 1},
		};
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

		ThrowIfFailed(device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState)));
	}

	void initRootSignature() {
		D3D12_DESCRIPTOR_RANGE rootDescRange[] = {
			{
				.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV,
				.NumDescriptors = 1,
				.BaseShaderRegister = 0,
				.RegisterSpace = 0,
				.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND,
			},
			{ 
				.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV,
				.NumDescriptors = 1,
				.BaseShaderRegister = 0,
				.RegisterSpace = 0,
				.OffsetInDescriptorsFromTableStart =
				D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND
			}

		};

		D3D12_ROOT_PARAMETER rootParameter[] = {
			{
				.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				.DescriptorTable = { 1, &rootDescRange[0]},	// adr. rekordu poprzedniego typu
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX,
			},
			{
				.ParameterType =
				D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE,
				.DescriptorTable = { 1, &rootDescRange[1]},
				.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
			}
		};

		D3D12_STATIC_SAMPLER_DESC tex_sampler_desc = {
			.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR,   
				//D3D12_FILTER_MIN_MAG_MIP_POINT, D3D12_FILTER_ANISOTROPIC
			.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP,    
				//_MODE_MIRROR, _MODE_CLAMP, _MODE_BORDER
			.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP,
			.MipLODBias = 0,
			.MaxAnisotropy = 0,
			.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER,
			.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK,
			.MinLOD = 0.0f,
			.MaxLOD = D3D12_FLOAT32_MAX,
			.ShaderRegister = 0,
			.RegisterSpace = 0,
			.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL
		};


		D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {
			.NumParameters = _countof(rootParameter),
			.pParameters = rootParameter,
			.NumStaticSamplers = 1,
			.pStaticSamplers = &tex_sampler_desc,
			.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
					D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
					D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
					D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS
					// | D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS
					,
		};

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(
			&rootSignatureDesc, 
			D3D_ROOT_SIGNATURE_VERSION_1,
			&signature, &error
		));
		ThrowIfFailed(device->CreateRootSignature(
			0,
			signature->GetBufferPointer(), signature->GetBufferSize(),
			IID_PPV_ARGS(&rootSignature)
		));
	}

	void initVsConstBufferResourceAndView() {
		D3D12_HEAP_PROPERTIES vsHeapTypeProp = {
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1,
		};
		D3D12_RESOURCE_DESC vsHeapResourceDesc =  {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
			.Width = sizeof(vs_const_buffer_t),
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = { .Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE,
		};

		DXInitAux::createBasicCommittedResource(&vsHeapTypeProp, &vsHeapResourceDesc, vsConstBuffer);

		D3D12_CONSTANT_BUFFER_VIEW_DESC vbViewDesc = {
			.BufferLocation = vsConstBuffer->GetGPUVirtualAddress(),
			.SizeInBytes = sizeof(vs_const_buffer_t),
		};
		device->CreateConstantBufferView(&vbViewDesc, cbvHeap->GetCPUDescriptorHandleForHeapStart());

	}

	void initCommandAllocatorAndList() {
		rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		rtvHandle = rtvHeap->GetCPUDescriptorHandleForHeapStart();
		for (UINT n = 0; n < FrameCount; n++) {
			ThrowIfFailed(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
			device->CreateRenderTargetView(renderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.ptr += rtvDescriptorSize;
		}

		ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator)));
		
		ThrowIfFailed(device->CreateCommandList(
			0, D3D12_COMMAND_LIST_TYPE_DIRECT, 
			commandAllocator.Get(), pipelineState.Get(), 
			IID_PPV_ARGS(&commandList)
		));
		ThrowIfFailed(commandList->Close());
	}

	void initInstanceBuffer() {
		D3D12_HEAP_PROPERTIES heap_prop = {
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};
		D3D12_RESOURCE_DESC resource_desc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
			.Width = INSTANCE_BUFFER_SIZE,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};
		device->CreateCommittedResource(
			&heap_prop,
			D3D12_HEAP_FLAG_NONE,
			&resource_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&instance_buffer)
		);

		UINT8* dst_data = nullptr;
		D3D12_RANGE read_range = { 0, 0 };
		instance_buffer->Map(
			0, &read_range, reinterpret_cast<void**>(&dst_data)
		);
		memcpy(dst_data, instance_matrices, INSTANCE_BUFFER_SIZE);
		instance_buffer->Unmap(0, nullptr);

		instance_buffer_view.BufferLocation = 
			instance_buffer->GetGPUVirtualAddress();
		instance_buffer_view.SizeInBytes = INSTANCE_BUFFER_SIZE;
		instance_buffer_view.StrideInBytes = sizeof(XMFLOAT4X4);
	}

	void initTextureView(HWND hwnd) {
		// Budowa właściwego zasobu tekstury
		D3D12_HEAP_PROPERTIES tex_heap_prop = {
			.Type = D3D12_HEAP_TYPE_DEFAULT,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};
			D3D12_RESOURCE_DESC tex_resource_desc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D,
			.Alignment = 0,
			.Width = bmp_width,
			.Height = bmp_height,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_R8G8B8A8_UNORM,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};

		device->CreateCommittedResource(
			&tex_heap_prop, D3D12_HEAP_FLAG_NONE,
			&tex_resource_desc, D3D12_RESOURCE_STATE_COPY_DEST,
			nullptr, IID_PPV_ARGS(&texture_resource)
		);

		// Budowa pomocniczego bufora wczytania tekstury do GPU
		ComPtr<ID3D12Resource> texture_upload_buffer = nullptr;
		
		// - ustalenie rozmiaru tego pom. bufora
		UINT64 RequiredSize = 0;
		auto Desc = texture_resource.Get()->GetDesc();
		ID3D12Device* pDevice = nullptr;
		texture_resource.Get()->GetDevice(
			__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice)
		);

		pDevice->GetCopyableFootprints(
			&Desc, 0, 1, 0, nullptr, nullptr, nullptr, &RequiredSize
		);

		pDevice->Release();
		
		// - utworzenie pom. bufora
		D3D12_HEAP_PROPERTIES tex_upload_heap_prop = {
			.Type = D3D12_HEAP_TYPE_UPLOAD,
			.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
			.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN,
			.CreationNodeMask = 1,
			.VisibleNodeMask = 1
		};
		
		D3D12_RESOURCE_DESC tex_upload_resource_desc = {
			.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER,
			.Alignment = 0,
			.Width = RequiredSize,
			.Height = 1,
			.DepthOrArraySize = 1,
			.MipLevels = 1,
			.Format = DXGI_FORMAT_UNKNOWN,
			.SampleDesc = {.Count = 1, .Quality = 0 },
			.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
			.Flags = D3D12_RESOURCE_FLAG_NONE
		};
		
		device->CreateCommittedResource(
			&tex_upload_heap_prop, D3D12_HEAP_FLAG_NONE,
			&tex_upload_resource_desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr, IID_PPV_ARGS(&texture_upload_buffer)
		);
		
		// - skopiowanie danych tekstury do pom. bufora
		D3D12_SUBRESOURCE_DATA texture_data = {
			.pData = bmp_bits.data(),
			.RowPitch = bmp_width * bmp_px_size,
			.SlicePitch = bmp_width * bmp_height * bmp_px_size
		};
		
		//@TODO: jakie reset?
		// commandList->Reset(commandAllocator.Get());
		// ... ID3D12GraphicsCommandList::Reset() - to dlatego lista
		// poleceń i obiekt stanu potoku muszą być wcześniej utworzone
		// ThrowIfFailed(commandAllocator->Reset());
		ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));

		UINT const MAX_SUBRESOURCES = 1;
		RequiredSize = 0;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT Layouts[MAX_SUBRESOURCES];
		UINT NumRows[MAX_SUBRESOURCES];
		UINT64 RowSizesInBytes[MAX_SUBRESOURCES];
		Desc = texture_resource.Get()->GetDesc();
		pDevice = nullptr;
		texture_resource.Get()->GetDevice(
			__uuidof(*pDevice), reinterpret_cast<void**>(&pDevice)
		);
		
		pDevice->GetCopyableFootprints(
			&Desc, 0, 1, 0, Layouts, NumRows, 
			RowSizesInBytes, &RequiredSize
		);
		pDevice->Release();

		BYTE* map_tex_data = nullptr;
		texture_upload_buffer->Map(
			0, nullptr, reinterpret_cast<void**>(&map_tex_data)
		);
		D3D12_MEMCPY_DEST DestData = {
			.pData = map_tex_data + Layouts[0].Offset,
			.RowPitch = Layouts[0].Footprint.RowPitch,
			.SlicePitch = 
				SIZE_T(Layouts[0].Footprint.RowPitch) * SIZE_T(NumRows[0])
		};
		for (UINT z = 0; z < Layouts[0].Footprint.Depth; ++z) {
			auto pDestSlice =
				static_cast<UINT8*>(DestData.pData) 
				+ DestData.SlicePitch * z;
			auto pSrcSlice =
				static_cast<const UINT8*>(texture_data.pData)
				+ texture_data.SlicePitch * LONG_PTR(z);
			for (UINT y = 0; y < NumRows[0]; ++y) {
				memcpy(
				pDestSlice + DestData.RowPitch * y,
				pSrcSlice + texture_data.RowPitch * LONG_PTR(y),
				static_cast<SIZE_T>(RowSizesInBytes[0])
				);
			}
		}
		texture_upload_buffer->Unmap(0, nullptr);
		
		// -  zlecenie procesorowi GPU jego skopiowania do właściwego
		//    zasobu tekstury
		D3D12_TEXTURE_COPY_LOCATION Dst = {
			.pResource = texture_resource.Get(),
			.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX,
			.SubresourceIndex = 0
		};
		D3D12_TEXTURE_COPY_LOCATION Src = {
			.pResource = texture_upload_buffer.Get(),
			.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT,
			.PlacedFootprint = Layouts[0]
		};
		commandList->CopyTextureRegion(
			&Dst, 0, 0, 0, &Src, nullptr
		);

		D3D12_RESOURCE_BARRIER tex_upload_resource_barrier = {
			.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
			.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
			.Transition = {
				.pResource = texture_resource.Get(),
				.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
				.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST,
				.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
			},
		};
		
		commandList->ResourceBarrier(
			1, &tex_upload_resource_barrier
		);       
		commandList->Close();
		ID3D12CommandList* cmd_list = commandList.Get();
		commandQueue->ExecuteCommandLists(1, &cmd_list);

		// - tworzy SRV (widok zasobu shadera) dla tekstury
		D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {
			.Format = tex_resource_desc.Format,
			.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D,
			.Shader4ComponentMapping =
				D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING,
			.Texture2D = {
				.MostDetailedMip = 0,
				.MipLevels = 1,
				.PlaneSlice = 0,
				.ResourceMinLODClamp = 0.0f
			},
		};

		D3D12_CPU_DESCRIPTOR_HANDLE cpu_desc_handle = 
		// @TODO: is it cbv?
			cbvHeap->GetCPUDescriptorHandleForHeapStart();
		
		cpu_desc_handle.ptr +=
			device->GetDescriptorHandleIncrementSize(
				D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
			);
		
		device->CreateShaderResourceView(
			texture_resource.Get(), &srv_desc, cpu_desc_handle 
		);

		//@TODO: jakie wait?

		// ... WaitForGPU() - nie można usuwać texture_upload_buffer
		// zanim nie skopiuje się jego zawartości
		WaitForPreviousFrame(hwnd);
	
	}
}

void PopulateCommandList(HWND hwnd) {
	ThrowIfFailed(commandAllocator->Reset());
	ThrowIfFailed(commandList->Reset(commandAllocator.Get(), pipelineState.Get()));

	commandList->SetGraphicsRootSignature(rootSignature.Get());
	
	ID3D12DescriptorHeap* ppHeaps[] = { cbvHeap.Get() };
	commandList->SetDescriptorHeaps(_countof(ppHeaps), ppHeaps);

	D3D12_GPU_DESCRIPTOR_HANDLE gpu_desc_handle =
	// @TODO: this heap?
		cbvHeap->GetGPUDescriptorHandleForHeapStart();
	
	commandList->SetGraphicsRootDescriptorTable(
		0, gpu_desc_handle
	);
	
	gpu_desc_handle.ptr +=
		device->GetDescriptorHandleIncrementSize(
			D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV
		);
	
	commandList->SetGraphicsRootDescriptorTable(
		1, gpu_desc_handle
	);
	
	commandList->RSSetViewports(1, &viewport);
	commandList->RSSetScissorRects(1, &rc);

	D3D12_RESOURCE_BARRIER barrier = {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition = {
			.pResource = renderTargets[frameIndex].Get(),
			.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			.StateBefore = D3D12_RESOURCE_STATE_PRESENT,
			.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET
		}
	};
	commandList->ResourceBarrier(1, &barrier);

	auto rtvHandleHeapStart = rtvHeap->GetCPUDescriptorHandleForHeapStart();
	rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	
	rtvHandleHeapStart.ptr += frameIndex * rtvDescriptorSize;
	
	D3D12_CPU_DESCRIPTOR_HANDLE cpudesc = depthBufferHeap->GetCPUDescriptorHandleForHeapStart();
	
	commandList->OMSetRenderTargets(
		1, &rtvHandleHeapStart,
		FALSE, 
		&cpudesc
	);

	const float clearColor[] = { 0.0f, 0.8f, 0.8f, 1.0f };
	commandList->ClearRenderTargetView(rtvHandleHeapStart, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(
		cpudesc,
		D3D12_CLEAR_FLAG_DEPTH , 1.0f, 0, 0, nullptr
	);

	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	commandList->IASetVertexBuffers(
  		0, 1, &vertexBufferView // widok buf. wierzch. ze źdźbłem
	);

	commandList->IASetVertexBuffers(
  		1, 1, &instance_buffer_view
	);

	commandList->DrawInstanced(
		CUBOID_VERTEX_COUNT, 
		NUM_CUBOID_INSTANCES, 
		CUBOID_START_POSITION, 
		CUBOID_INSTANCE_DATA_START
	);

	commandList->DrawInstanced(
  		HEXPRISM_VERTEX_COUNT, 
		NUM_HEXPRISM_INSTANCES, 
		HEXPRISM_START_POSITION, 
		HEXPRISM_INSTANCE_DATA_START
	);

	D3D12_RESOURCE_BARRIER barrier2 = {
		.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION,
		.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE,
		.Transition = {
			.pResource = renderTargets[frameIndex].Get(),
			.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES,
			.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET,
			.StateAfter = D3D12_RESOURCE_STATE_PRESENT
		}
	};
	commandList->ResourceBarrier(1, &barrier2);

	ThrowIfFailed(commandList->Close());
}

void WaitForPreviousFrame(HWND hwnd) {
	// WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
	// This is code implemented as such for simplicity. More advanced samples 
	// illustrate how to use fences for efficient resource usage.

	// Signal and increment the fence value.
	const UINT64 fenceVal = fenceValue;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceVal));
	fenceValue++;

	// Wait until the previous frame is finished.
	if (fence->GetCompletedValue() < fenceVal) {
		ThrowIfFailed(fence->SetEventOnCompletion(fenceVal, fenceEvent));
		WaitForSingleObject(fenceEvent, INFINITE);
	}

	frameIndex = swapChain->GetCurrentBackBufferIndex();
}

void InitDirect3D(HWND hwnd) {
	initTriangleAndInstanceData();
	
	if (GetClientRect(hwnd, &rc) == 0) {
		throw std::logic_error("GetClientRect failed");
	}
	DXInitAux::initDeviceAndFactory();
	DXInitAux::initViewPort();
	DXInitAux::initCommandQueue();
	DXInitAux::initSwapChain(hwnd);
	DXInitAux::initCBVRTVHeaps();
	DXInitAux::initCommandAllocatorAndList();

	DXInitAux::initDepthBuffer();
	DXInitAux::initVsConstBufferResourceAndView();

	DXInitAux::initRootSignature();
	DXInitAux::initPipelineState();
	DXInitAux::initVertexBuffer();

	DXInitAux::initInstanceBuffer();

	// DXInitAux::initWicFactory();

	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	fenceValue = 1;

	// Create an event handle to use for frame synchronization.
	fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	if (fenceEvent == nullptr) {
		ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	D3D12_RANGE constBufferDataRange = {0, 0};
	ThrowIfFailed(vsConstBuffer->Map(0, &constBufferDataRange, reinterpret_cast<void**>(&vsConstBufferPointer)));
	copyConstBufferToGpu();

	// Wait for the command list to execute; we are reusing the same command 
	// list in our main loop but for now, we just want to wait for setup to 
	// complete before continuing.
	WaitForPreviousFrame(hwnd);

	DXInitAux::initTextureView(hwnd);
}

void OnUpdate(HWND hwnd) {
	calcNewMatrix();
}

void OnRender(HWND hwnd) {
	// Record all the commands we need to render the scene into the command list.
	PopulateCommandList(hwnd);

	// Execute the command list.
	ID3D12CommandList* ppCommandLists[] = { commandList.Get() };
	commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	// Present the frame.
	ThrowIfFailed(swapChain->Present(1, 0));
	WaitForPreviousFrame(hwnd);
}

void OnDestroy(HWND hwnd) {
	// Wait for the GPU to be done with all resources.
	WaitForPreviousFrame(hwnd);
	
	CloseHandle(fenceEvent);
}