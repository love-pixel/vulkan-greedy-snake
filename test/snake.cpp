#include "./../inc/graphic/vulkan/vulkan.hpp"
#include "./../inc/dev/window_group/window_group.hpp"
#include "./../inc/dev/window_group/platform_support.hpp"
#include "./../inc/core/memory.hpp"
#include "./../inc/core/vec2.hpp"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "./../external/glm/glm.hpp"
#include "./../external/glm/gtc/matrix_transform.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "./../external/stb/stb_image.h"

#include <iostream>
#include <random>
#include <deque>
#include <chrono>

using namespace cw;

// glslangValidator -V snake.vert -o snake.vert.spv

constexpr bool vsync = false;

enum class difficulty_t {
	e_easy, e_normal, e_hard, e_null
};

struct snake_game_ci_t {
	core::extent2_t<core::ull_t> extent = { 30, 20 };
	core::ull_t window_rate = 20;
	core::ull_t win_score = 30;
	difficulty_t difficulty = difficulty_t::e_easy;
	bool console_game = true;
	decltype(auto) set_extent(core::extent2_t<core::ull_t> extent) { this->extent = extent; return *this; }
	decltype(auto) set_window_rate(core::ull_t window_rate) { this->window_rate = window_rate; return *this; }
	decltype(auto) set_win_score(core::ull_t win_score) { this->win_score = win_score; return *this; }
	decltype(auto) set_difficulty(difficulty_t difficulty) { this->difficulty = difficulty; return *this; }
	decltype(auto) set_console_game(bool console_game) { this->console_game = console_game; return *this; }
};

class snake_game_t {
private:
	enum class cell_e {
		e_empty, e_wall, e_food, e_head, e_body, e_tail, e_null
	};
	enum class direction_e {
		e_left, e_right, e_up, e_down, e_null
	};
	enum class game_state_e {
		e_win, e_failed, e_continue, e_pause, e_null
	};
	static decltype(auto) to_time(difficulty_t difficulty) {
		auto time = std::chrono::milliseconds(135);
		switch (difficulty) {
		case difficulty_t::e_easy: return time - std::chrono::milliseconds(0);
		case difficulty_t::e_normal: return time - std::chrono::milliseconds(20);
		case difficulty_t::e_hard: return time - std::chrono::milliseconds(40);
		}
		return time;
	}
	static decltype(auto) to_char(cell_e cell) {
		switch (cell) {
		case cell_e::e_empty:return ' ';
		case cell_e::e_wall:return '#';
		case cell_e::e_food:return '$';
		case cell_e::e_head:return '@';
		case cell_e::e_body:return '*';
		case cell_e::e_tail:return 'O';
		case cell_e::e_null:return ' ';
		}
		return 'X';
	}
	static decltype(auto) to_string(direction_e direction) {
		switch (direction) {
		case direction_e::e_left: return "left"s;
		case direction_e::e_right: return "right"s;
		case direction_e::e_up: return "up"s;
		case direction_e::e_down: return "down"s;
		}
		return "null"s;
	}
	static decltype(auto) to_string(game_state_e state) {
		switch (state) {
		case game_state_e::e_win: return "win"s;
		case game_state_e::e_failed: return "failed"s;
		case game_state_e::e_continue: return "continue"s;
		case game_state_e::e_pause: return "pause"s;
		}
		return "null"s;
	}

	bool m_console = true;
	std::unique_ptr<dev::window_group_t> m_window_group;

	struct {
		//difficulty_t difficulty = difficulty_t::e_normal;
		std::chrono::milliseconds difficulty_time;
		std::chrono::high_resolution_clock::time_point last_time = std::chrono::high_resolution_clock::now();

		direction_e current_direction = direction_e::e_null;
		game_state_e state = game_state_e::e_null;

		std::default_random_engine random_engine;
		std::uniform_int_distribution<core::ull_t> random_x, random_y;

		core::ull_t win_score = 30;
		std::vector<std::vector<cell_e>> map;
		std::optional<core::offset2_t<core::ull_t>> food;
		std::deque<core::offset2_t<core::ull_t>> snake;

