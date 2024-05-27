#include <vma/vk_mem_alloc.h>

#include <assert.h>

#include <Resources.hpp>

namespace EVK {

void VertexBufferObject::Fill(void *vertices, const VkDeviceSize &size, const VkDeviceSize &offset){
	// destroying old vertex buffer if it exists
	if(contents){
		if(contents->size == size){
			devices->FillExistingDeviceLocalBuffer(contents->bufferHandle, vertices, size);
			contents->offset = offset;
			return;
		} else {
			CleanUpContents();
		}
	}
	
	contents = Contents();
	devices->CreateAndFillDeviceLocalBuffer(contents->bufferHandle, contents->allocation, vertices, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	contents->offset = offset;
	contents->size = size;
}
void VertexBufferObject::CleanUpContents(){
	if(!contents){
		return;
	}
	vmaDestroyBuffer(devices->GetAllocator(), contents->bufferHandle, contents->allocation);
	contents.reset();
}

void IndexBufferObject::Fill(uint32_t *indices, size_t indexCount, const VkDeviceSize &offset){
	// destroying old vertex buffer if it exists
	if(contents){
		if(contents->indexCount == indexCount){
			devices->FillExistingDeviceLocalBuffer(contents->bufferHandle, indices, VkDeviceSize(indexCount * sizeof(int32_t)));
			contents->offset = offset;
			return;
		} else {
			CleanUpContents();
		}
	}
	
	contents = Contents();
	devices->CreateAndFillDeviceLocalBuffer(contents->bufferHandle, contents->allocation, indices, VkDeviceSize(indexCount * sizeof(int32_t)), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	contents->offset = offset;
	contents->indexCount = uint32_t(indexCount);
}
void IndexBufferObject::CleanUpContents(){
	if(!contents){
		return;
	}
	vmaDestroyBuffer(devices->GetAllocator(), contents->bufferHandle, contents->allocation);
	contents.reset();
}

UniformBufferObject::UniformBufferObject(std::shared_ptr<Devices> _devices, VkDeviceSize _size, std::optional<Dynamic> _dynamic)
: devices(std::move(_devices)), size(std::move(_size)), dynamic(std::move(_dynamic)) {
	for(size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i){
		// creating buffer
		devices->CreateBuffer(size,
							  VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
							  VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
							  buffersFlying[i],
							  allocationsFlying[i],
							  &(allocationInfosFlying[i]));
	}
}

StorageBufferObject::StorageBufferObject(std::shared_ptr<Devices> _devices,
										 VkDeviceSize _size,
										 VkBufferUsageFlags usages,
										 VkMemoryPropertyFlags memoryProperties)
: devices(std::move(_devices)), size(std::move(_size)) {
	for(size_t i=0; i<MAX_FRAMES_IN_FLIGHT; ++i){
		// creating buffer
		devices->CreateBuffer(size,
							  VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | usages,
							  memoryProperties,
							  buffersFlying[i],
							  allocationsFlying[i],
							  &(allocationInfosFlying[i]));
	}
}

bool StorageBufferObject::Fill(const std::vector<std::byte> &data){
	if(data.size() != size){
		std::cout << "Data does not match storage buffer size.\n";
		return false;
	}
	for(int i=0; i<MAX_FRAMES_IN_FLIGHT; ++i){
		devices->FillExistingDeviceLocalBuffer(buffersFlying[i], (void *)(data.data()), size);
	}
	return true;
}

TextureImage::TextureImage(std::shared_ptr<Devices> _devices, PNGImageBlueprint fromPNG)
: devices(std::move(_devices)){
	SDL_Surface *const surface = IMG_Load(fromPNG.imageFilename.c_str());
	
	if(!surface) throw std::runtime_error("failed to load texture image!");
	
	ConstructFromData(DataImageBlueprint{
		.data = (uint8_t *)surface->pixels,
		.width = uint32_t(surface->w),
		.height = uint32_t(surface->h),
		.pitch = uint32_t(surface->pitch),
		.format = VK_FORMAT_R8G8B8A8_SRGB,
		.mip = true
	});
	//SDLPixelFormatToVulkanFormat((SDL_PixelFormatEnum)devices->surface->format->format); // 24 bit-depth images don't seem to work
	
	SDL_FreeSurface(surface);
}
TextureImage::TextureImage(std::shared_ptr<Devices> _devices, DataImageBlueprint fromRaw)
: devices(std::move(_devices)) {
	ConstructFromData(std::move(fromRaw));
}
TextureImage::TextureImage(std::shared_ptr<Devices> _devices, Data3DImageBlueprint fromRaw3D)
: devices(std::move(_devices)) {
	const VkDeviceSize imageSize = fromRaw3D.height * fromRaw3D.pitch * fromRaw3D.depth;
	
	// creating a staging buffer, copying in the pixels and then freeing the SDL devices->surface and the staging buffer memory allocation
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;
	devices->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingAllocation, &stagingAllocInfo);
	memcpy(stagingAllocInfo.pMappedData, fromRaw3D.data, (size_t)imageSize);
	vmaFlushAllocation(devices->GetAllocator(), stagingAllocation, 0, VK_WHOLE_SIZE);
	
	// creating the image
	const VkImageCreateInfo imageCI = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_3D,
		.extent.width = fromRaw3D.width,
		.extent.height = fromRaw3D.height,
		.extent.depth = fromRaw3D.depth,
		.mipLevels = 1,
		.arrayLayers = 1,
		.format = fromRaw3D.format,
		.tiling = VK_IMAGE_TILING_OPTIMAL, // VK_IMAGE_TILING_LINEAR for row-major order if we want to access texels in the memory of the image
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	devices->CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, allocation);
	
	const VkImageSubresourceRange subresourceRange = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 1
	};
	devices->TransitionImageLayout(image, fromRaw3D.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
	devices->CopyBufferToImage(stagingBuffer, image, fromRaw3D.width, fromRaw3D.height, fromRaw3D.depth);
	
	vmaDestroyBuffer(devices->GetAllocator(), stagingBuffer, stagingAllocation);
	
	// Creating an image view for the texture image
	view = devices->CreateImageView({
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_3D,
		.format = fromRaw3D.format,
		.components = {},
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1}
	});
	
