#pragma once

#define VK_USE_PLATFORM_WIN32_KHR
#include <vulkan/vulkan.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <bitset>
#include <optional>
#include <numeric>
#include <climits>
#include <unordered_map>
#include <fstream>
using namespace std::string_literals;

#ifdef max
#undef max
#endif

/*
queue [ 0 ]
		flags/count [ 15 / 16 ]
		[ Graphics ]
		[ Compute ]
		[ Transfer ]
		[ SparseBinding ]
queue [ 1 ]
		flags/count [ 12 / 2 ]
		[ Transfer ]
		[ SparseBinding ]
queue [ 2 ]
		flags/count [ 14 / 8 ]
		[ Compute ]
		[ Transfer ]
		[ SparseBinding ]
		0	1	2	C
	G	16			1
	C	16		8	2
	T	16	2	8	3
	S	16	2	8	3

	注意：Graphics和Compute在同一个family会导致GPU使用过高，而将其分开的话就能无GPU开销。
*/

namespace cw {
	namespace graphic {
		namespace vulkan {
			namespace func {
				inline decltype(auto) find_queue_indexes(vk::PhysicalDevice physical_device, vk::QueueFlags flags, std::optional<std::uint32_t> const& max_count = std::nullopt) {
					auto properties = physical_device.getQueueFamilyProperties();
					// find the closest matching
					using index_excess_t = std::pair<std::uint32_t, std::uint32_t>;
					std::vector<index_excess_t> supports;
					for (std::uint32_t i = 0; i < properties.size(); ++i) {
						if (flags & properties[i].queueFlags) {
							index_excess_t ie;
							ie.first = i;
							ie.second = 0;
							if (bool(flags & vk::QueueFlagBits::eGraphics) == false && bool(properties[i].queueFlags & vk::QueueFlagBits::eGraphics) == true) ++ie.second;
							if (bool(flags & vk::QueueFlagBits::eCompute) == false && bool(properties[i].queueFlags & vk::QueueFlagBits::eCompute) == true) ++ie.second;
							if (bool(flags & vk::QueueFlagBits::eTransfer) == false && bool(properties[i].queueFlags & vk::QueueFlagBits::eTransfer) == true) ++ie.second;
							if (bool(flags & vk::QueueFlagBits::eSparseBinding) == false && bool(properties[i].queueFlags & vk::QueueFlagBits::eSparseBinding) == true) ++ie.second;
							supports.push_back(ie);
						}
					}
					std::optional<std::vector<std::uint32_t>> result = std::nullopt;
					std::vector<std::uint32_t> results;
					if (!supports.empty()) {
						std::sort(supports.begin(), supports.end(), [](index_excess_t const& a, index_excess_t const& b)->bool {return a.second < b.second; });
						for (const auto& iter : supports) {
							if (max_count.has_value()) {
								if (properties[iter.first].queueCount >= max_count.value()) results.push_back(iter.first);
							}
							else results.push_back(iter.first);
						}
					}
					if (!results.empty()) result = results;
					if (!result.has_value()) std::cerr << "can't find queue index" << std::endl;
					return result;
				}
				inline decltype(auto) find_memory_index(vk::PhysicalDevice physical_device, std::uint32_t type_bits, vk::MemoryPropertyFlags flags) {
					auto properties = physical_device.getMemoryProperties();
					std::optional <std::uint32_t> result = std::nullopt;
					for (std::uint32_t i = 0; i < properties.memoryTypeCount; i++) {
						if ((type_bits & 1) == 1) {
							if ((properties.memoryTypes[i].propertyFlags & flags) == flags) {
								result = i;
								break;
							}
						}
						type_bits >>= 1;
					}
					if (!result.has_value()) std::cerr << "can't memory queue index" << std::endl;
					return result;
				}
				inline decltype(auto) find_formats(vk::PhysicalDevice physical_device, std::vector<vk::Format> const& formats, vk::FormatProperties const& features) {
					std::optional<std::vector<vk::Format>> result = std::nullopt;
					std::vector<vk::Format> results;
					for (const auto& format : formats) {
						auto property = physical_device.getFormatProperties(format);
						if ((features.linearTilingFeatures != vk::FormatFeatureFlagBits()) && !(property.linearTilingFeatures & features.linearTilingFeatures)) continue;
						if ((features.optimalTilingFeatures != vk::FormatFeatureFlagBits()) && !(property.optimalTilingFeatures & features.optimalTilingFeatures)) continue;
						if ((features.bufferFeatures != vk::FormatFeatureFlagBits()) && !(property.bufferFeatures & features.bufferFeatures)) continue;
						results.push_back(format);
					}
					if (!results.empty()) result = results;
					if (!result.has_value()) std::cerr << "can't find formats" << std::endl;
					return result;
				}
				inline decltype(auto) find_depth_stencil_formats(vk::PhysicalDevice physical_device,
					std::vector<vk::Format> const& formats = {
						vk::Format::eD32SfloatS8Uint,
						vk::Format::eD32Sfloat,
						vk::Format::eD24UnormS8Uint,
						vk::Format::eD16UnormS8Uint,
						vk::Format::eD16Unorm
					}, vk::FormatProperties const& features = vk::FormatProperties(vk::FormatFeatureFlags(), vk::FormatFeatureFlagBits::eDepthStencilAttachment, vk::FormatFeatureFlags())) {
					return find_formats(physical_device, formats, features);
				}
				// if the bool in std::pair<vk::SurfaceFormatKHR, bool> is true means ignore vk::ColorSpaceKHR
				inline decltype(auto) find_surface_formats(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, std::optional<std::vector<std::pair<vk::SurfaceFormatKHR, bool>>> const& surface_formats = std::nullopt) {
					std::optional<std::vector<vk::SurfaceFormatKHR>> result = std::nullopt;
					std::vector<vk::SurfaceFormatKHR> results;
					auto supports = physical_device.getSurfaceFormatsKHR(surface);
					if (surface_formats.has_value()) {
						for (const auto& need : surface_formats.value()) {
							for (const auto& support : supports) {
								if (need.first.format != vk::Format::eUndefined && support.format != need.first.format) continue;
								if (!need.second && support.colorSpace != need.first.colorSpace) continue;
								results.push_back(support);
							}
						}
					}
					else results = supports;
					if (!results.empty()) result = results;
					if (!result.has_value()) std::cerr << "can't find surface formats" << std::endl;
					return result;
				}
				inline decltype(auto) find_surface_present_modes(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface,
					std::optional<std::vector<vk::PresentModeKHR>> const& present_modes = std::vector<vk::PresentModeKHR>{
						vk::PresentModeKHR::eMailbox,
						vk::PresentModeKHR::eImmediate,
						vk::PresentModeKHR::eFifo
					}) {
					std::optional<std::vector<vk::PresentModeKHR>> result = std::nullopt;
					std::vector<vk::PresentModeKHR> results;
					auto supports = physical_device.getSurfacePresentModesKHR(surface);
					if (present_modes.has_value()) {
						for (const auto& need : present_modes.value())
							for (const auto& support : supports)
								if (need == support) { results.push_back(need); break; }
					}
					else results = supports;
					if (!results.empty()) result = results;
					if (!result.has_value()) std::cerr << "can't find present modes" << std::endl;
					return result;
				}
				inline decltype(auto) find_surface_composite_alphas(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface,
					std::optional<std::vector<vk::CompositeAlphaFlagBitsKHR>> const& flags = std::vector<vk::CompositeAlphaFlagBitsKHR>{
						vk::CompositeAlphaFlagBitsKHR::eOpaque,
						vk::CompositeAlphaFlagBitsKHR::ePreMultiplied,
						vk::CompositeAlphaFlagBitsKHR::ePostMultiplied,
						vk::CompositeAlphaFlagBitsKHR::eInherit
					}) {
					std::optional<std::vector<vk::CompositeAlphaFlagBitsKHR>> result = std::nullopt;
					std::vector<vk::CompositeAlphaFlagBitsKHR> results;
					auto capability = physical_device.getSurfaceCapabilitiesKHR(surface);
					if (flags.has_value()) {
						for (const auto& iter : flags.value())
							if (iter & capability.supportedCompositeAlpha) results.push_back(iter);
					}
					else results.push_back(vk::CompositeAlphaFlagBitsKHR::eOpaque);
					if (!results.empty()) result = results;
					if (!result.has_value()) std::cerr << "can't find composite alphas" << std::endl;
					return result;
				}
				inline decltype(auto) find_surface_image_usage(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface,
					std::optional<std::vector<vk::ImageUsageFlags>> const& flags = std::vector<vk::ImageUsageFlags>{
						vk::ImageUsageFlagBits::eTransferSrc,
						vk::ImageUsageFlagBits::eTransferDst
					}) {
					auto capability = physical_device.getSurfaceCapabilitiesKHR(surface);
					vk::ImageUsageFlags result;
					if (flags.has_value()) {
						for (const auto& iter : flags.value())
							if (iter & capability.supportedUsageFlags) result |= iter;
					}
					else result = capability.supportedUsageFlags;
					return result;
				}
				inline decltype(auto) find_surface_image_count(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface, std::optional<bool> max_or_min = true, std::optional<std::uint32_t> const& count = std::nullopt) {
					auto capability = physical_device.getSurfaceCapabilitiesKHR(surface);
					std::uint32_t result = 0;
					if (max_or_min.has_value()) result = max_or_min.value() ? capability.maxImageCount : capability.minImageCount;
					else if (count.has_value()) result = count.value();
					else result = (capability.maxImageCount + capability.minImageCount) / 2;
					if (result > capability.maxImageCount) result = capability.maxImageCount;
					if (result < capability.minImageCount) result = capability.minImageCount;
					return result;
				}
				inline decltype(auto) find_surface_transforms(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface,
					std::optional<std::vector<vk::SurfaceTransformFlagBitsKHR>> const& flags = std::vector<vk::SurfaceTransformFlagBitsKHR>{
						vk::SurfaceTransformFlagBitsKHR::eIdentity
					}) {
					auto capability = physical_device.getSurfaceCapabilitiesKHR(surface);
					std::optional<std::vector<vk::SurfaceTransformFlagBitsKHR>> result;
					std::vector<vk::SurfaceTransformFlagBitsKHR> results;
					if (flags.has_value()) {
						for (const auto& iter : flags.value())
							if (iter & capability.supportedTransforms) results.push_back(iter);
					}
					else results.push_back(capability.currentTransform);
					if (!results.empty()) result = results;
					if (!result.has_value()) std::cerr << "can't find surface transforms" << std::endl;
					return result;
				}

