#include "DXRenderer.h"
#include "WinCtx.h"
#include "DXHelper.h"


DXRenderer::DXRenderer(UINT width, UINT height, std::wstring name)
	:
	mWidth(width), 
	mHeight(height), 
	mTitle(name), 
	mUseWarpDevice(false),
	mFrameIndex(0), 
	mViewport(0.0f, 0.0f, static_cast<FLOAT>(width), static_cast<float>(height)),
	mScissorRect(0, 0, static_cast<LONG>(width), static_cast<LONG>(height)),
	mRtvDescrptiorSize(0)
{
	//WCHAR assetsPath[512];
	//TODO: Setup Helper class

	mAspectRatio = static_cast<float>(width) / static_cast<float>(height);
}

DXRenderer::~DXRenderer()
{
}

void DXRenderer::OnInit()
{
	LoadPipeline();
	LoadAssets();
}

void DXRenderer::LoadPipeline()
{
	UINT dxgiFactoryFlags = 0;

#if defined(_DEBUG)
	// Enable Debug Layer
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();

			dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
		}
	}
#endif

	ComPtr<IDXGIFactory4> factory;
	ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

	if (mUseWarpDevice)
	{
		ComPtr<IDXGIAdapter> warpAdapter;
		ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		ThrowIfFailed(D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
	}
	else
	{
		ComPtr<IDXGIAdapter1> hardwareAdapter;
		GetHardwareAdapter(factory.Get(), &hardwareAdapter);

		ThrowIfFailed(D3D12CreateDevice(hardwareAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice)));
	}

	// Command Queue Description and Creation
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ThrowIfFailed(mDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&mCommandQueue)));

	// Swap Chain Description and Creation
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = FrameCount;
	swapChainDesc.Width = mWidth;
	swapChainDesc.Height = mHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.SampleDesc.Count = 1;

	ComPtr<IDXGISwapChain1> swapChain;
	ThrowIfFailed(factory->CreateSwapChainForHwnd(mCommandQueue.Get(), WinCtx::GetHwnd(), &swapChainDesc, nullptr, nullptr, &swapChain));

	// Block fullscreen transition
	ThrowIfFailed(factory->MakeWindowAssociation(WinCtx::GetHwnd(), DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain.As(&mSwapChain));
	mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();

	// Descriptor Heap Creation
	{
		D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
		rtvHeapDesc.NumDescriptors = FrameCount;
		rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(mDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

		mRtvDescrptiorSize = mDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Frame Resources Creation
	{
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());

		// One RTV for every frame
		for (UINT n = 0; n < FrameCount; n++)
		{
			ThrowIfFailed(mSwapChain->GetBuffer(n, IID_PPV_ARGS(&mRenderTargets[n])));
			mDevice->CreateRenderTargetView(mRenderTargets[n].Get(), nullptr, rtvHandle);
			rtvHandle.Offset(1, mRtvDescrptiorSize);
		}
	}

	ThrowIfFailed(mDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator)));
}

