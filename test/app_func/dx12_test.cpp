
#include <drive/app.h>
#include <test.h>
#include <d3d12.h>
#include <dxgi.h>
#include <dxgi1_4.h>
#include <vector>


ID3D12Device* dev = 0;
ID3D12CommandQueue *cmd_qu = 0;
IDXGIFactory4* factory = 0;
DXGI_ADAPTER_DESC adapterDesc;
IDXGIAdapter* adapter;
IDXGIOutput* adapterOutput;
int videoCardMemory = 0;
std::vector<char> videoCardDesc(128);
IDXGISwapChain3* swapChain = 0;
D3D12_DESCRIPTOR_HEAP_DESC renderTargetViewHeapDesc = {};
D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle = {};
ID3D12DescriptorHeap* m_renderTargetViewHeap;
ID3D12Resource* m_backBufferRenderTarget[2];
unsigned int m_bufferIndex;
ID3D12CommandAllocator* m_commandAllocator;
ID3D12GraphicsCommandList* m_commandList;
ID3D12PipelineState* m_pipelineState;
ID3D12Fence* m_fence;
HANDLE m_fenceEvent;
unsigned long long m_fenceValue;


int
test_setup(struct drv_app_ctx *ctx)
{
        struct drv_app_data app_data = {};
        drv_app_data_get(ctx, &app_data);

        drv_app_gpu_device(DRV_APP_GPU_DEVICE_DX12, (void**)&dev);

        HRESULT res; 

        D3D12_COMMAND_QUEUE_DESC cmd_qu_desc = {};
        cmd_qu_desc.Type     = D3D12_COMMAND_LIST_TYPE_DIRECT;
	cmd_qu_desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	cmd_qu_desc.Flags    = D3D12_COMMAND_QUEUE_FLAG_NONE;
	cmd_qu_desc.NodeMask = 0;

        res = dev->CreateCommandQueue(
                &cmd_qu_desc,
                __uuidof(ID3D12CommandQueue),
                (void**)&cmd_qu);

        if(FAILED(res)) {
		return 0;
	}

        // Create a DirectX graphics interface factory.
	res = CreateDXGIFactory1(__uuidof(IDXGIFactory4), (void**)&factory);

	if(FAILED(res)) {
		return 0;
	}

        // Use the factory to create an adapter for the primary graphics interface (video card).
        res = factory->EnumAdapters(0, &adapter);
	if(FAILED(res)) {
		return 0;
	}

        // Enumerate the primary adapter output (monitor).
	res = adapter->EnumOutputs(0, &adapterOutput);
	if(FAILED(res)) {
		return 0;
	}
        
        UINT numModes;

        // Get the number of modes that fit the DXGI_FORMAT_R8G8B8A8_UNORM display format for the adapter output (monitor).
	res = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, NULL);
	if(FAILED(res)) {
		return 0;
	}

        // Create a list to hold all the possible display modes for this monitor/video card combination.
        std::vector<DXGI_MODE_DESC> displayModeList(numModes);
	if(displayModeList.empty()) {
		return 0;
	}

        // Now fill the display mode list structures.
	res = adapterOutput->GetDisplayModeList(DXGI_FORMAT_R8G8B8A8_UNORM, DXGI_ENUM_MODES_INTERLACED, &numModes, displayModeList.data());
	if(FAILED(res)) {
		return 0;
	}

        // Now go through all the display modes and find the one that matches the screen height and width.
	// When a match is found store the numerator and denominator of the refresh rate for that monitor.
        unsigned int numerator, denominator;

        for(auto &mode : displayModeList) {
                if(mode.Height < 100) {
                        continue;      
                }

                if(mode.Width < 100) {
                        continue;
                }

                numerator = mode.RefreshRate.Numerator;
		denominator = mode.RefreshRate.Denominator;

                break;
        }

        // Get the adapter (video card) description.
	res = adapter->GetDesc(&adapterDesc);
	if(FAILED(res)) {
		return 0;
	}

	// Store the dedicated video card memory in megabytes.
	videoCardMemory = (int)(adapterDesc.DedicatedVideoMemory / 1024 / 1024);

	// Convert the name of the video card to a character array and store it.
  /*      UINT length;
	errno_t err = wcstombs_s(&length, videoCardDesc.data(), 128, adapterDesc.Description, 128);
	if(err != 0) {
		return 0;
	}*/

        //rtv = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

        // Initialize the swap chain description.
        DXGI_SWAP_CHAIN_DESC swapChainDesc;
	ZeroMemory(&swapChainDesc, sizeof(swapChainDesc));

        IDXGISwapChain* swpChain;

        // Set the swap chain to use double buffering.
	swapChainDesc.BufferCount = 2;
        // Set the height and width of the back buffers in the swap chain.
	swapChainDesc.BufferDesc.Height = 100;
	swapChainDesc.BufferDesc.Width = 100;

        // Set a regular 32-bit surface for the back buffers.
	swapChainDesc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	// Set the usage of the back buffers to be render target outputs.
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;

	// Set the swap effect to discard the previous buffer contents after swapping.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;

	// Set the handle for the window to render to.
	swapChainDesc.OutputWindow = (HWND)app_data.hwnd;
        swapChainDesc.Windowed = true;

        // vsync
        swapChainDesc.BufferDesc.RefreshRate.Numerator = numerator;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = denominator;

        // Turn multisampling off.
	swapChainDesc.SampleDesc.Count = 1;
	swapChainDesc.SampleDesc.Quality = 0;
	
        // Set the scan line ordering and scaling to unspecified.
	swapChainDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	swapChainDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

	// Don't set the advanced flags.
	swapChainDesc.Flags = 0;

        // Finally create the swap chain using the swap chain description.	
	res = factory->CreateSwapChain(cmd_qu, &swapChainDesc, &swpChain);
	if(FAILED(res))
	{
		return 0;
	}

	// Next upgrade the IDXGISwapChain to a IDXGISwapChain3 interface and store it in a private member variable named m_swapChain.
	// This will allow us to use the newer functionality such as getting the current back buffer index.
	res = swpChain->QueryInterface(__uuidof(IDXGISwapChain3), (void**)&swapChain);
	if(FAILED(res))
	{
		return 0;
	}

	// Clear pointer to original swap chain interface since we are using version 3 instead (m_swapChain).
	swpChain = 0;

	// Release the factory now that the swap chain has been created.
	factory->Release();
	factory = 0;

        // Initialize the render target view heap description for the two back buffers.
	ZeroMemory(&renderTargetViewHeapDesc, sizeof(renderTargetViewHeapDesc));

	// Set the number of descriptors to two for our two back buffers.  Also set the heap tyupe to render target views.
	renderTargetViewHeapDesc.NumDescriptors = 2;
	renderTargetViewHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	renderTargetViewHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	// Create the render target view heap for the back buffers.
	res = dev->CreateDescriptorHeap(&renderTargetViewHeapDesc, __uuidof(ID3D12DescriptorHeap), (void**)&m_renderTargetViewHeap);
	if(FAILED(res)) {
		return 0;
	}

	// Get a handle to the starting memory location in the render target view heap to identify where the render target views will be located for the two back buffers.
	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();

	// Get the size of the memory location for the render target view descriptors.
        unsigned int renderTargetViewDescriptorSize;
	renderTargetViewDescriptorSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	// Get a pointer to the first back buffer from the swap chain.
	res = swapChain->GetBuffer(0, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[0]);
	if(FAILED(res)) {
		return 0;
	}

	// Create a render target view for the first back buffer.
	dev->CreateRenderTargetView(m_backBufferRenderTarget[0], NULL, renderTargetViewHandle);

	// Increment the view handle to the next descriptor location in the render target view heap.
	renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;

	// Get a pointer to the second back buffer from the swap chain.
	res = swapChain->GetBuffer(1, __uuidof(ID3D12Resource), (void**)&m_backBufferRenderTarget[1]);
	if(FAILED(res)) {
		return 0;
	}

	// Create a render target view for the second back buffer.
	dev->CreateRenderTargetView(m_backBufferRenderTarget[1], NULL, renderTargetViewHandle);

        // Finally get the initial index to which buffer is the current back buffer.
	m_bufferIndex = swapChain->GetCurrentBackBufferIndex();

        // Create a command allocator.
	res = dev->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, __uuidof(ID3D12CommandAllocator), (void**)&m_commandAllocator);
	if(FAILED(res))
	{
		return 0;
	}

        // Create a basic command list.
	res = dev->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_commandAllocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void**)&m_commandList);
	if(FAILED(res))
	{
		return 0;
	}

	// Initially we need to close the command list during initialization as it is created in a recording state.
	res = m_commandList->Close();
	if(FAILED(res))
	{
		return 0;
	}

        // Create a fence for GPU synchronization.
	res = dev->CreateFence(0, D3D12_FENCE_FLAG_NONE, __uuidof(ID3D12Fence), (void**)&m_fence);
	if(FAILED(res))
	{
		return false;
	}
	
	// Create an event object for the fence.
	m_fenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
	if(m_fenceEvent == NULL)
	{
		return false;
	}

	// Initialize the starting fence value. 
	m_fenceValue = 1;
	
	return true;
}