				inline decltype(auto) get_surface_supports(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
					auto queue_family_properties = physical_device.getQueueFamilyProperties();
					std::vector<vk::Bool32> surface_supports(queue_family_properties.size());
					bool exist = false;
					for (uint32_t i = 0; i < queue_family_properties.size(); i++) { surface_supports[i] = physical_device.getSurfaceSupportKHR(i, surface); if (surface_supports[i]) exist = true; }
					assert(exist);
					return surface_supports;
				}
				inline decltype(auto) get_surface_extent(vk::PhysicalDevice physical_device, vk::SurfaceKHR surface) {
					return physical_device.getSurfaceCapabilitiesKHR(surface).currentExtent;
				}
			}

			struct device_usage_t {
				using detail_t = std::pair<bool, std::uint32_t>;
				detail_t graphic = detail_t{ true, 1 };
				detail_t compute = detail_t{ true, 1 };
				detail_t transfer = detail_t{ true, 1 };
				detail_t sparse_binding = detail_t{ true, 1 };
				std::optional<vk::PhysicalDeviceType> physical_type = std::nullopt;
				std::optional<vk::PhysicalDeviceFeatures> features = std::nullopt;
				std::optional<std::vector<const char*>> device_extensions = std::nullopt;
				decltype(auto) set_graphic(bool const& graphic, std::uint32_t const& count = 1) { this->graphic = { graphic, count }; return *this; }
				decltype(auto) set_compute(bool const& compute, std::uint32_t const& count = 1) { this->compute = { compute, count }; return *this; }
				decltype(auto) set_transfer(bool const& transfer, std::uint32_t const& count = 1) { this->transfer = { transfer, count }; return *this; }
				decltype(auto) set_sparse_binding(bool const& sparse_binding, std::uint32_t const& count = 1) { this->sparse_binding = { sparse_binding, count }; return *this; }
				decltype(auto) set_physical_type(vk::PhysicalDeviceType const& physical_type) { this->physical_type = physical_type; return *this; }
				decltype(auto) set_features(vk::PhysicalDeviceFeatures const& features) { this->features = features; return *this; }
				decltype(auto) set_device_extensions(std::vector<const char*> const& device_extensions) { this->device_extensions = device_extensions; return *this; }
			};
			struct device_ci_t {
				std::uint32_t api_version = VK_API_VERSION_1_1;
				bool debug = true;
				bool monitor = true;
				bool surface = true;
				device_usage_t usage;
				std::optional<std::vector<const char*>> instance_layers;
				std::optional<std::vector<const char*>> instance_extensions;
				decltype(auto) set_api_version(std::uint32_t const& api_version) { this->api_version = api_version; return *this; }
				decltype(auto) set_debug(bool const& debug) { this->debug = debug; return *this; }
				decltype(auto) set_monitor(bool const& monitor) { this->monitor = monitor; return *this; }
				decltype(auto) set_surface(bool const& surface) { this->surface = surface; return *this; }
				decltype(auto) set_usage(device_usage_t const& usage) { this->usage = usage; return *this; }
				decltype(auto) set_instance_layers(std::vector<const char*> const& instance_layers) { this->instance_layers = instance_layers; return *this; }
				decltype(auto) set_instance_extensions(std::vector<const char*> const& instance_extensions) { this->instance_extensions = instance_extensions; return *this; }
			};
			class device_t {
			public:
				struct queue_family_t {
					std::uint32_t index = std::numeric_limits<std::uint32_t>::max();
					//std::uint32_t count = std::numeric_limits<std::uint32_t>::max();
					std::vector<vk::Queue> queues;
					vk::CommandPool command_pool;
					decltype(auto) set_index(std::uint32_t const& index) { this->index = index; return *this; }
					decltype(auto) set_queues(std::vector<vk::Queue> const& queues) { this->queues = queues; return *this; }
					decltype(auto) set_command_pool(vk::CommandPool const& command_pool) { this->command_pool = command_pool; return *this; }
					//decltype(auto) set_count(std::uint32_t const& count) { this->count = count; return *this; }
				};
				using single_command_t = std::pair<vk::CommandBuffer, const queue_family_t*>;

