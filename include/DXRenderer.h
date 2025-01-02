#pragma once
#include "stdafx.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

class DXRenderer
{
public:
	DXRenderer(UINT width, UINT height, std::wstring name);
	~DXRenderer();

	void OnInit();
	void OnUpdate();
	void OnRender();
	void OnDestroy();

	void OnKeyDown(UINT8);
	void OnKeyUp(UINT8);

	UINT GetWidth() const { return mWidth; };
	UINT GetHeight() const { return mHeight; }
	const WCHAR* GetTitle() const { return mTitle.c_str(); }

	void ParseCommandLineArgs(_In_reads_(argc) WCHAR* argv[], int argc);

private:
	// Accessors
	std::wstring GetAssetFullPath(LPCWSTR assetName);

	void GetHardwareAdapter(
		_In_ IDXGIFactory1* pFactory,
		_Outptr_result_maybenull_ IDXGIAdapter1** ppAdapter,
		bool requestHighPerformanceAdapter = false);

	void SetCustomWindowText(LPCWSTR text);

	// Functions
	void LoadPipeline();
	void LoadAssets();
	std::vector<UINT8> GenerateTextureData();
	void PopulateCommandList();
	void WaitForPreviousFrame();

	// Display
	UINT mWidth;
	UINT mHeight;
	float mAspectRatio;

	bool mUseWarpDevice;

	static const UINT FrameCount = 2;
	static const UINT TextureWidth = 256;
	static const UINT TextureHeight = 256;
	static const UINT TexturePixelSize = 4;

	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT2 uv;
	};

	// Pipeline Objects
	CD3DX12_VIEWPORT mViewport;
	CD3DX12_RECT mScissorRect;

	ComPtr<IDXGISwapChain3> mSwapChain;
	ComPtr<ID3D12Device> mDevice;
	ComPtr<ID3D12Resource> mRenderTargets[FrameCount];
	ComPtr<ID3D12CommandAllocator> mCommandAllocator;
	ComPtr<ID3D12CommandQueue> mCommandQueue;
	ComPtr<ID3D12RootSignature> mRootSignature;
	ComPtr<ID3D12DescriptorHeap> mRtvHeap;
	ComPtr<ID3D12DescriptorHeap> mSrvHeap;
	ComPtr<ID3D12PipelineState> mPipelineState;
	ComPtr<ID3D12GraphicsCommandList> mCommandList;
	UINT mRtvDescrptiorSize;

	// App Resources
	ComPtr<ID3D12Resource> mVertexBuffer;
	D3D12_VERTEX_BUFFER_VIEW mVertexBufferView;
	ComPtr<ID3D12Resource> mTexture;

	// Synchronization Objects
	UINT mFrameIndex;
	HANDLE mFenceEvent;
	ComPtr<ID3D12Fence> mFence;
	UINT64 mFenceValue;

	// Misc.
	std::wstring mAssetsPath;
	std::wstring mTitle;

};