	extent = imageCI.extent;
	format = imageCI.format;
}
TextureImage::TextureImage(std::shared_ptr<Devices> _devices, CubemapPNGImageBlueprint fromPNGCubemaps)
: devices(std::move(_devices)) {
	// loading image onto an sdl devices->surface
	std::array<SDL_Surface *, 6> surfaces;
	surfaces[0] = IMG_Load(fromPNGCubemaps.imageFilenames[0].c_str());
	if(!surfaces[0]) throw std::runtime_error("failed to load texture image!");
	const uint32_t faceWidth = surfaces[0]->w;
	//const uint32_t width = 6*faceWidth;
	const uint32_t height = surfaces[0]->h;
	const size_t faceSize = height * surfaces[0]->pitch;
	const VkDeviceSize imageSize = 6 * faceSize;
	const VkFormat imageFormat = VK_FORMAT_R8G8B8A8_SRGB;//SDLPixelFormatToVulkanFormat((SDL_PixelFormatEnum)devices->surface->format->format); // 24 bit-depth images don't seem to work
	
	for(int i=1; i<6; i++){
		surfaces[i] = IMG_Load(fromPNGCubemaps.imageFilenames[i].c_str());
		if(!surfaces[i]) throw std::runtime_error("failed to load texture image!");
		assert(surfaces[i]->w == faceWidth);
		assert(surfaces[i]->h == height);
	}
	
	// creating a staging buffer, copying in the pixels and then freeing the SDL devices->surface and the staging buffer memory allocation
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;
	devices->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingAllocation, &stagingAllocInfo);
	for(int i=0; i<6; i++){
		memcpy((uint8_t *)stagingAllocInfo.pMappedData + faceSize*i, surfaces[i]->pixels, faceSize);
		SDL_FreeSurface(surfaces[i]);
	}
	vmaFlushAllocation(devices->GetAllocator(), stagingAllocation, 0, VK_WHOLE_SIZE);
	
	// creating the image
	const VkImageCreateInfo imageCI = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = faceWidth,
		.extent.height = height,
		.extent.depth = 1,
		.mipLevels = 1,
		.arrayLayers = 6,
		.format = imageFormat,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT
	};
	devices->CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, allocation);
	
	VkCommandBuffer commandBuffer = devices->BeginSingleTimeCommands();
	
	VkBufferImageCopy regions[6];
	for(int face=0; face<6; face++){
		regions[face].bufferOffset = faceSize * face;
		regions[face].bufferRowLength = 0;
		regions[face].bufferImageHeight = 0;
		regions[face].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		regions[face].imageSubresource.mipLevel = 0;
		regions[face].imageSubresource.baseArrayLayer = face;
		regions[face].imageSubresource.layerCount = 1;
		regions[face].imageOffset = {0, 0, 0};
		regions[face].imageExtent = {faceWidth, height, 1};
	}
	
	const VkImageSubresourceRange subresourceRange = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = 1,
		.baseArrayLayer = 0,
		.layerCount = 6
	};
	devices->TransitionImageLayout(image, imageFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
	
	vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 6, regions);
	
	devices->TransitionImageLayout(image, imageFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, subresourceRange);
	
	devices->EndSingleTimeCommands(commandBuffer);
	
	vmaDestroyBuffer(devices->GetAllocator(), stagingBuffer, stagingAllocation);
	
	// Creating an image view for the texture image
	view = devices->CreateImageView({
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_CUBE,
		.format = imageFormat,
		.components = {},
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 6}
	});
	
	extent = imageCI.extent;
	format = imageCI.format;
}
TextureImage::TextureImage(std::shared_ptr<Devices> _devices, ManualImageBlueprint fromBlueprint)
: devices(std::move(_devices)) {
	ConstructManual(std::move(fromBlueprint));
}