void
test_tick()
{
        HRESULT result;
	D3D12_RESOURCE_BARRIER barrier;
	D3D12_CPU_DESCRIPTOR_HANDLE renderTargetViewHandle;
	unsigned int renderTargetViewDescriptorSize;
	float color[4];
	ID3D12CommandList* ppCommandLists[1];
	unsigned long long fenceToWaitFor;

        // Reset (re-use) the memory associated command allocator.
	result = m_commandAllocator->Reset();
	if(FAILED(result))
	{
		return;
	}

	// Reset the command list, use empty pipeline state for now since there are no shaders and we are just clearing the screen.
	result = m_commandList->Reset(m_commandAllocator, m_pipelineState);
	if(FAILED(result))
	{
		return;
	}

	// Record commands in the command list now.
	// Start by setting the resource barrier.
	barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier.Transition.pResource = m_backBufferRenderTarget[m_bufferIndex];
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	m_commandList->ResourceBarrier(1, &barrier);

        // Get the render target view handle for the current back buffer.
	renderTargetViewHandle = m_renderTargetViewHeap->GetCPUDescriptorHandleForHeapStart();
	renderTargetViewDescriptorSize = dev->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	if(m_bufferIndex == 1)
	{
		renderTargetViewHandle.ptr += renderTargetViewDescriptorSize;
	}

	// Set the back buffer as the render target.
	m_commandList->OMSetRenderTargets(1, &renderTargetViewHandle, FALSE, NULL);

        // Then set the color to clear the window to.
	color[0] = 1.0;
	color[1] = 0.2;
	color[2] = 0.2;
	color[3] = 1.0;
	m_commandList->ClearRenderTargetView(renderTargetViewHandle, color, 0, NULL);

        // Indicate that the back buffer will now be used to present.
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	m_commandList->ResourceBarrier(1, &barrier);

        // Close the list of commands.
	result = m_commandList->Close();
	if(FAILED(result))
	{
		return;
	}

	// Load the command list array (only one command list for now).
	ppCommandLists[0] = m_commandList;

	// Execute the list of commands.
	cmd_qu->ExecuteCommandLists(1, ppCommandLists);

        // Finally present the back buffer to the screen since rendering is complete.

        // Lock to screen refresh rate.
	result = swapChain->Present(1, 0);
	if (FAILED(result))
	{
		return;
	}

        // Signal and increment the fence value.
	fenceToWaitFor = m_fenceValue;
	result = cmd_qu->Signal(m_fence, fenceToWaitFor);
	if(FAILED(result))
	{
		return;
	}
	m_fenceValue++;

	// Wait until the GPU is done rendering.
	if(m_fence->GetCompletedValue() < fenceToWaitFor)
	{
		result = m_fence->SetEventOnCompletion(fenceToWaitFor, m_fenceEvent);
		if(FAILED(result))
		{
			return;
		}
		WaitForSingleObject(m_fenceEvent, INFINITE);
	}

        // Alternate the back buffer index back and forth between 0 and 1 each frame.
	m_bufferIndex == 0 ? m_bufferIndex = 1 : m_bufferIndex = 0;


}


