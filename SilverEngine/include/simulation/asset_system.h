#pragma once

#include "core.h"

namespace sv {

	typedef void* AssetType;

	typedef Result(*AssetCreateFn)(const char* filePath, void* pObject);
	typedef Result(*AssetDestroyFn)(void* pObject);
	typedef bool(*AssetIsUnusedFn)(void* pObject);

	class AssetRef {

		// This is a copy of a internal struct to inline some getters
		struct Internal {
			std::atomic<int>	refCount = 0;
			float				unusedTime = float_max;
			const char*			filePath = nullptr;
			size_t				hashCode = 0u;
			void*				assetType = nullptr;
		};

	public:
		AssetRef() = default;
		~AssetRef();
		AssetRef(const AssetRef& other);
		AssetRef& operator=(const AssetRef& other);
		AssetRef(AssetRef&& other) noexcept;
		AssetRef& operator=(AssetRef&& other) noexcept;
		inline bool operator==(const AssetRef& other) const noexcept { return pInternal == other.pInternal; }
		inline bool operator!=(const AssetRef& other) const noexcept { return pInternal != other.pInternal; }

		Result load(const char* filePath);
		Result load(size_t hashCode);
		void unload();

		inline void* get() const { return pInternal ? (reinterpret_cast<ui8*>(pInternal) + sizeof(Internal)) : nullptr; }
		const char* getAssetTypeStr() const;
		inline const char* getFilePath() const { return  pInternal ? reinterpret_cast<Internal*>(pInternal)->filePath : nullptr; }
		inline size_t getHashCode() const { return  pInternal ? reinterpret_cast<Internal*>(pInternal)->hashCode : 0u; }
		inline bool hasReference() const noexcept { return pInternal != nullptr; }

	private:
		void* pInternal = nullptr;

	};

	class Asset {
	public:
		inline Result load(const char* filePath) { return m_Ref.load(filePath); }
		inline Result load(size_t hashCode) { return m_Ref.load(hashCode); }

		inline void unload() { m_Ref.unload(); }

		inline AssetRef& getRef() noexcept { return m_Ref; }
		inline const AssetRef& getRef() const noexcept { return m_Ref; }

		inline const char* getAssetTypeStr() const { return m_Ref.getAssetTypeStr(); }
		inline const char* getFilePath() const { return m_Ref.getFilePath(); }
		inline size_t getHashCode() const { return m_Ref.getHashCode(); }
		inline bool hasReference() const noexcept { return m_Ref.hasReference(); }

	protected:
		AssetRef m_Ref;

	};

	struct AssetFile {
		
		AssetType assetType = nullptr;
		void* pInternalAsset = nullptr;
		std::filesystem::file_time_type lastModification;
		ui32 refreshID;

	};

	struct AssetRegisterTypeDesc {

		const char*			name;
		const char**		pExtensions;
		ui32				extensionsCount;
		AssetCreateFn		createFn;
		AssetDestroyFn		destroyFn;
		AssetCreateFn		recreateFn;
		AssetIsUnusedFn		isUnusedFn;
		size_t				assetSize;
		float				unusedLifeTime;

	};

	Result asset_refresh();

	void asset_free_unused();
	void asset_free_unused(AssetType assetType);

	Result asset_register_type(const AssetRegisterTypeDesc* desc, AssetType* pAssetType);

	const char* asset_filepath_get(size_t hashCode);
	AssetType asset_type_get(const char* name);
	const std::string& asset_folderpath_get();

	const std::unordered_map<std::string, AssetFile>& asset_map_get();

}