void DXRenderer::LoadAssets()
{
	// Root Signature Creation
	{
		CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
		rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error));
		ThrowIfFailed(mDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&mRootSignature)));
	}

	// Pipeline State Creation
	{
		// Shader Compilation 
		ComPtr<ID3DBlob> vertexShader;
		ComPtr<ID3DBlob> pixelShader;

#if defined(_DEBUG)
		UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compileFlags = 0;
#endif

		ThrowIfFailed(D3DCompileFromFile(L"D:\\Workspace\\DXRT\\Assets\\Shaders\\shaders.hlsl", nullptr, nullptr, "VSMain", "vs_5_0", compileFlags, 0, &vertexShader, nullptr));
		ThrowIfFailed(D3DCompileFromFile(L"D:\\Workspace\\DXRT\\Assets\\Shaders\\shaders.hlsl", nullptr, nullptr, "PSMain", "ps_5_0", compileFlags, 0, &pixelShader, nullptr));

		// Vertex Input Layout
		D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
		{
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
			{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
		};

		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
		psoDesc.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
		psoDesc.pRootSignature = mRootSignature.Get();
		psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
		psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
		psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
		psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
		psoDesc.DepthStencilState.DepthEnable = FALSE;
		psoDesc.DepthStencilState.StencilEnable = FALSE;
		psoDesc.SampleMask = UINT_MAX;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.SampleDesc.Count = 1;
		ThrowIfFailed(mDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPipelineState)));
	}


	// Command List Creation and close it for now
	ThrowIfFailed(mDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, mCommandAllocator.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
	ThrowIfFailed(mCommandList->Close());

	// Vertex Buffer Creation
	{
		Vertex triangleVertices[] =
		{
			{ { 0.0f, 0.25f * mAspectRatio, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
			{ { 0.25f, -0.25f * mAspectRatio, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
			{ { -0.25f, -0.25f * mAspectRatio, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
		};

		const UINT vertexBufferSize = sizeof(triangleVertices);

		ThrowIfFailed(mDevice->CreateCommittedResource(
			&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS(&mVertexBuffer)
		));

		// Copy triangle data to vertex buffer
		UINT8* pVertexDataBegin;
		CD3DX12_RANGE readRange(0, 0);
		ThrowIfFailed(mVertexBuffer->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin)));
		memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
		mVertexBuffer->Unmap(0, nullptr);

		// Init v buffer view
		mVertexBufferView.BufferLocation = mVertexBuffer->GetGPUVirtualAddress();
		mVertexBufferView.StrideInBytes = sizeof(Vertex);
		mVertexBufferView.SizeInBytes = vertexBufferSize;
	}


	// Synchronization Objects Creation
	{
		ThrowIfFailed(mDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
		mFenceValue = 1;

		mFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		if (mFenceEvent == nullptr)
			ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));

		WaitForPreviousFrame();
	}
}

void DXRenderer::OnUpdate()
{
}

void DXRenderer::OnRender()
{
	PopulateCommandList();

	ID3D12CommandList* ppCommandLists[] = { mCommandList.Get() };
	mCommandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

	ThrowIfFailed(mSwapChain->Present(1, 0));
	
	WaitForPreviousFrame();
}

void DXRenderer::OnDestroy()
{
	WaitForPreviousFrame();

	CloseHandle(mFenceEvent);
}


void DXRenderer::PopulateCommandList()
{
	ThrowIfFailed(mCommandAllocator->Reset());
	
	ThrowIfFailed(mCommandList->Reset(mCommandAllocator.Get(), mPipelineState.Get()));

	mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
	mCommandList->RSSetViewports(1, &mViewport);
	mCommandList->RSSetScissorRects(1, &mScissorRect);


	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	mCommandList->ResourceBarrier(1, &barrier);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mFrameIndex, mRtvDescrptiorSize);

	// Record Commands
	const float clearColor[] = { 0.3f, 0.3f, 0.8f, 1.0f };
	mCommandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
	mCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	mCommandList->IASetVertexBuffers(0, 1, &mVertexBufferView);
	mCommandList->DrawInstanced(3, 1, 0, 0);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(mRenderTargets[mFrameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	mCommandList->ResourceBarrier(1, &barrier);

	ThrowIfFailed(mCommandList->Close());
}

void DXRenderer::WaitForPreviousFrame()
{
	const UINT64 fenceVal = mFenceValue;
	ThrowIfFailed(mCommandQueue->Signal(mFence.Get(), fenceVal));
	mFenceValue++;

	if (mFence->GetCompletedValue() < fenceVal)
	{
		ThrowIfFailed(mFence->SetEventOnCompletion(fenceVal, mFenceEvent));
		WaitForSingleObject(mFenceEvent, INFINITE);
	}

	mFrameIndex = mSwapChain->GetCurrentBackBufferIndex();
}

void DXRenderer::OnKeyDown(UINT8)
{
}

void DXRenderer::OnKeyUp(UINT8)
{
}

std::wstring DXRenderer::GetAssetFullPath(LPCWSTR assetName)
{
	return mAssetsPath + assetName;
}

_Use_decl_annotations_
void DXRenderer::GetHardwareAdapter(IDXGIFactory1* pFactory, IDXGIAdapter1** ppAdapter, bool requestHighPerformanceAdapter)
{
	*ppAdapter = nullptr;

	ComPtr<IDXGIAdapter1> adapter;
	ComPtr<IDXGIFactory6> factory6;
	DXGI_GPU_PREFERENCE gpuPref = requestHighPerformanceAdapter ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED;

	if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
	{
		for (UINT adapterIndex = 0;
			SUCCEEDED(factory6->EnumAdapterByGpuPreference(adapterIndex, gpuPref, IID_PPV_ARGS(&adapter)));
			adapterIndex++)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			// Skip basic render driver adapter
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;

			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
				break;
		}
	}

	if (adapter.Get() == nullptr)
	{
		for (UINT adapterIndex = 0;
			SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter));
			adapterIndex++)
		{
			DXGI_ADAPTER_DESC1 desc;
			adapter->GetDesc1(&desc);

			// Skip basic render driver adapter
			if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
				continue;

			if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)))
				break;
		}
	}

	*ppAdapter = adapter.Detach();
}

void DXRenderer::SetCustomWindowText(LPCWSTR text)
{
	std::wstring windowText = mTitle + L": " + text;
	SetWindowText(WinCtx::GetHwnd(), windowText.c_str());
}

_Use_decl_annotations_
void DXRenderer::ParseCommandLineArgs(WCHAR* argv[], int argc)
{
	for (int i = 1; i < argc; i++)
	{
		if (_wcsnicmp(argv[i], L"-warp", wcslen(argv[i])) == 0 ||
			_wcsnicmp(argv[i], L"/warp", wcslen(argv[i])) == 0)
		{
			mUseWarpDevice = true;
			mTitle = mTitle + L" (WARP)";
		}
	}
}