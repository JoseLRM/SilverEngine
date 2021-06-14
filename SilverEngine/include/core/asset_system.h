#pragma once

#include "defines.h"
#include "utils/serialize.h"
#include "platform/os.h"

#define SV_DEFINE_ASSET_PTR(name, ptr_type) struct name {				\
		SV_INLINE ptr_type get() const noexcept { ptr_type* ptr = reinterpret_cast<ptr_type*>(sv::get_asset_content(asset_ptr)); return ptr ? *ptr : nullptr; } \
		SV_INLINE const char* get_filepath() const noexcept { return sv::get_asset_filepath(asset_ptr); } \
		SV_INLINE void set(void* ptr) const noexcept { ptr_type* p = reinterpret_cast<ptr_type*>(sv::get_asset_content(asset_ptr)); *p = (ptr_type)ptr; } \
		SV_INLINE operator sv::AssetPtr& () { return asset_ptr; }		\
		SV_INLINE operator const sv::AssetPtr& () const { return asset_ptr; } \
		sv::AssetPtr asset_ptr;											\
    }

#define SV_DEFINE_ASSET(name, type) struct name {						\
		SV_INLINE type* get() const noexcept { return reinterpret_cast<type*>(sv::get_asset_content(asset_ptr)); } \
		SV_INLINE const char* get_filepath() const noexcept { return sv::get_asset_filepath(asset_ptr); } \
		SV_INLINE type* operator->() const noexcept { return get(); }	\
		SV_INLINE operator sv::AssetPtr& () { return asset_ptr; }			\
		SV_INLINE operator const sv::AssetPtr& () const { return asset_ptr; } \
		sv::AssetPtr asset_ptr;											\
    }

namespace sv {

    constexpr u32 ASSET_NAME_SIZE = 30u;

	struct SV_API AssetPtr {

		AssetPtr() = default;

		AssetPtr::AssetPtr(void* ptr) : ptr(ptr)
	    {
			std::atomic<i32>* ref = reinterpret_cast<std::atomic<i32>*>(ptr);
			ref->fetch_add(1);
	    }

		AssetPtr::~AssetPtr()
	    {
			if (ptr) {
				std::atomic<i32>* ref = reinterpret_cast<std::atomic<i32>*>(ptr);
				ref->fetch_add(-1);
				ptr = nullptr;
			}
	    }

		AssetPtr::AssetPtr(const AssetPtr& other)
	    {
			if (other.ptr) {
				ptr = other.ptr;
				std::atomic<i32>* ref = reinterpret_cast<std::atomic<i32>*>(ptr);
				ref->fetch_add(1);
			}
	    }

		AssetPtr& AssetPtr::operator=(const AssetPtr& other)
	    {
			if (ptr) {
				std::atomic<i32>* ref = reinterpret_cast<std::atomic<i32>*>(ptr);
				ref->fetch_add(-1);
			}

			ptr = other.ptr;

			if (ptr) {
				std::atomic<i32>* ref = reinterpret_cast<std::atomic<i32>*>(ptr);
				ref->fetch_add(1);
			}

			return *this;
	    }

		AssetPtr::AssetPtr(AssetPtr&& other) noexcept
	    {
			ptr = other.ptr;
			other.ptr = nullptr;
	    }

		AssetPtr& AssetPtr::operator=(AssetPtr&& other) noexcept
	    {
			if (ptr) {
				std::atomic<i32>* ref = reinterpret_cast<std::atomic<i32>*>(ptr);
				ref->fetch_add(-1);
			}

			ptr = other.ptr;
			other.ptr = nullptr;

			return *this;
	    }

		inline bool operator==(const AssetPtr& other) const noexcept { return ptr == other.ptr; }
		inline bool operator!=(const AssetPtr& other) const noexcept { return ptr != other.ptr; }

		void* ptr = nullptr;

    };

    void _close_assets();
    void _update_assets();

    SV_API bool create_asset(AssetPtr& asset_ptr, const char* asset_type_name);

    // Load the asset if exists and use the extension to determine how this file should be treated
    // If it is in use simply get the existing asset
    SV_API bool load_asset_from_file(AssetPtr& asset_ptr, const char* filepath);

    // Only get the asset if is in use
    SV_API bool get_asset_from_file(AssetPtr& asset_ptr, const char* filepath);
    
    SV_API void unload_asset(AssetPtr& asset_ptr);

    SV_API void* get_asset_content(const AssetPtr& asset_ptr);
    SV_API const char* get_asset_filepath(const AssetPtr& asset_ptr);

    typedef bool(*AssetCreateFn)(void* asset);
    typedef bool(*AssetLoadFileFn)(void* asset, const char* filepath);
    typedef bool(*AssetReloadFileFn)(void* asset, const char* filepath);
    typedef bool(*AssetFreeFn)(void* asset);

    struct AssetTypeDesc {
	
		const char*	  name;
		u32		  asset_size;
		const char**	  extensions;
		u32		  extension_count;
		AssetCreateFn	  create;
		AssetLoadFileFn	  load_file;
		AssetReloadFileFn reload_file;
		AssetFreeFn	  free;
		f32		  unused_time;

    };

    SV_API bool register_asset_type(const AssetTypeDesc* desc);

    SV_API void update_asset_files();
    SV_API void free_unused_assets();

    SV_INLINE void serialize_asset(Serializer& s, const AssetPtr& asset_ptr)
    {
		if (asset_ptr.ptr == nullptr) serialize_u8(s, 0u);
		else {
			const char* filepath = get_asset_filepath(asset_ptr);

			if (filepath == nullptr) {
				serialize_u8(s, 0u);
			}
			else {

				serialize_u8(s, 1u);
				serialize_string(s, filepath);
			}
		}
    }

    SV_INLINE void deserialize_asset(Deserializer& d, AssetPtr& asset_ptr)
    {
		u8 type;
		deserialize_u8(d, type);

		switch (type)
		{
		case 1u:
		{
			char filepath[FILEPATH_SIZE + 1u];

			size_t size = deserialize_string_size(d);

			if (size > FILEPATH_SIZE) {
				SV_LOG_ERROR("The asset filepath size of %ul exceeds the size limit of %ul", size, FILEPATH_SIZE);
			}

			deserialize_string(d, filepath, FILEPATH_SIZE + 1u);

			if (!load_asset_from_file(asset_ptr, filepath)) {
				SV_LOG_ERROR("Can't load the asset '%s'", filepath);
			}
		}break;
		}
    }

}