		decltype(auto) build(core::ull_t win, difficulty_t difficulty, core::extent2_t<core::ull_t> extent) {
			difficulty_time = to_time(difficulty);
			win_score = win;
			random_x = std::uniform_int_distribution<core::ull_t>(1, extent.width() - 2);
			random_y = std::uniform_int_distribution<core::ull_t>(1, extent.height() - 2);

			map.resize(extent.height());
			for (auto iter = map.begin() + 1; iter != map.end() - 1; ++iter) {
				iter->resize(extent.width(), cell_e::e_empty);
				iter->front() = iter->back() = cell_e::e_wall;
			}
			map.front().resize(extent.width(), cell_e::e_wall);
			map.back().resize(extent.width(), cell_e::e_wall);
			state = game_state_e::e_pause;
		}
		decltype(auto) clean() {}
		decltype(auto) reset() {
			state = game_state_e::e_pause;
			current_direction = direction_e::e_null;
			if (food.has_value()) snake.push_back(food.value());
			for (auto iter : snake) map[iter.y()][iter.x()] = cell_e::e_empty;
			snake.clear();
			food = std::nullopt;
		}
		decltype(auto) random() { return core::offset2_t<core::ull_t>(random_x(random_engine), random_y(random_engine)); }
		decltype(auto) random_unique() {
			auto result = random();
			while (map[result.y()][result.x()] != cell_e::e_empty) { result = random(); }
			return result;
		}
		decltype(auto) game_logic(direction_e direction) {
			if (!food.has_value()) food = random_unique();
			if (snake.empty()) snake.push_back(random_unique());

			core::offset2_t<core::ull_t> next = snake.front();
			switch (current_direction) {
			case direction_e::e_up: { next.y() -= 1; }break;
			case direction_e::e_down: { next.y() += 1; }break;
			case direction_e::e_left: { next.x() -= 1; }break;
			case direction_e::e_right: { next.x() += 1; }break;
			}
			if (next != food.value() && next != snake.front() && map[next.y()][next.x()] != cell_e::e_empty) { state = game_state_e::e_failed; return; }

			map[food.value().y()][food.value().x()] = cell_e::e_food;
			map[snake.front().y()][snake.front().x()] = cell_e::e_body;
			map[snake.back().y()][snake.back().x()] = cell_e::e_empty;
			snake.push_front(next);

			if (next != food.value()) snake.pop_back();
			else food = std::nullopt;

			map[snake.back().y()][snake.back().x()] = cell_e::e_tail;
			map[snake.front().y()][snake.front().x()] = cell_e::e_head;

			if (snake.size() > win_score) { state = game_state_e::e_win; return; }
		}
		decltype(auto) caculate_direction(dev::key_e key) {
			switch (key) {
			case dev::key_e::e_w:
			case dev::key_e::e_up: {
				return current_direction != direction_e::e_down ? direction_e::e_up : current_direction;
			}break;
			case dev::key_e::e_a:
			case dev::key_e::e_left: {
				return current_direction != direction_e::e_right ? direction_e::e_left : current_direction;
			}break;
			case dev::key_e::e_s:
			case dev::key_e::e_down: {
				return current_direction != direction_e::e_up ? direction_e::e_down : current_direction;
			}break;
			case dev::key_e::e_d:
			case dev::key_e::e_right: {
				return current_direction != direction_e::e_left ? direction_e::e_right : current_direction;
			}break;
			}
			return current_direction;
		}
		decltype(auto) console_display(bool clean = true) const {
			if (clean) std::system("cls");
			std::cout << "[ vulkan snake game in console ]" << std::endl;
			std::cout << "how to use : \n\t" << "[ space ] -> begin/continue/pause\n\t" << "[ r ] -> reset\n\t" << "[ wasd ] or [ arrow ] -> move snake\n\t" << "[ f ] -> move fast" << std::endl;
			std::cout << "direction/state : [ " << to_string(current_direction) << "/" << to_string(state) << " ]" << std::endl;
			std::cout << "win_score/score : [ " << win_score << "/" << snake.size() - 1 << " ]" << std::endl;
			for (const auto& y : map) {
				for (const auto& x : y) std::cout << to_char(x);
				std::cout << std::endl;
			}
		}
		decltype(auto) update(std::queue<dev::event_t> queue) {
			bool is_run_logic = false;
			auto current_time = std::chrono::high_resolution_clock::now();
			if (current_time - last_time > difficulty_time) {
				last_time = current_time;
				is_run_logic = true;
			}

			while (!queue.empty()) {
				auto event = queue.front();
				if (event.etype == dev::event_e::e_keydown) {
					auto key = std::get<dev::key_e>(event.detail);
					std::cout << dev::to_string(key) << std::endl;
					if (key == dev::key_e::e_space && (state == game_state_e::e_continue || state == game_state_e::e_pause)) state = state == game_state_e::e_continue ? game_state_e::e_pause : game_state_e::e_continue;
					else if (key == dev::key_e::e_r) reset();
					else if (key == dev::key_e::e_f) is_run_logic = true;
					else if (state == game_state_e::e_continue) current_direction = caculate_direction(key);
				}
				queue.pop();
			}
			if (state == game_state_e::e_continue && is_run_logic) game_logic(current_direction);
		}
	}m_logic;

	struct {
		struct vertex_t {
			glm::vec3 pos;
			glm::vec2 uv;
		};
		struct mvp_t {
			glm::mat4 model;
			glm::mat4 view;
			glm::mat4 proj;
		};
		struct instance_t {
			glm::vec3 pos;
			glm::vec3 scale;
			glm::vec4 color;
			std::uint32_t texture_index;
		};
		std::wstring vert_path = L"./res/shader/snake.vert.spv", frag_path = L"./res/shader/snake.frag.spv";