				//device_t() {}
				device_t(device_ci_t const& ci) {
					// instance
					{
						for (const auto& iter : vk::enumerateInstanceLayerProperties()) m_instance_support_layers.push_back(iter.layerName);
						for (const auto& iter : vk::enumerateInstanceExtensionProperties()) m_instance_support_extensions.push_back(iter.extensionName);
						std::vector<std::string> layers, extensions;
						if (ci.instance_layers.has_value()) layers.assign(ci.instance_layers.value().begin(), ci.instance_layers.value().end());
						if (ci.instance_extensions.has_value()) extensions.assign(ci.instance_extensions.value().begin(), ci.instance_extensions.value().end());
						if (ci.debug) {
							layers.push_back("VK_LAYER_KHRONOS_validation");
							layers.push_back("VK_LAYER_LUNARG_standard_validation");
							extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
							extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
						}
						if (ci.monitor) layers.push_back("VK_LAYER_LUNARG_monitor");
						if (ci.surface) {
							extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);
#ifdef VK_USE_PLATFORM_WIN32_KHR
							extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
#endif
						}

						std::vector<const char*> enable_layers, enable_extensions;
						for (const auto& user : layers) {
							for (const auto& support : m_instance_support_layers) {
								if (std::string(user) == support) {
									bool exist = false;
									for (const auto& iter : enable_layers) if (support == iter) exist = true;
									if (!exist) enable_layers.push_back(user.c_str());
									break;
								}
							}
						}
						for (const auto& user : extensions) {
							for (const auto& support : m_instance_support_extensions) {
								if (std::string(user) == support) {
									bool exist = false;
									for (const auto& iter : enable_extensions) if (support == iter) exist = true;
									if (!exist) enable_extensions.push_back(user.c_str());
									break;
								}
							}
						}
						if (!enable_layers.empty()) {
							std::vector<std::string> names;
							for (const auto& iter : enable_layers) names.push_back(iter);
							m_instance_enable_layers = names;
						}
						if (!enable_extensions.empty()) {
							std::vector<std::string> names;
							for (const auto& iter : enable_extensions) names.push_back(iter);
							m_instance_enable_extensions = names;
						}

						auto app_info = vk::ApplicationInfo().setApiVersion(ci.api_version);

						m_instance = vk::createInstance(
							vk::InstanceCreateInfo()
							.setPApplicationInfo(&app_info)
							.setEnabledLayerCount(enable_layers.size())
							.setPpEnabledLayerNames(enable_layers.data())
							.setEnabledExtensionCount(enable_extensions.size())
							.setPpEnabledExtensionNames(enable_extensions.data())
						);
						m_dispatch.init(m_instance, (vk::Device)nullptr);//, (vk::Device)nullptr

						if (ci.debug) {
							m_debug_utils_messenger = m_instance.createDebugUtilsMessengerEXT(
								vk::DebugUtilsMessengerCreateInfoEXT()
								.setMessageSeverity(vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError)
								.setMessageType(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance)
								.setPfnUserCallback(debug_utils_messenger_callback),
								nullptr, m_dispatch);
						}
					}
					// physical_device
					std::unordered_map<std::uint32_t, std::uint32_t> index_to_count;
					{
						for (const auto& gpu : m_instance.enumeratePhysicalDevices()) {
							//bool is_continue = false;
							if (ci.usage.physical_type.has_value() && ci.usage.physical_type.value() != gpu.getProperties().deviceType) continue;

							if (ci.usage.features.has_value()) {
								auto support_feature = gpu.getFeatures();
								const vk::Bool32* support_data = &support_feature.robustBufferAccess;
								const vk::Bool32* ci_data = &ci.usage.features.value().robustBufferAccess;
								bool is_continue = false;
								for (std::uint32_t i = 0; i < sizeof(vk::PhysicalDeviceFeatures) / sizeof(vk::Bool32); ++i) {
									if (ci_data[i] == true && support_data[i] == false) {
										is_continue = true; break;
									}
								}
								if (is_continue) continue;
							}
							if (ci.usage.graphic.first) if (!func::find_queue_indexes(gpu, vk::QueueFlagBits::eGraphics, ci.usage.graphic.second).has_value()) continue;
							if (ci.usage.compute.first) if (!func::find_queue_indexes(gpu, vk::QueueFlagBits::eCompute, ci.usage.compute.second).has_value()) continue;
							if (ci.usage.transfer.first) if (!func::find_queue_indexes(gpu, vk::QueueFlagBits::eTransfer, ci.usage.transfer.second).has_value()) continue;
							if (ci.usage.sparse_binding.first) if (!func::find_queue_indexes(gpu, vk::QueueFlagBits::eSparseBinding, ci.usage.sparse_binding.second).has_value()) continue;

							m_physical_device = gpu;
							break;
						}
						assert(m_physical_device != VK_NULL_HANDLE);

						m_queue_family_properties = m_physical_device.getQueueFamilyProperties();
						if (ci.usage.graphic.first) {
							auto index = func::find_queue_indexes(m_physical_device, vk::QueueFlagBits::eGraphics, ci.usage.graphic.second).value()[0];
							if (index_to_count.find(index) == index_to_count.end()) index_to_count.insert({ index , ci.usage.graphic.second });
							else { auto& proxy = index_to_count[index]; if (proxy < ci.usage.graphic.second) proxy = ci.usage.graphic.second; }
							m_queue_familys.graphic = queue_family_t().set_index(index);
						}
						if (ci.usage.compute.first) {
							auto index = func::find_queue_indexes(m_physical_device, vk::QueueFlagBits::eCompute, ci.usage.compute.second).value()[0];
							if (index_to_count.find(index) == index_to_count.end()) index_to_count.insert({ index , ci.usage.compute.second });
							else { auto& proxy = index_to_count[index]; if (proxy < ci.usage.compute.second) proxy = ci.usage.compute.second; }
							m_queue_familys.compute = queue_family_t().set_index(index);
						}
						if (ci.usage.transfer.first) {
							auto index = func::find_queue_indexes(m_physical_device, vk::QueueFlagBits::eTransfer, ci.usage.transfer.second).value()[0];
							//index = 0;
							if (index_to_count.find(index) == index_to_count.end()) index_to_count.insert({ index , ci.usage.transfer.second });
							else { auto& proxy = index_to_count[index]; if (proxy < ci.usage.transfer.second) proxy = ci.usage.transfer.second; }
							m_queue_familys.transfer = queue_family_t().set_index(index);
							//m_queue_familys.transfer = queue_family_t().set_index(index);
						}
						if (ci.usage.sparse_binding.first) {
							auto index = func::find_queue_indexes(m_physical_device, vk::QueueFlagBits::eSparseBinding, ci.usage.sparse_binding.second).value()[0];
							if (index_to_count.find(index) == index_to_count.end()) index_to_count.insert({ index , ci.usage.sparse_binding.second });
							else { auto& proxy = index_to_count[index]; if (proxy < ci.usage.sparse_binding.second) proxy = ci.usage.sparse_binding.second; }
							m_queue_familys.sparse_binding = queue_family_t().set_index(index);
						}

						if (m_queue_familys.graphic.has_value()) m_queue_familys.graphic.value().set_queues(std::vector<vk::Queue>(index_to_count[m_queue_familys.graphic.value().index], nullptr));
						if (m_queue_familys.compute.has_value()) m_queue_familys.compute.value().set_queues(std::vector<vk::Queue>(index_to_count[m_queue_familys.compute.value().index], nullptr));
						if (m_queue_familys.transfer.has_value()) m_queue_familys.transfer.value().set_queues(std::vector<vk::Queue>(index_to_count[m_queue_familys.transfer.value().index], nullptr));
						if (m_queue_familys.sparse_binding.has_value()) m_queue_familys.sparse_binding.value().set_queues(std::vector<vk::Queue>(index_to_count[m_queue_familys.sparse_binding.value().index], nullptr));

						m_support_feature = m_physical_device.getFeatures();
						if (ci.usage.features.has_value()) m_enable_feature = ci.usage.features.value();
						m_memory_property = m_physical_device.getMemoryProperties();
					}
					// device
					{
						for (const auto& iter : m_physical_device.enumerateDeviceExtensionProperties()) m_device_support_extensions.push_back(iter.extensionName);
						std::vector<std::string> extensions;
						if (ci.usage.device_extensions.has_value()) extensions.assign(ci.usage.device_extensions.value().begin(), ci.usage.device_extensions.value().end());
						if (ci.debug) extensions.push_back(VK_EXT_DEBUG_MARKER_EXTENSION_NAME);
						if (ci.surface) extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

						std::vector<const char*> enable_extensions;
						for (const auto& user : extensions) {
							for (const auto& support : m_device_support_extensions) {
								if (std::string(user) == support) {
									bool exist = false;
									for (const auto& iter : enable_extensions) if (support == iter) exist = true;
									if (!exist) enable_extensions.push_back(user.c_str());
									break;
								}
							}
						}
						if (!enable_extensions.empty()) {
							std::vector<std::string> names;
							for (const auto& iter : enable_extensions) names.push_back(iter);
							m_device_enable_extensions = names;
						}

						std::vector<vk::DeviceQueueCreateInfo> queue_cis;
						std::unordered_map<std::uint32_t, std::vector<float>> index_to_priorities;
						for (const auto& iter : index_to_count) index_to_priorities.insert({ iter.first, std::vector<float>(iter.second, 0.0f) });
						for (const auto& iter : index_to_priorities) queue_cis.push_back(
							vk::DeviceQueueCreateInfo()
							.setQueueFamilyIndex(iter.first)
							.setQueueCount(iter.second.size())
							.setPQueuePriorities(iter.second.data())
						);

						m_device = m_physical_device.createDevice(
							vk::DeviceCreateInfo()
							.setEnabledExtensionCount(enable_extensions.size())
							.setPpEnabledExtensionNames(enable_extensions.data())
							.setQueueCreateInfoCount(queue_cis.size())
							.setPQueueCreateInfos(queue_cis.data())
							.setPEnabledFeatures(m_enable_feature.has_value() ? &m_enable_feature.value() : nullptr)
						);

						if (m_queue_familys.graphic.has_value()) {
							auto& family = m_queue_familys.graphic.value();
							for (std::uint32_t i = 0; i < family.queues.size(); ++i) family.queues[i] = m_device.getQueue(family.index, i);
							family.set_command_pool(m_device.createCommandPool(
								vk::CommandPoolCreateInfo()
								.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
								.setQueueFamilyIndex(family.index))
							);
						}
						if (m_queue_familys.compute.has_value()) {
							auto& family = m_queue_familys.compute.value();
							for (std::uint32_t i = 0; i < family.queues.size(); ++i) family.queues[i] = m_device.getQueue(family.index, i);
							family.set_command_pool(m_device.createCommandPool(
								vk::CommandPoolCreateInfo()
								.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
								.setQueueFamilyIndex(family.index))
							);
						}
						if (m_queue_familys.transfer.has_value()) {
							auto& family = m_queue_familys.transfer.value();
							for (std::uint32_t i = 0; i < family.queues.size(); ++i) family.queues[i] = m_device.getQueue(family.index, i);
							family.set_command_pool(m_device.createCommandPool(
								vk::CommandPoolCreateInfo()
								.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
								.setQueueFamilyIndex(family.index))
							);
						}
						if (m_queue_familys.sparse_binding.has_value()) {
							auto& family = m_queue_familys.sparse_binding.value();
							for (std::uint32_t i = 0; i < family.queues.size(); ++i) family.queues[i] = m_device.getQueue(family.index, i);
							family.set_command_pool(m_device.createCommandPool(
								vk::CommandPoolCreateInfo()
								.setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
								.setQueueFamilyIndex(family.index))
							);
						}
						//if (m_queue_familys.graphic.has_value()) m_queues.graphic = m_device.getQueue(m_queue_familys.graphic.value().index, 0);
						//if (m_queue_familys.compute.has_value()) m_queues.compute = m_device.getQueue(m_queue_familys.compute.value().index, 0);
						//if (m_queue_familys.transfer.has_value()) m_queues.transfer = m_device.getQueue(m_queue_familys.transfer.value().index, 0);
						//if (m_queue_familys.sparse_binding.has_value()) m_queues.sparse_binding = m_device.getQueue(m_queue_familys.sparse_binding.value().index, 0);
					}
				}
				~device_t() {
					m_device.waitIdle();
					if (m_queue_familys.sparse_binding.has_value()) { m_device.destroyCommandPool(m_queue_familys.sparse_binding.value().command_pool); }
					if (m_queue_familys.transfer.has_value()) { m_device.destroyCommandPool(m_queue_familys.transfer.value().command_pool); }
					if (m_queue_familys.compute.has_value()) { m_device.destroyCommandPool(m_queue_familys.compute.value().command_pool); }
					if (m_queue_familys.graphic.has_value()) { m_device.destroyCommandPool(m_queue_familys.graphic.value().command_pool); }
					m_device.destroy();
					if (m_debug_utils_messenger.has_value()) m_instance.destroyDebugUtilsMessengerEXT(m_debug_utils_messenger.value(), nullptr, m_dispatch);
					m_instance.destroy();
				}