void TextureImage::ConstructFromData(DataImageBlueprint _blueprint){
	const VkDeviceSize imageSize = _blueprint.height * _blueprint.pitch;
	
	// calculated number of mipmap levels
	mipLevels = _blueprint.mip ? uint32_t(floor(log2(double(_blueprint.width > _blueprint.height ? _blueprint.width : _blueprint.height)))) + 1 : 1;
	
	// creating a staging buffer, copying in the pixels and then freeing the SDL devices->surface and the staging buffer memory allocation
	VkBuffer stagingBuffer;
	VmaAllocation stagingAllocation;
	VmaAllocationInfo stagingAllocInfo;
	devices->CreateBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingAllocation, &stagingAllocInfo);
	memcpy(stagingAllocInfo.pMappedData, _blueprint.data, (size_t)imageSize);
	vmaFlushAllocation(devices->GetAllocator(), stagingAllocation, 0, VK_WHOLE_SIZE);
	
	// creating the image
	const VkImageCreateInfo imageCI = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.imageType = VK_IMAGE_TYPE_2D,
		.extent.width = _blueprint.width,
		.extent.height = _blueprint.height,
		.extent.depth = 1,
		.mipLevels = mipLevels,
		.arrayLayers = 1,
		.format = _blueprint.format,
		.tiling = VK_IMAGE_TILING_OPTIMAL, // VK_IMAGE_TILING_LINEAR for row-major order if we want to access texels in the memory of the image
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
		.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE
	};
	devices->CreateImage(imageCI, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, allocation);
	
	const VkImageSubresourceRange subresourceRange = {
		.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
		.baseMipLevel = 0,
		.levelCount = mipLevels,
		.baseArrayLayer = 0,
		.layerCount = 1
	};
	devices->TransitionImageLayout(image, _blueprint.format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, subresourceRange);
	devices->CopyBufferToImage(stagingBuffer, image, _blueprint.width, _blueprint.height);
	devices->GenerateMipmaps(image, _blueprint.format, _blueprint.width, _blueprint.height, mipLevels);
	
	vmaDestroyBuffer(devices->GetAllocator(), stagingBuffer, stagingAllocation);
	
	// Creating an image view for the texture image
	view = devices->CreateImageView({
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = _blueprint.format,
		.components = {},
		.subresourceRange = {VK_IMAGE_ASPECT_COLOR_BIT, 0, mipLevels, 0, 1}
	});
	
	extent = imageCI.extent;
	format = imageCI.format;
}
void TextureImage::ConstructManual(ManualImageBlueprint _blueprint){
	
	devices->CreateImage(_blueprint.imageCI, _blueprint.properties, image, allocation);
	
	view = devices->CreateImageView({
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = nullptr,
		.flags = 0,
		.image = image,
		.viewType = _blueprint.imageViewType,
		.format = _blueprint.imageCI.format,
		.components = {},
		.subresourceRange = {_blueprint.aspectFlags, 0, _blueprint.imageCI.mipLevels, 0, _blueprint.imageCI.arrayLayers}
	});
	
	extent = _blueprint.imageCI.extent;
	format = _blueprint.imageCI.format;
}