		std::vector<std::vector<cell_e>>* map = nullptr;

		std::unique_ptr<vku::device_t> device;
		std::unique_ptr<vku::window_t> window;
		struct {
			std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
			vk::PipelineLayout pipeline_layout;
			vk::Pipeline pipeline;

			vk::DescriptorPool descriptor_pool;
			std::vector<vk::DescriptorSet> descriptor_sets;
		}pipeline;

		struct {
			std::vector<vertex_t> vertex = {
				{ {  1.0f,  1.0f, 0.0f }, { 1.0f, 1.0f } },
				{ {  0.0f,  1.0f, 0.0f }, { 0.0f, 1.0f } },
				{ {  0.0f,  0.0f, 0.0f }, { 0.0f, 0.0f } },
				{ {  1.0f,  0.0f, 0.0f }, { 1.0f, 0.0f } }
			};
			std::vector<uint32_t> index = { 0,1,2, 2,3,0 };
		}data;
		struct {
			vku::buffer_t vertex;
			vku::buffer_t index;
			vku::buffer_t mvp;
			vku::buffer_t instance;
		}buffer;
		struct {
			vk::Image image;
			vk::ImageLayout layout;
			vk::DeviceMemory memory;
			vk::ImageView view;
			std::uint32_t width, height;
			std::uint32_t layer;
			vk::Sampler sampler;
		}texture;