				decltype(auto) find_memory_index(std::uint32_t type_bits, vk::MemoryPropertyFlags flags) const {
					std::optional <std::uint32_t> result = std::nullopt;
					for (std::uint32_t i = 0; i < m_memory_property.memoryTypeCount; i++) {
						if ((type_bits & 1) == 1) {
							if ((m_memory_property.memoryTypes[i].propertyFlags & flags) == flags) {
								result = i;
								break;
							}
						}
						type_bits >>= 1;
					}
					if (!result.has_value()) std::cerr << "can't memory queue index" << std::endl;
					return result.value();
				}
				
				decltype(auto) get_queue_family(vk::QueueFlagBits const& flag) const {
					switch (flag) {
					case vk::QueueFlagBits::eGraphics: return m_queue_familys.graphic.value();
					case vk::QueueFlagBits::eCompute: return m_queue_familys.compute.value();
					case vk::QueueFlagBits::eTransfer: return m_queue_familys.transfer.value();
					case vk::QueueFlagBits::eSparseBinding: return m_queue_familys.sparse_binding.value();
					}
					assert(0);
				}
				decltype(auto) get_queue_familys() const { return m_queue_familys; }

				decltype(auto) get_instance() const { return m_instance; }
				decltype(auto) get_physical_device() const { return m_physical_device; }
				decltype(auto) get_device() const { return m_device; }
				decltype(auto) get_dispatch() const { return m_dispatch; }