TextureSampler::TextureSampler(std::shared_ptr<Devices> _devices,
							   const VkSamplerCreateInfo &samplerCI)
: devices(_devices) {
	
	if(vkCreateSampler(devices->GetLogicalDevice(), &samplerCI, nullptr, &handle) != VK_SUCCESS){
		throw std::runtime_error("Failed to create texture sampler!");
	}
}

BufferedRenderPass::BufferedRenderPass(std::shared_ptr<Devices> _devices,
									   const VkRenderPassCreateInfo *const pRenderPassCI)
: devices(std::move(_devices)) {
	
//	size = std::optional<vec<2, uint32_t>>();
	
	if(vkCreateRenderPass(devices->GetLogicalDevice(), pRenderPassCI, nullptr, &renderPass) != VK_SUCCESS){
		throw std::runtime_error("Failed to create render pass!");
	}
	
//	targetTextureImageIndices = blueprint.targetTextureImageIndices;
	
//	const std::vector<int>::iterator it = std::find(swapChainSizeMatchedBRPs.begin(), swapChainSizeMatchedBRPs.end(), index);
//	if(blueprint.resizeWithSwapChain != (it != swapChainSizeMatchedBRPs.end())){
//		if(blueprint.resizeWithSwapChain){
//			swapChainSizeMatchedBRPs.push_back(index);
//		} else {
//			swapChainSizeMatchedBRPs.erase(it);
//		}
//	}
}

bool BufferedRenderPass::SetImages(std::vector<std::shared_ptr<TextureImage>> images){
	if(images.size() < 1){
		std::cout << "Empty image array passed.\n";
		return false;
	}
	
	if(images[0]->Extent().depth != 1){
		std::cout << "Buffered render pass targets images must be 2D.\n";
		return false;
	}
	
	const VkExtent3D newSize = images[0]->Extent();
	
	VkImageView attachments[images.size()];
	for(int i=0; i<images.size(); ++i){
		if(images[i]->Extent().width != newSize.width ||
		   images[i]->Extent().height != newSize.height ||
		   images[i]->Extent().depth != newSize.depth){
			std::cout << "All images must be the same size.\n";
			return false;
		}
		attachments[i] = images[i]->View();
	}
	
	CleanUpTargets();
	
	targets = Targets();
	targets->images = std::move(images);
	
	const VkFramebufferCreateInfo frameBufferCI = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = renderPass,
		.attachmentCount = uint32_t(targets->images.size()),
		.pAttachments = attachments,
		.width = newSize.width,
		.height = newSize.height,
		.layers = 1
	};
	for(VkFramebuffer &fb : targets->frameBuffersFlying){
		if(vkCreateFramebuffer(devices->GetLogicalDevice(), &frameBufferCI, nullptr, &fb) != VK_SUCCESS){
			throw std::runtime_error("Failed to create framebuffer!");
		}
	}
	
	return true;
}

} // namespace EVK