		decltype(auto) update_mvp() {
			auto extent = window->extent();
			auto radians = 90.0f;
			auto width = static_cast<float>(extent.width);
			auto height = static_cast<float>(extent.height);

			auto& mvp = core::memory_view_t(buffer.mvp.byte(), buffer.mvp.map()).ref<mvp_t>();

			mvp.proj = glm::perspective(glm::radians(radians), 1.0f, 0.0f, 100.0f);

			width >= height ? mvp.proj[0][0] *= (height / width) : mvp.proj[1][1] *= (width / height);

			mvp.view = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, -1.0f));
			mvp.model = glm::mat4(1.0f);
			mvp.model = glm::translate(mvp.model, glm::vec3(-1.5f, -1.0f, 0.0f));
			buffer.mvp.unmap();
		}
		decltype(auto) update_instance() {
			auto extent = core::extent2_t<std::size_t>{ map->at(0).size(), map->size() };
			auto count = buffer.instance.byte() / sizeof(instance_t);
			assert(extent.width() * extent.height() == count);
			auto instance_view = core::memory_view_t(buffer.instance.byte(), buffer.instance.map());
			float scale = 0.2f;
			float alpha = 0.8f;
			for (std::size_t y = 0; y < extent.height(); ++y) {
				for (std::size_t x = 0; x < extent.width(); ++x) {
					auto& instance = instance_view.sub_view((y * extent.width() + x) * sizeof(instance_t), sizeof(instance_t)).ref<instance_t>();
					instance.texture_index = (std::uint32_t)map->at(y)[x];
					instance.color = glm::vec4(0.2f, 1.0f, 0.5f, alpha);
					instance.scale = glm::vec3(scale, scale, 1.0f);
					instance.pos = glm::vec3(x * scale, y * scale, 0.0f);
				}
			}
			buffer.instance.unmap();
		}

		decltype(auto) build_vulkan(std::unique_ptr<dev::window_group_t>& window_group) {
			device = std::make_unique<vku::device_t>(
				vku::device_ci_t()
				.set_usage(vku::device_usage_t())
				//.set_debug(false)
				//.set_monitor(false)
				);
			window = std::make_unique<vku::window_t>(
				vku::window_ci_t()
				.set_device(device.get())
				.set_vsync(vsync)
				.set_hinstance(dev::priv::get_hinstance(window_group.get()))
				.set_hwnd(dev::priv::get_hwnd(window_group.get()))
				);
		}
		decltype(auto) clean_vulkan() {
			window = nullptr;
			device = nullptr;
		}
		decltype(auto) build_buffer() {
			// vertex buffer
			buffer.vertex = vku::buffer_t(
				vku::buffer_ci_t()
				.set_device(device.get())
				.set_usage_flags(vk::BufferUsageFlagBits::eTransferSrc)
				.set_memory_flags(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
				.set_memory_view(data.vertex)
			).copy(vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
			// index buffer
			buffer.index = vku::buffer_t(
				vku::buffer_ci_t()
				.set_device(device.get())
				.set_usage_flags(vk::BufferUsageFlagBits::eTransferSrc)
				.set_memory_flags(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
				.set_memory_view(data.index)
			).copy(vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst, vk::MemoryPropertyFlagBits::eDeviceLocal);
			// ubo buffer
			buffer.mvp = vku::buffer_t(
				vku::buffer_ci_t()
				.set_device(device.get())
				.set_usage_flags(vk::BufferUsageFlagBits::eUniformBuffer)
				.set_memory_flags(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
				//.set_memory_view(m_ubo)
				.set_byte(sizeof(mvp_t))
			);
			update_mvp();
			// instance buffer
			auto count = map->size() * map->at(0).size();
			buffer.instance = vku::buffer_t(
				vku::buffer_ci_t()
				.set_device(device.get())
				.set_usage_flags(vk::BufferUsageFlagBits::eVertexBuffer)
				.set_memory_flags(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
				//.set_memory_view(m_ubo)
				.set_byte(sizeof(instance_t) * count)
			);
			update_instance();
		}
		decltype(auto) clean_buffer() {
			buffer.instance = nullptr;
			buffer.mvp = nullptr;
			buffer.index = nullptr;
			buffer.vertex = nullptr;
		}
		decltype(auto) build_texture() {
			struct texture_t {
				texture_t(const char* path) {
					data = stbi_load(path, &width, &height, &channel, STBI_rgb_alpha);
				}
				~texture_t() {
					stbi_image_free(data);
				}
				decltype(auto) byte()const noexcept { return width * height * channel * sizeof(core::u8_t); }

				int width = 0, height = 0, channel = 0;
				core::u8_t* data = nullptr;
			};
			auto layer_count = (std::uint32_t)cell_e::e_null;

			texture_t res_empty("./res/texture/empty.png");

			auto buffer = vku::buffer_t(
				vku::buffer_ci_t()
				.set_device(device.get())
				.set_usage_flags(vk::BufferUsageFlagBits::eTransferSrc)
				.set_memory_flags(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
				.set_byte(res_empty.byte() * layer_count)
			);

			auto to_name = [](cell_e cell) {
				switch (cell) {
				case cell_e::e_empty:return std::string("empty");
				case cell_e::e_wall :return std::string("wall");
				case cell_e::e_food :return std::string("food");
				case cell_e::e_head :return std::string("head");
				case cell_e::e_body :return std::string("body");
				case cell_e::e_tail :return std::string("tail");
				case cell_e::e_null :return std::string("null");
				}
				return std::string("");
			};

			for (auto i = (std::uint32_t)cell_e::e_empty; i < (std::uint32_t)cell_e::e_null; ++i) {
				auto path = std::string("./res/texture/") + to_name(static_cast<cell_e>(i)) + ".png";
				texture_t tex(path.c_str());
				assert(tex.byte() == res_empty.byte());
				buffer.copy_from({ tex.byte() , tex.data }, std::nullopt, 0, tex.byte() * i);
			}

			auto format = vk::Format::eR8G8B8A8Unorm;
			//auto format = vk::Format::eR8G8B8A8Uint;

			auto imageCreateInfo = vk::ImageCreateInfo();
			imageCreateInfo.imageType = vk::ImageType::e2D;
			imageCreateInfo.format = format;
			imageCreateInfo.samples = vk::SampleCountFlagBits::e1;
			imageCreateInfo.tiling = vk::ImageTiling::eOptimal;
			imageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
			imageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
			imageCreateInfo.extent = { (std::uint32_t)res_empty.width, (std::uint32_t)res_empty.height, 1 };
			imageCreateInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
			imageCreateInfo.arrayLayers = layer_count;
			imageCreateInfo.mipLevels = 1;
			
			texture.image = vk::Device(*device).createImage(imageCreateInfo);
			auto memReqs = vk::Device(*device).getImageMemoryRequirements(texture.image);
			auto memAllocInfo = vk::MemoryAllocateInfo();
			memAllocInfo.allocationSize = memReqs.size;
			memAllocInfo.memoryTypeIndex = device->find_memory_index(memReqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal);
			texture.memory = vk::Device(*device).allocateMemory(memAllocInfo);
			vk::Device(*device).bindImageMemory(texture.image, texture.memory, 0);

			std::vector<vk::BufferImageCopy> bufferCopyRegions;

			for (uint32_t layer = 0; layer < layer_count; layer++)
			{
				vk::BufferImageCopy bufferCopyRegion = {};
				bufferCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
				bufferCopyRegion.imageSubresource.mipLevel = 0;
				bufferCopyRegion.imageSubresource.baseArrayLayer = layer;
				bufferCopyRegion.imageSubresource.layerCount = 1;
				bufferCopyRegion.imageExtent.width = res_empty.width;
				bufferCopyRegion.imageExtent.height = res_empty.height;
				bufferCopyRegion.imageExtent.depth = 1;
				bufferCopyRegion.imageOffset.x = 0;
				bufferCopyRegion.imageOffset.y = 0;
				bufferCopyRegion.imageOffset.z = 0;
				bufferCopyRegion.bufferOffset = res_empty.byte() * layer;
				bufferCopyRegions.push_back(bufferCopyRegion);
			}

			auto cmd = device->begin_single_command();

			// Image barrier for optimal image (target)
			// Set initial layout for all array layers (faces) of the optimal (target) tiled texture
			vk::ImageSubresourceRange subresourceRange = {};
			subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
			subresourceRange.baseMipLevel = 0;
			subresourceRange.levelCount = 1;
			subresourceRange.layerCount = layer_count;

			// set image layout
			auto imageMemoryBarrier =
				vk::ImageMemoryBarrier()
				.setOldLayout(vk::ImageLayout::eUndefined)
				.setNewLayout(vk::ImageLayout::eTransferDstOptimal)
				.setImage(texture.image)
				.setSubresourceRange(subresourceRange)
				.setSrcAccessMask(vk::AccessFlags())
				.setDstAccessMask(vk::AccessFlagBits::eTransferWrite);

			cmd.first.pipelineBarrier(
				vk::PipelineStageFlagBits::eAllCommands,
				vk::PipelineStageFlagBits::eAllCommands,
				vk::DependencyFlags(),
				nullptr, 
				nullptr, 
				{ imageMemoryBarrier }
			);

			// copy
			cmd.first.copyBufferToImage(
				buffer,
				texture.image,
				vk::ImageLayout::eTransferDstOptimal,
				bufferCopyRegions);

			// set image layout
			imageMemoryBarrier.setOldLayout(vk::ImageLayout::eTransferDstOptimal);
			imageMemoryBarrier.setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal);
			imageMemoryBarrier.setSrcAccessMask(vk::AccessFlagBits::eTransferWrite | vk::AccessFlagBits::eHostWrite);
			imageMemoryBarrier.setDstAccessMask(vk::AccessFlagBits::eShaderRead);

			cmd.first.pipelineBarrier(
				vk::PipelineStageFlagBits::eAllCommands,
				vk::PipelineStageFlagBits::eAllCommands,
				vk::DependencyFlags(),
				nullptr,
				nullptr,
				{ imageMemoryBarrier }
			);
			device->end_single_command(cmd);

			

			texture.layout = vk::ImageLayout::eShaderReadOnlyOptimal;

			// Create sampler
			auto samplerCreateInfo = vk::SamplerCreateInfo();
			samplerCreateInfo.magFilter = vk::Filter::eLinear;
			samplerCreateInfo.minFilter = vk::Filter::eLinear;
			samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
			samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eClampToEdge;
			samplerCreateInfo.addressModeV = samplerCreateInfo.addressModeU;
			samplerCreateInfo.addressModeW = samplerCreateInfo.addressModeU;
			samplerCreateInfo.mipLodBias = 0.0f;
			samplerCreateInfo.maxAnisotropy = 1.0f;
			samplerCreateInfo.anisotropyEnable = VK_FALSE;// device->enabledFeatures.samplerAnisotropy;
			samplerCreateInfo.compareOp = vk::CompareOp::eNever;
			samplerCreateInfo.minLod = 0.0f;
			samplerCreateInfo.maxLod = (float)1;
			samplerCreateInfo.borderColor = vk::BorderColor::eFloatOpaqueWhite;
			texture.sampler = vk::Device(*device).createSampler(samplerCreateInfo);

			// Create image view
			auto viewCreateInfo = vk::ImageViewCreateInfo();
			viewCreateInfo.viewType = vk::ImageViewType::e2DArray;
			viewCreateInfo.format = format;
			viewCreateInfo.components = { vk::ComponentSwizzle::eR, vk::ComponentSwizzle::eG, vk::ComponentSwizzle::eB, vk::ComponentSwizzle::eA };
			viewCreateInfo.subresourceRange = { vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 };
			viewCreateInfo.subresourceRange.layerCount = layer_count;
			viewCreateInfo.subresourceRange.levelCount = 1;
			viewCreateInfo.image = texture.image;
			texture.view = vk::Device(*device).createImageView(viewCreateInfo);

		}
		decltype(auto) clean_texture() {
			vk::Device(*device).destroySampler(texture.sampler);
			vk::Device(*device).destroyImageView(texture.view);
			vk::Device(*device).freeMemory(texture.memory);
			vk::Device(*device).destroyImage(texture.image);
		}
		decltype(auto) build_layout() {
			// pipeline layout
			std::vector<vk::DescriptorSetLayoutBinding> descriptor_set_layout_bindings_u;
			descriptor_set_layout_bindings_u.push_back(
				vk::DescriptorSetLayoutBinding()
				.setStageFlags(vk::ShaderStageFlagBits::eVertex)
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1)
				.setBinding(0)
			);
			descriptor_set_layout_bindings_u.push_back(
				vk::DescriptorSetLayoutBinding()
				.setStageFlags(vk::ShaderStageFlagBits::eFragment)
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDescriptorCount(1)
				.setBinding(1)
			);
			pipeline.descriptor_set_layouts.push_back(
				vk::Device(*device).createDescriptorSetLayout(
					vk::DescriptorSetLayoutCreateInfo()
					.setBindingCount(descriptor_set_layout_bindings_u.size())
					.setPBindings(descriptor_set_layout_bindings_u.data())
				)
			);
			pipeline.pipeline_layout = vk::Device(*device).createPipelineLayout(
				vk::PipelineLayoutCreateInfo()
				.setSetLayoutCount(pipeline.descriptor_set_layouts.size())
				.setPSetLayouts(pipeline.descriptor_set_layouts.data())
			);

			// descriptor
			std::vector<vk::DescriptorPoolSize> pool_sizes;
			pool_sizes.push_back(
				vk::DescriptorPoolSize()
				.setType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1)
			);
			pool_sizes.push_back(
				vk::DescriptorPoolSize()
				.setType(vk::DescriptorType::eCombinedImageSampler)
				.setDescriptorCount(1)
			);
			auto descriptor_pool_ci = vk::DescriptorPoolCreateInfo()
				.setPoolSizeCount(pool_sizes.size())
				.setPPoolSizes(pool_sizes.data())
				.setMaxSets(2);

			pipeline.descriptor_pool = vk::Device(*device).createDescriptorPool(descriptor_pool_ci);

			auto descriptor_set_ai = vk::DescriptorSetAllocateInfo()
				.setDescriptorPool(pipeline.descriptor_pool)
				.setDescriptorSetCount(pipeline.descriptor_set_layouts.size())
				.setPSetLayouts(pipeline.descriptor_set_layouts.data());

			pipeline.descriptor_sets = vk::Device(*device).allocateDescriptorSets(descriptor_set_ai);

			// mvp
			auto descriptor_buffer_info = vk::DescriptorBufferInfo()
				.setBuffer(buffer.mvp)
				.setOffset(0)
				.setRange(buffer.mvp.byte());
			auto buffer_write = vk::WriteDescriptorSet()
				.setDescriptorType(vk::DescriptorType::eUniformBuffer)
				.setDescriptorCount(1)
				.setDstSet(pipeline.descriptor_sets[0])
				.setDstBinding(0)
				.setPBufferInfo(&descriptor_buffer_info);
			// image
			auto descriptor_image_info = vk::DescriptorImageInfo()
				.setImageView(texture.view)
				.setImageLayout(texture.layout)
				.setSampler(texture.sampler);
			auto image_write = vk::WriteDescriptorSet()
				.setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
				.setDescriptorCount(1)
				.setDstSet(pipeline.descriptor_sets[0])
				.setDstBinding(1)
				.setPImageInfo(&descriptor_image_info);
			// write
			std::vector<vk::WriteDescriptorSet> write_descriptor_sets;
			write_descriptor_sets.push_back(buffer_write);
			write_descriptor_sets.push_back(image_write);
			vk::Device(*device).updateDescriptorSets(write_descriptor_sets, 0);

		}
		decltype(auto) clean_layout() {
			vk::Device(*device).destroyDescriptorPool(pipeline.descriptor_pool);
			//vk::Device(*device).freeDescriptorSets(pipeline.descriptor_pool, pipeline.descriptor_sets);

			vk::Device(*device).destroyPipelineLayout(pipeline.pipeline_layout);
			for (auto iter : pipeline.descriptor_set_layouts) vk::Device(*device).destroyDescriptorSetLayout(iter);
		}
		decltype(auto) build_pipeline() {
			// vertex input state
			std::vector<vk::VertexInputBindingDescription> vertex_input_binding_descriptions;
			vertex_input_binding_descriptions.push_back(
				vk::VertexInputBindingDescription()
				.setBinding(0)
				.setInputRate(vk::VertexInputRate::eVertex)
				.setStride(sizeof(vertex_t))
			);
			vertex_input_binding_descriptions.push_back(
				vk::VertexInputBindingDescription()
				.setBinding(1)
				.setInputRate(vk::VertexInputRate::eInstance)
				.setStride(sizeof(instance_t))
			);
			std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions;
			// vertex
			std::uint32_t binding = 0, location = 0;
			vertex_input_attribute_descriptions.push_back(
				vk::VertexInputAttributeDescription()
				.setBinding(binding)
				.setLocation(location++)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setOffset(offsetof(vertex_t, pos))
			);
			vertex_input_attribute_descriptions.push_back(
				vk::VertexInputAttributeDescription()
				.setBinding(binding)
				.setLocation(location++)
				.setFormat(vk::Format::eR32G32Sfloat)
				.setOffset(offsetof(vertex_t, uv))
			);
			++binding;
			vertex_input_attribute_descriptions.push_back(
				vk::VertexInputAttributeDescription()
				.setBinding(binding)
				.setLocation(location++)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setOffset(offsetof(instance_t, pos))
			);
			vertex_input_attribute_descriptions.push_back(
				vk::VertexInputAttributeDescription()
				.setBinding(binding)
				.setLocation(location++)
				.setFormat(vk::Format::eR32G32B32Sfloat)
				.setOffset(offsetof(instance_t, scale))
			);
			vertex_input_attribute_descriptions.push_back(
				vk::VertexInputAttributeDescription()
				.setBinding(binding)
				.setLocation(location++)
				.setFormat(vk::Format::eR32G32B32A32Sfloat)
				.setOffset(offsetof(instance_t, color))
			);
			vertex_input_attribute_descriptions.push_back(
				vk::VertexInputAttributeDescription()
				.setBinding(binding)
				.setLocation(location++)
				.setFormat(vk::Format::eR32Uint)
				.setOffset(offsetof(instance_t, texture_index))
			);
			
			auto vertex_input_ci = vk::PipelineVertexInputStateCreateInfo()
				.setVertexBindingDescriptionCount(vertex_input_binding_descriptions.size())
				.setPVertexBindingDescriptions(vertex_input_binding_descriptions.data())
				.setVertexAttributeDescriptionCount(vertex_input_attribute_descriptions.size())
				.setPVertexAttributeDescriptions(vertex_input_attribute_descriptions.data());
			// shader
			std::vector<vk::PipelineShaderStageCreateInfo> shader_cis;
			shader_cis.push_back(
				vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eVertex)
				.setModule(device->build_shader(vert_path))
				.setPName("main")
			);
			shader_cis.push_back(
				vk::PipelineShaderStageCreateInfo()
				.setStage(vk::ShaderStageFlagBits::eFragment)
				.setModule(device->build_shader(frag_path))
				.setPName("main")
			);
			// assembly
			auto input_assembly_ci = vk::PipelineInputAssemblyStateCreateInfo().setTopology(vk::PrimitiveTopology::eTriangleList);
			// multisample
			auto multisample_ci = vk::PipelineMultisampleStateCreateInfo().setRasterizationSamples(vk::SampleCountFlagBits::e1);
			// viewport and scissor
			auto viewport_ci = vk::PipelineViewportStateCreateInfo()
				.setViewportCount(1)
				.setPViewports(nullptr)
				.setScissorCount(1)
				.setPScissors(nullptr);
			// dynamic 
			std::vector<vk::DynamicState> dynamic_states;
			dynamic_states.push_back(vk::DynamicState::eViewport);
			dynamic_states.push_back(vk::DynamicState::eScissor);
			auto dynamic_ci = vk::PipelineDynamicStateCreateInfo()
				.setDynamicStateCount(static_cast<uint32_t>(dynamic_states.size()))
				.setPDynamicStates(dynamic_states.data());
			// color blend
			/*
				SrcColorBlendFactor : src(rgb) = Current(rgb) * BlendFactor
				DstColorBlendFactor : dst(rgb) = Old(rgb) * BlendFactor
				ColorBlendOp		: new(rgb) = src(rgb) <BlendOp> dst(rgb)
			*/
			std::vector<vk::PipelineColorBlendAttachmentState> color_blend_attachment_states;
			color_blend_attachment_states.push_back(
				vk::PipelineColorBlendAttachmentState()
				.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA)
				//.setColorWriteMask((vk::ColorComponentFlags)0xf)
				.setBlendEnable(VK_TRUE)
				.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
				.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
				.setColorBlendOp(vk::BlendOp::eAdd)
				.setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
				.setDstAlphaBlendFactor(vk::BlendFactor::eZero)
				.setAlphaBlendOp(vk::BlendOp::eAdd)
			);
			auto color_blend_ci = vk::PipelineColorBlendStateCreateInfo()
				.setAttachmentCount(color_blend_attachment_states.size())
				.setPAttachments(color_blend_attachment_states.data())
				.setLogicOpEnable(VK_FALSE);
			// depth and stencil
			auto depth_stencil_ci = vk::PipelineDepthStencilStateCreateInfo()
				.setDepthTestEnable(VK_TRUE)
				.setDepthWriteEnable(VK_TRUE)
				.setDepthCompareOp(vk::CompareOp::eAlways)//vk::CompareOp::eLessOrEqual £¡£¡£¡×¢Òâ£¡£¡£¡
				.setDepthBoundsTestEnable(VK_FALSE)
				.setBack(vk::StencilOpState().setFailOp(vk::StencilOp::eKeep).setPassOp(vk::StencilOp::eKeep).setCompareOp(vk::CompareOp::eAlways))
				.setStencilTestEnable(VK_FALSE)
				.setFront(vk::StencilOpState().setFailOp(vk::StencilOp::eKeep).setPassOp(vk::StencilOp::eKeep).setCompareOp(vk::CompareOp::eAlways));
			// rasterization
			auto rasterization_ci = vk::PipelineRasterizationStateCreateInfo()
				.setPolygonMode(vk::PolygonMode::eFill)
				.setCullMode(vk::CullModeFlagBits::eNone)
				.setFrontFace(vk::FrontFace::eCounterClockwise)
				.setDepthClampEnable(VK_FALSE)
				.setRasterizerDiscardEnable(VK_FALSE)
				.setDepthBiasEnable(VK_FALSE)
				.setLineWidth(1.0f);
			// pipeline
			pipeline.pipeline = vk::Device(*device).createGraphicsPipeline(nullptr,
				vk::GraphicsPipelineCreateInfo()
				.setRenderPass(*window)
				.setLayout(pipeline.pipeline_layout)
				.setPVertexInputState(&vertex_input_ci)
				.setStageCount(shader_cis.size())
				.setPStages(shader_cis.data())
				.setPInputAssemblyState(&input_assembly_ci)
				.setPMultisampleState(&multisample_ci)
				.setPViewportState(&viewport_ci)
				.setPDynamicState(&dynamic_ci)
				.setPColorBlendState(&color_blend_ci)
				.setPDepthStencilState(&depth_stencil_ci)
				.setPRasterizationState(&rasterization_ci)
			);
			for (auto iter : shader_cis) device->clean_shader(iter.module);
		}
		decltype(auto) clean_pipeline() {
			vk::Device(*device).destroyPipeline(pipeline.pipeline);
		}
		decltype(auto) render() {
			window->caculate({
				[this](vk::CommandBuffer cmd, vk::Rect2D rect) {
					auto viewport = vk::Viewport()
						.setWidth((float)rect.extent.width)
						.setHeight((float)rect.extent.height)
						.setX(0.0f)
						.setY(0.0f)
						.setMinDepth(0.0f)
						.setMaxDepth(1.0f);
					cmd.setViewport(0, { viewport });
					cmd.setScissor(0, { rect });
					// pipeline layout and descriptor can used for different pipeline
					cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipeline.pipeline_layout, 0, pipeline.descriptor_sets, nullptr);

					cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.pipeline);
					cmd.bindVertexBuffers(0, { buffer.vertex }, { 0 });
					cmd.bindVertexBuffers(1, { buffer.instance }, { 0 });
					cmd.bindIndexBuffer(buffer.index, 0, vk::IndexType::eUint32);
					auto count = buffer.instance.byte() / sizeof(instance_t);
					cmd.drawIndexed(buffer.index.byte() / sizeof(std::uint32_t), count, 0, 0, 0);
				}
			});
		}
		decltype(auto) build(std::unique_ptr<dev::window_group_t>& window_group, std::vector<std::vector<cell_e>>* map) {
			this->map = map;
			build_vulkan(window_group);
			build_buffer();
			build_texture();
			build_layout();
			build_pipeline();
			
			render();
		}
		decltype(auto) clean() {
			
			clean_pipeline();
			clean_layout();
			clean_texture();
			clean_buffer();
			clean_vulkan();
		}
		
		decltype(auto) update(std::queue<dev::event_t> queue) {
			while (!queue.empty()) {
				auto event = queue.front();
				if (event.etype == dev::event_e::e_resize) update_mvp();
				queue.pop();
			}
			update_instance();
			window->run();
		}
	}m_vulkan;
public:
	decltype(auto) run() {
		while (m_window_group->is_active()) {
			auto& queue = m_window_group->update();
			m_logic.update(queue);
			m_vulkan.update(queue);
			if (m_console) m_logic.console_display();
		}
	}
public:
	snake_game_t(snake_game_ci_t const& ci) : m_console(ci.console_game) {
		// build window
		dev::extent_t window_extent = { static_cast<core::u32_t>((ci.extent.width() + 1) * ci.window_rate), static_cast<core::u32_t>((ci.extent.height() + 1) * ci.window_rate) };
		m_window_group = dev::build_window_group(
			dev::window_group_ci_t()
			.set_title(L"Vulkan Snake")
			.set_rect({ {200, 100}, window_extent })
		);

		// build logic
		m_logic.build(ci.win_score, ci.difficulty, ci.extent);

		// build vulkan
		m_vulkan.build(m_window_group, &m_logic.map);
	}
	~snake_game_t() {
		m_vulkan.clean();
		m_logic.clean();
	}
};

int main(int argc, char** argv) {
	{ 
		snake_game_t(
			snake_game_ci_t()
			.set_console_game(true)
			//.set_something() or by default
		).run(); 
	}
	//std::cin.get();
}