				decltype(auto) begin_single_command(vk::QueueFlagBits const& flag = vk::QueueFlagBits::eGraphics) const {
					single_command_t result;
					result.second = &get_queue_family(flag);
					result.first = m_device.allocateCommandBuffers(
						vk::CommandBufferAllocateInfo()
						.setCommandPool(result.second->command_pool)
						.setCommandBufferCount(1)
						.setLevel(vk::CommandBufferLevel::ePrimary)
					)[0];
					result.first.begin(vk::CommandBufferBeginInfo().setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
					return result;
				}
				decltype(auto) end_single_command(single_command_t const& single_command) const {
					single_command.first.end();
					auto fence = m_device.createFence(vk::FenceCreateInfo());
					single_command.second->queues[0].submit({ vk::SubmitInfo()
						.setCommandBufferCount(1)
						.setPCommandBuffers(&single_command.first) 
						}, fence
					);
					m_device.waitForFences({ fence }, VK_TRUE, UINT64_MAX);
					m_device.destroyFence(fence);
					m_device.freeCommandBuffers(single_command.second->command_pool, { single_command.first });
				}

				decltype(auto) build_shader(std::wstring const& filename) const {
					std::size_t shader_size;
					char* shader_code = NULL;
					std::ifstream is(filename, std::ios::binary | std::ios::in | std::ios::ate);
					if (is.is_open()) {
						shader_size = is.tellg();
						is.seekg(0, std::ios::beg);
						shader_code = new char[shader_size];
						is.read(shader_code, shader_size);
						is.close();
						assert(shader_size > 0);
					}
					if (shader_code) {
						auto shader_module = m_device.createShaderModule(
							vk::ShaderModuleCreateInfo()
							.setCodeSize(shader_size)
							.setPCode((uint32_t*)shader_code)
						);
						delete[] shader_code;
						return shader_module;
					}
					else {
						std::wcerr << L"Error: Could not open shader file \"" << filename << L"\"" << std::endl;
						return (vk::ShaderModule)nullptr;
					}
				}
				decltype(auto) clean_shader(vk::ShaderModule const& shader_module) const {
					m_device.destroyShaderModule(shader_module);
				}

				operator vk::Instance() const { return m_instance; }
				operator vk::PhysicalDevice() const { return m_physical_device; }
				operator vk::Device() const { return m_device; }
				operator vk::DispatchLoaderDynamic() const { return m_dispatch; }
			private:
				static VKAPI_ATTR VkBool32 VKAPI_CALL debug_utils_messenger_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
					std::string prefix("");
					if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT) prefix = "[ VERBOSE ]";
					else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT) prefix = "[ INFO ]";
					else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) prefix = "[ WARNING ]";
					else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) prefix = "[ ERROR ]";
					std::stringstream debugMessage;
					debugMessage << prefix << " : [ " << pCallbackData->messageIdNumber << " ][ " << pCallbackData->pMessageIdName << " ] : " << pCallbackData->pMessage;
					if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) std::cerr << debugMessage.str() << std::endl;
					else std::cout << debugMessage.str() << std::endl;
					return VK_FALSE;
				}
			private:
				vk::Instance m_instance;
				std::vector<std::string> m_instance_support_layers;
				std::vector<std::string> m_instance_support_extensions;
				std::optional<std::vector<std::string>> m_instance_enable_layers;
				std::optional<std::vector<std::string>> m_instance_enable_extensions;
				vk::DispatchLoaderDynamic m_dispatch;
				std::optional<vk::DebugUtilsMessengerEXT> m_debug_utils_messenger;

				vk::PhysicalDevice m_physical_device;
				std::vector<vk::QueueFamilyProperties> m_queue_family_properties;
				struct queue_familys_t {
					std::optional<queue_family_t> graphic;
					std::optional<queue_family_t> compute;
					std::optional<queue_family_t> transfer;
					std::optional<queue_family_t> sparse_binding;
				}m_queue_familys;
				vk::PhysicalDeviceFeatures m_support_feature;
				std::optional<vk::PhysicalDeviceFeatures> m_enable_feature;
				vk::PhysicalDeviceMemoryProperties m_memory_property;

				vk::Device m_device;
				std::vector<std::string> m_device_support_extensions;
				std::optional<std::vector<std::string>> m_device_enable_extensions;
			};
		}
	}
	//namespace vku = graphic::vulkan;
}