void
test_shutdown()
{
	int error;


	// Before shutting down set to windowed mode or when you release the swap chain it will throw an exception.
	if(swapChain) {
		swapChain->SetFullscreenState(false, NULL);
	}
		
	// Close the object handle to the fence event.
	error = CloseHandle(m_fenceEvent);
	if(error == 0)
	{
	}
		
	// Release the fence.
	if(m_fence) {
		m_fence->Release();
		m_fence = 0;
	}

	// Release the empty pipe line state.
	if(m_pipelineState)
	{
		m_pipelineState->Release();
		m_pipelineState = 0;
	}

	// Release the command list.
	if(m_commandList)
	{
		m_commandList->Release();
		m_commandList = 0;
	}

	// Release the command allocator.
	if(m_commandAllocator)
	{
		m_commandAllocator->Release();
		m_commandAllocator = 0;
	}

	// Release the back buffer render target views.
	if(m_backBufferRenderTarget[0])
	{
		m_backBufferRenderTarget[0]->Release();
		m_backBufferRenderTarget[0] = 0;
	}
	if(m_backBufferRenderTarget[1])
	{
		m_backBufferRenderTarget[1]->Release();
		m_backBufferRenderTarget[1] = 0;
	}
	
	// Release the render target view heap.
	if(m_renderTargetViewHeap)
	{
		m_renderTargetViewHeap->Release();
		m_renderTargetViewHeap = 0;
	}

	// Release the swap chain.
	if(swapChain)
	{
		swapChain->Release();
		swapChain = 0;
	}

	// Release the command queue.
	if(cmd_qu)
	{
		cmd_qu->Release();
		cmd_qu = 0;
	}
	
	// Release the device.
	if(dev)
	{
		dev->Release();
		dev = 0;
	}
}